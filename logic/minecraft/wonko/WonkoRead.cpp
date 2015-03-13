#include "WonkoFormat.h"

#include <QRegularExpression>

#include "../onesix/OneSixFormat.h"
#include "minecraft/Package.h"
#include "WonkoPackageVersion.h"
#include "Json.h"
#include "minecraft/Libraries.h"

class WonkoReadException : public Exception
{
public:
	explicit WonkoReadException(const QString &msg) : Exception(msg)
	{
	}
};

using namespace Json;

static OneSixLibraryPtr convertLibrary(LibraryPtr ptr, const bool isNative)
{
	std::shared_ptr<OneSixLibrary> out = std::make_shared<OneSixLibrary>();
	std::shared_ptr<WonkoLibrary> lib = std::dynamic_pointer_cast<WonkoLibrary>(ptr);

	out->m_name = lib->m_name;
	out->m_base_url = lib->m_base_url;
	out->m_absolute_url = lib->m_absolute_url;
	out->insertType = lib->insertType;
	out->insertData = lib->insertData;
	out->dependType = lib->dependType;

	QSet<OpSys> platforms;

	// platforms -> rules
	if (!lib->platforms().isEmpty() && lib->platforms() != WonkoLibrary::allPlatforms())
	{
		QMap<QString, OpSys> mapping;
		mapping["win32"] = Os_Windows;
		mapping["win64"] = Os_Windows;
		mapping["lin32"] = Os_Linux;
		mapping["lin64"] = Os_Linux;
		mapping["osx32"] = Os_OSX;
		mapping["osx64"] = Os_OSX;

		out->applyRules = true;
		out->m_rules += ImplicitRule::create(Disallow);
		for (const QString &os : lib->platforms())
		{
			platforms += mapping.contains(os) ? mapping[os] : Os_Other;
		}
		for (const OpSys system : platforms)
		{
			out->m_rules += OsRule::create(Allow, system, QString());
		}
	}

	if (isNative && lib->m_absolute_url.isValid())
	{
		QString natives = lib->m_absolute_url.toString();
		natives = QRegularExpression("natives[^.]*").match(natives).captured();
		natives = natives.replace("32", "${arch}").replace("64", "${arch}");
		if (!natives.isEmpty())
		{
			for (const OpSys system : platforms)
			{
				out->m_native_classifiers[system] = natives;
			}

			out->applyExcludes = true;
			out->extract_excludes = QStringList() << "META-INF/";
		}
	}

	return out;
}

static std::shared_ptr<Minecraft::Libraries>
convertLibs(std::shared_ptr<Minecraft::Libraries> libs, const bool isNative)
{
	if (libs == nullptr)
	{
		return std::make_shared<Minecraft::Libraries>();
	}

	std::shared_ptr<Minecraft::Libraries> out = std::make_shared<Minecraft::Libraries>();
	out->shouldOverwriteLibs = libs->shouldOverwriteLibs;
	out->removeLibs = libs->removeLibs;

	auto convertListOfLibraries = [isNative](const QList<LibraryPtr> libs)
	{
		QMap<QString, OneSixLibraryPtr> out;
		for (LibraryPtr lib : libs)
		{
			OneSixLibraryPtr converted = convertLibrary(lib, isNative);
			if (out.contains(converted->rawName()))
			{
				OneSixLibraryPtr existing = out[converted->rawName()];
				existing->m_native_classifiers.unite(converted->m_native_classifiers);
				if (!converted->m_rules.isEmpty())
				{
					// remove the implicit disallow
					converted->m_rules.removeFirst();
				}
				existing->m_rules.append(converted->m_rules);
			}
			else
			{
				out.insert(converted->rawName(), converted);
			}
		}
		QList<LibraryPtr> result;
		for (LibraryPtr ptr : out.values())
		{
			result.append(ptr);
		}
		return result;
	};

	out->addLibs = convertListOfLibraries(libs->addLibs);
	out->overwriteLibs = convertListOfLibraries(libs->overwriteLibs);

	return out;
}

PackagePtr WonkoFormat::fromJson(const QJsonDocument &doc, const QString &filename)
{
	WonkoVersionPtr file = std::make_shared<WonkoPackageVersion>();
	PackagePtr result = std::make_shared<Package>();
	// read metadata -- not sure if we need to here.
	const QJsonObject json = ensureObject(doc);
	{
		const int formatVersion = ensureInteger(json, "formatVersion");
		if (formatVersion > CURRENT_WONKO_VERSION)
		{
			throw WonkoReadException(
				QObject::tr("Unknown wonko format version: %1").arg(formatVersion));
		}
		file->load(json);
		result->name = result->fileId = file->uid();
		result->setPatchFilename(filename);
		result->version = file->descriptor();
		result->type = file->typeString();
		result->m_releaseTime = file->timestamp();
		result->dependencies = file->dependencies();
	}

	// actual data
	{
		// folders -> traits
		if (file->resource<FoldersResource>("general.folders"))
		{
			std::shared_ptr<FoldersResource> folders =
				file->resource<FoldersResource>("general.folders");
			const QStringList paths = folders->folderPaths();
			if (paths.contains("minecraft/texturepacks"))
			{
				result->resources.traits.insert("texturepacks");
			}
			else if (paths.contains("minecraft/resourcepacks"))
			{
				result->resources.traits.insert("resourcepacks");
			}
			else
			{
				result->resources.traits.insert("no-texturepacks");
			}
		}

		// libraries and libraries
		result->resources.libraries =
			convertLibs(file->resource<Minecraft::Libraries>("java.libraries"), false);
		result->resources.natives =
			convertLibs(file->resource<Minecraft::Libraries>("java.natives"), true);

		// various odd bits and pieces
		if (file->resource<StringResource>("java.mainClass"))
		{
			result->resources.mainClass =
				file->resource<StringResource>("java.mainClass")->data();
		}
		if (file->resource<StringResource>("mc.appletClass"))
		{
			result->resources.appletClass =
				file->resource<StringResource>("mc.appletClass")->data();
		}
		if (file->resource<StringResource>("mc.assets"))
		{
			result->resources.assets = file->resource<StringResource>("mc.assets")->data();
		}
		if (file->resource<StringResource>("mc.arguments"))
		{
			result->resources.addMinecraftArguments =
				file->resource<StringResource>("mc.arguments")->data();
		}
		if (file->resource<StringListResource>("mc.tweakers"))
		{
			result->resources.addTweakers =
				file->resource<StringListResource>("mc.tweakers")->data();
		}
	}

	return result;
}
