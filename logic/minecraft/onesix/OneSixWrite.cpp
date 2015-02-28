#include "OneSixFormat.h"
#include "minecraft/VersionFile.h"
#include "MMCJson.h"
#include "ParseUtils.h"
#include <QJsonArray>

using namespace MMCJson;

QJsonObject OneSixFormat::toJson(std::shared_ptr<ImplicitRule> rule)
{
	QJsonObject ruleObj;
	ruleObj.insert("action", rule->m_result == Allow ? QString("allow") : QString("disallow"));
	return ruleObj;
}

QJsonObject OneSixFormat::toJson(std::shared_ptr<OsRule> rule)
{
	QJsonObject ruleObj;
	ruleObj.insert("action", rule->m_result == Allow ? QString("allow") : QString("disallow"));
	QJsonObject osObj;
	{
		osObj.insert("name", OpSys_toString(rule->m_system));
		osObj.insert("version", rule->m_version_regexp);
	}
	ruleObj.insert("os", osObj);
	return ruleObj;
}

QJsonObject OneSixFormat::toJson(RawLibraryPtr raw)
{
	QJsonObject libRoot;
	libRoot.insert("name", (QString)raw->m_name);
	if (raw->m_absolute_url.size())
		libRoot.insert("MMC-absoluteUrl", raw->m_absolute_url);
	if (raw->m_hint.size())
		libRoot.insert("MMC-hint", raw->m_hint);
	if (raw->m_base_url != "http://" + URLConstants::AWS_DOWNLOAD_LIBRARIES &&
		raw->m_base_url != "https://" + URLConstants::AWS_DOWNLOAD_LIBRARIES &&
		raw->m_base_url != "https://" + URLConstants::LIBRARY_BASE && !raw->m_base_url.isEmpty())
	{
		libRoot.insert("url", raw->m_base_url);
	}
	if (raw->isNative())
	{
		QJsonObject nativeList;
		auto iter = raw->m_native_classifiers.begin();
		while (iter != raw->m_native_classifiers.end())
		{
			nativeList.insert(OpSys_toString(iter.key()), iter.value());
			iter++;
		}
		libRoot.insert("natives", nativeList);
		if (raw->extract_excludes.size())
		{
			QJsonArray excludes;
			QJsonObject extract;
			for (auto exclude : raw->extract_excludes)
			{
				excludes.append(exclude);
			}
			extract.insert("exclude", excludes);
			libRoot.insert("extract", extract);
		}
	}
	if (raw->m_rules.size())
	{
		QJsonArray allRules;
		for (auto &rule : raw->m_rules)
		{
			QJsonObject ruleObj;
			auto implicitRule = std::dynamic_pointer_cast<ImplicitRule>(rule);
			auto osRule = std::dynamic_pointer_cast<ImplicitRule>(rule);
			if(implicitRule)
			{
				ruleObj = toJson(implicitRule);
				allRules.append(ruleObj);
			}
			else if(osRule)
			{
				ruleObj = toJson(implicitRule);
				allRules.append(ruleObj);
			}
			else
			{
				qWarning() << "Couldn't recognize rule type!";
			}
		}
		libRoot.insert("rules", allRules);
	}
	return libRoot;
}

QJsonDocument OneSixFormat::toJson(VersionFilePtr file, bool saveOrder)
{
	QJsonObject root;
	if (saveOrder)
	{
		root.insert("order", file->order);
	}
	writeString(root, "name", file->name);
	writeString(root, "fileId", file->fileId);
	writeString(root, "version", file->version);
	writeString(root, "mcVersion", file->mcVersion);
	// FIXME: write version of library with name 'net.minecraft:minecraft'
	// writeString(root, "id", file->id);
	writeString(root, "mainClass", file->mainClass);
	writeString(root, "appletClass", file->appletClass);
	writeString(root, "minecraftArguments", file->overwriteMinecraftArguments);
	writeString(root, "+minecraftArguments", file->addMinecraftArguments);
	writeString(root, "-minecraftArguments", file->removeMinecraftArguments);
	writeString(root, "type", file->type);
	writeString(root, "assets", file->assets);
	if (file->fileId == "net.minecraft")
	{
		writeString(root, "releaseTime", file->m_releaseTimeString);
	}
	root.insert("minimumLauncherVersion", CURRENT_MINIMUM_LAUNCHER_VERSION);
	writeStringList(root, "tweakers", file->overwriteTweakers);
	writeStringList(root, "+tweakers", file->addTweakers);
	writeStringList(root, "-tweakers", file->removeTweakers);
	writeStringList(root, "+traits", file->traits.toList());
	writeObjectList<OneSixFormat>(root, "libraries", file->overwriteLibs);
	writeObjectList<OneSixFormat>(root, "+libraries", file->addLibs);
	writeObjectList<OneSixFormat>(root, "+jarMods", file->jarMods);
	// FIXME: removed libs are special snowflakes.
	if (file->removeLibs.size())
	{
		QJsonArray array;
		for (auto lib : file->removeLibs)
		{
			QJsonObject rmlibobj;
			rmlibobj.insert("name", lib);
			array.append(rmlibobj);
		}
		root.insert("-libraries", array);
	}
	// write the contents to a json document.
	{
		QJsonDocument out;
		out.setObject(root);
		return out;
	}
}

QJsonObject OneSixFormat::toJson(JarmodPtr mod)
{
	QJsonObject out;
	writeString(out, "name", mod->name);
	return out;
}
