#include "OneSixFormat.h"
#include "minecraft/VersionFile.h"
#include "MMCJson.h"
#include "ParseUtils.h"
#include <QJsonArray>

using namespace MMCJson;

RuleAction RuleAction_fromString(QString name)
{
	if (name == "allow")
		return Allow;
	if (name == "disallow")
		return Disallow;
	return Defer;
}

QList<std::shared_ptr<Rule>> readLibraryRules(const QJsonObject &objectWithRules)
{
	QList<std::shared_ptr<Rule>> rules;
	auto rulesVal = objectWithRules.value("rules");
	if (!rulesVal.isArray())
		return rules;

	QJsonArray ruleList = rulesVal.toArray();
	for (auto ruleVal : ruleList)
	{
		std::shared_ptr<Rule> rule;
		if (!ruleVal.isObject())
			continue;
		auto ruleObj = ruleVal.toObject();
		auto actionVal = ruleObj.value("action");
		if (!actionVal.isString())
			continue;
		auto action = RuleAction_fromString(actionVal.toString());
		if (action == Defer)
			continue;

		auto osVal = ruleObj.value("os");
		if (!osVal.isObject())
		{
			// add a new implicit action rule
			rules.append(ImplicitRule::create(action));
			continue;
		}

		auto osObj = osVal.toObject();
		auto osNameVal = osObj.value("name");
		if (!osNameVal.isString())
			continue;
		OpSys requiredOs = OpSys_fromString(osNameVal.toString());
		QString versionRegex = osObj.value("version").toString();
		// add a new OS rule
		rules.append(OsRule::create(action, requiredOs, versionRegex));
	}
    return rules;
}


RawLibraryPtr readRawLibrary(const QJsonObject &libObj, const QString &filename)
{
	RawLibraryPtr out(new RawLibrary());
	if (!libObj.contains("name"))
	{
		throw JSONValidationError(filename +
								  "contains a library that doesn't have a 'name' field");
	}
	out->m_name = libObj.value("name").toString();

	auto readString = [libObj, filename](const QString & key, QString & variable) -> bool
	{
		if (!libObj.contains(key))
			return false;
		QJsonValue val = libObj.value(key);

		if (!val.isString())
		{
			qWarning() << key << "is not a string in" << filename << "(skipping)";
			return false;
		}

		variable = val.toString();
		return true;
	};

	readString("url", out->m_base_url);
	readString("MMC-hint", out->m_hint);
	readString("MMC-absulute_url", out->m_absolute_url);
	readString("MMC-absoluteUrl", out->m_absolute_url);
	readString("absoluteUrl", out->m_absolute_url);
	if (libObj.contains("extract"))
	{
		out->applyExcludes = true;
		auto extractObj = ensureObject(libObj.value("extract"));
		for (auto excludeVal : ensureArray(extractObj.value("exclude")))
		{
			out->extract_excludes.append(ensureString(excludeVal));
		}
	}
	if (libObj.contains("natives"))
	{
		QJsonObject nativesObj = ensureObject(libObj.value("natives"));
		for (auto it = nativesObj.begin(); it != nativesObj.end(); ++it)
		{
			if (!it.value().isString())
			{
				qWarning() << filename << "contains an invalid native (skipping)";
			}
			OpSys opSys = OpSys_fromString(it.key());
			if (opSys != Os_Other)
			{
				out->m_native_classifiers[opSys] = it.value().toString();
			}
		}
	}
	if (libObj.contains("rules"))
	{
		out->applyRules = true;
		out->m_rules = readLibraryRules(libObj);
	}
	return out;
}

