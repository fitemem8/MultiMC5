#include "WonkoFormat.h"
#include "../onesix/OneSixFormat.h"

#include "minecraft/Package.h"
#include "MMCJson.h"
#include "ParseUtils.h"
#include <QJsonArray>

using namespace MMCJson;

PackagePtr WonkoFormat::fromJson(const QJsonDocument &doc, const QString &filename)
{
	auto file = std::make_shared<Package>();
	// read metadata -- not sure if we need to here.
	auto json = doc.object();
	{
		auto formatVersion = ensureInteger(json.value("formatVersion"));
		if(formatVersion > CURRENT_WONKO_VERSION)
		{
			throw JSONValidationError(QObject::tr("Unknown wonko format version: %1").arg(formatVersion));
		}
		file->name = file->fileId = ensureString(json.value("uid"));
		file->setPatchFilename(filename);
		file->version = ensureString(json.value("version"));
		file->type = ensureString(json.value("type"));

		qint64 unixTimestamp = ensureInteger(json.value("time"));
		file->m_releaseTime = QDateTime::fromMSecsSinceEpoch(unixTimestamp * 1000);
		if(json.contains("requires"))
		{
			auto arr = ensureArray(json.value("requires"));
			for(auto item: arr)
			{
				auto obj = ensureObject(item);
				QString uid = ensureString(obj.value("uid"));
				QString version;
				if(obj.contains("version"))
				{
					version = ensureString(obj.value("version"));
				}
				file->dependencies[uid] = version;
			}
		}
	}

	auto & resourceData = file->resources;
	auto data = json.value("data").toObject();
	// read actual data
	{
		if (data.contains("general.traits"))
		{
			for (auto traitVal : ensureArray(data.value("general.traits")))
			{
				resourceData.traits.insert(ensureString(traitVal));
			}
		}

		if (data.contains("java.libraries"))
		{
			for (auto libVal : ensureArray(data.value("java.libraries")))
			{
				QJsonObject libObj = ensureObject(libVal);
				// NOTE: parsing using the OneSix format here.
				auto lib = OneSixFormat::readRawLibraryPlus(libObj, filename);
				resourceData.libraries.addLibs.append(lib);
			}
		}

		if (data.contains("java.mainClass"))
		{
			resourceData.mainClass = ensureString(data.value("java.mainClass"));
		}

		if (data.contains("mc.appletClass"))
		{
			resourceData.appletClass = ensureString(data.value("mc.appletClass"));
		}

		if (data.contains("mc.assets"))
		{
			resourceData.assets = ensureString(data.value("mc.assets"));
		}

		if (data.contains("mc.arguments"))
		{
			resourceData.addMinecraftArguments = ensureString(data.value("mc.arguments"));
		}

		if (data.contains("mc.tweakers"))
		{
			for (auto tweakerVal : ensureArray(data.value("mc.tweakers")))
			{
				resourceData.addTweakers.append(ensureString(tweakerVal));
			}
		}
	}

	return file;
}