RawLibraryPtr OneSixFormat::readRawLibraryPlus(const QJsonObject &libObj, const QString &filename)
{
	auto lib = readRawLibrary(libObj, filename);
	if (libObj.contains("insert"))
	{
		QJsonValue insertVal = ensureExists(libObj.value("insert"), "library insert rule");
		if (insertVal.isString())
		{
			// it's just a simple string rule. OK.
			QString insertString = insertVal.toString();
			if (insertString == "apply")
			{
				lib->insertType = RawLibrary::Apply;
			}
			else if (insertString == "prepend")
			{
				lib->insertType = RawLibrary::Prepend;
			}
			else if (insertString == "append")
			{
				lib->insertType = RawLibrary::Append;
			}
			else if (insertString == "replace")
			{
				lib->insertType = RawLibrary::Replace;
			}
			else
			{
				throw JSONValidationError("A '+' library in " + filename + " contains an invalid insert type");
			}
		}
		else if (insertVal.isObject())
		{
			// it's a more complex rule, specifying what should be:
			//   * replaced (for now only this)
			// this was never used, AFAIK. tread carefully.
			QJsonObject insertObj = insertVal.toObject();
			if (insertObj.isEmpty())
			{
				throw JSONValidationError("Empty compound insert rule in " + filename);
			}
			QString insertString = insertObj.keys().first();
			// really, only replace makes sense in combination with
			if(insertString != "replace")
			{
				throw JSONValidationError("Compound insert rule is not 'replace' in " + filename);
			}
			lib->insertData = insertObj.value(insertString).toString();
		}
		else
		{
			throw JSONValidationError("A '+' library in " + filename + " contains an unknown/invalid insert rule");
		}
	}
	if (libObj.contains("MMC-depend"))
	{
		const QString dependString = ensureString(libObj.value("MMC-depend"));
		if (dependString == "hard")
		{
			lib->dependType = RawLibrary::Hard;
		}
		else if (dependString == "soft")
		{
			lib->dependType = RawLibrary::Soft;
		}
		else
		{
			throw JSONValidationError("A '+' library in " + filename + " contains an invalid depend type");
		}
	}
	return lib;
}

VersionFilePtr OneSixFormat::fromJson(const QJsonDocument& doc, const QString& filename, const bool requireOrder)
{
	VersionFilePtr out(new VersionFile());
	if (doc.isEmpty() || doc.isNull())
	{
		throw JSONValidationError(filename + " is empty or null");
	}
	if (!doc.isObject())
	{
		throw JSONValidationError(filename + " is not an object");
	}

	QJsonObject root = doc.object();

	if (requireOrder)
	{
		if (root.contains("order"))
		{
			out->setOrder(ensureInteger(root.value("order")));
		}
		else
		{
			// FIXME: evaluate if we don't want to throw exceptions here instead
			qCritical() << filename << "doesn't contain an order field";
		}
	}

	out->name = root.value("name").toString();
	out->fileId = root.value("fileId").toString();
	out->version = root.value("version").toString();
	QString mcVersion = root.value("mcVersion").toString();
	if(!mcVersion.isEmpty())
	{
		out->dependencies["net.minecraft"] = mcVersion;
	}
	out->setPatchFilename(filename);

	auto readString = [root](const QString &key, QString &variable)
	{
		if (root.contains(key))
		{
			variable = ensureString(root.value(key));
		}
	};

	auto readStringRet = [root](const QString &key) -> QString
	{
		if (root.contains(key))
		{
			return ensureString(root.value(key));
		}
		return QString();
	};

	auto & resourceData = out->resources;
	readString("mainClass", resourceData.mainClass);
	readString("appletClass", resourceData.appletClass);
	{
		QString minecraftArguments;
		QString processArguments;
		readString("minecraftArguments", minecraftArguments);
		if (minecraftArguments.isEmpty())
		{
			readString("processArguments", processArguments);
			QString toCompare = processArguments.toLower();
			if (toCompare == "legacy")
			{
				minecraftArguments = " ${auth_player_name} ${auth_session}";
			}
			else if (toCompare == "username_session")
			{
				minecraftArguments = "--username ${auth_player_name} --session ${auth_session}";
			}
			else if (toCompare == "username_session_version")
			{
				minecraftArguments = "--username ${auth_player_name} "
									"--session ${auth_session} "
									"--version ${profile_name}";
			}
		}
		if(!minecraftArguments.isEmpty())
		{
			resourceData.overwriteMinecraftArguments = minecraftArguments;
		}

	}
	readString("+minecraftArguments", resourceData.addMinecraftArguments);
	readString("-minecraftArguments", resourceData.removeMinecraftArguments);
	readString("type", out->type);

	parse_timestamp(readStringRet("releaseTime"), out->m_releaseTimeString, out->m_releaseTime);

	readString("assets", resourceData.assets);

	if (root.contains("minimumLauncherVersion"))
	{
		int minimumLauncherVersion = ensureInteger(root.value("minimumLauncherVersion"));
		if(minimumLauncherVersion > CURRENT_MINIMUM_LAUNCHER_VERSION)
		{
			throw JSONValidationError(QString("patch %1 is in a newer format than MultiMC can handle").arg(filename));
		}
	}

	if (root.contains("tweakers"))
	{
		resourceData.shouldOverwriteTweakers = true;
		for (auto tweakerVal : ensureArray(root.value("tweakers")))
		{
			resourceData.overwriteTweakers.append(ensureString(tweakerVal));
		}
	}

	if (root.contains("+tweakers"))
	{
		for (auto tweakerVal : ensureArray(root.value("+tweakers")))
		{
			resourceData.addTweakers.append(ensureString(tweakerVal));
		}
	}

	if (root.contains("-tweakers"))
	{
		for (auto tweakerVal : ensureArray(root.value("-tweakers")))
		{
			resourceData.removeTweakers.append(ensureString(tweakerVal));
		}
	}

	if (root.contains("+traits"))
	{
		for (auto tweakerVal : ensureArray(root.value("+traits")))
		{
			resourceData.traits.insert(ensureString(tweakerVal));
		}
	}

	if (root.contains("libraries"))
	{
		resourceData.shouldOverwriteLibs = true;
		for (auto libVal : ensureArray(root.value("libraries")))
		{
			auto libObj = ensureObject(libVal);

			auto lib = readRawLibrary(libObj, filename);
			resourceData.overwriteLibs.append(lib);
		}
	}

	QString minecraftVersion;
	readString("id", minecraftVersion);
	if(!minecraftVersion.isEmpty())
	{
		auto libptr = std::make_shared<RawLibrary>();
		auto name = QString("net.minecraft:minecraft:%1").arg(minecraftVersion);
		auto url = QString("http://s3.amazonaws.com/Minecraft.Download/versions/%1/%2.jar")
			.arg(minecraftVersion)
			.arg(minecraftVersion);
		libptr->setRawName(GradleSpecifier(name));
		libptr->setAbsoluteUrl(url);
	}

	if (root.contains("+jarMods"))
	{
		for (auto libVal : ensureArray(root.value("+jarMods")))
		{
			QJsonObject libObj = ensureObject(libVal);
			// parse the jarmod
			JarmodPtr lib = OneSixFormat::fromJson(libObj, filename);
			// and add to jar mods
			resourceData.jarMods.append(lib);
		}
	}

	if (root.contains("+libraries"))
	{
		for (auto libVal : ensureArray(root.value("+libraries")))
		{
			QJsonObject libObj = ensureObject(libVal);
			// parse the library
			auto lib = readRawLibraryPlus(libObj, filename);
			resourceData.addLibs.append(lib);
		}
	}

	if (root.contains("-libraries"))
	{
		for (auto libVal : ensureArray(root.value("-libraries")))
		{
			auto libObj = ensureObject(libVal);
			resourceData.removeLibs.append(ensureString(libObj.value("name")));
		}
	}
	return out;
}

JarmodPtr OneSixFormat::fromJson(const QJsonObject &libObj, const QString &filename)
{
	JarmodPtr out(new Jarmod());
	if (!libObj.contains("name"))
	{
		throw JSONValidationError(filename +
								  "contains a jarmod that doesn't have a 'name' field");
	}
	out->name = libObj.value("name").toString();
	return out;
}
