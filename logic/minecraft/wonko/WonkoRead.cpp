#include "WonkoFormat.h"
#include "../onesix/OneSixFormat.h"

#include "minecraft/VersionFile.h"
#include "MMCJson.h"
#include "ParseUtils.h"
#include <QJsonArray>

using namespace MMCJson;

VersionFilePtr WonkoFormat::fromJson(const QJsonDocument &doc, const QString &filename)
{
	auto file = std::make_shared<VersionFile>();
	// read metadata -- not sure if we need to here.
	auto json = doc.object();
	{
		auto formatVersion = ensureInteger(json.value("formatVersion"));
		if(formatVersion > CURRENT_WONKO_VERSION)
		{
			throw JSONValidationError(QObject::tr("Unknown wonko format version: %1").arg(formatVersion));
		}
		file->name = file->fileId = ensureString(json.value("uid"));
		file->filename = filename;
		file->version = ensureString(json.value("version"));
		file->type = ensureString(json.value("type"));

		qint64 unixTimestamp = ensureInteger(json.value("time"));
		file->m_releaseTime = QDateTime::fromMSecsSinceEpoch(unixTimestamp * 1000);
		//FIXME: handle better.
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
				if(uid == "net.minecraft")
				{
					file->mcVersion = version;
					break;
				}
			}
		}
	}

	auto data = json.value("data").toObject();
	// read actual data
	{
		if (data.contains("general.traits"))
		{
			for (auto traitVal : ensureArray(data.value("general.traits")))
			{
				file->traits.insert(ensureString(traitVal));
			}
		}

		if (data.contains("java.libraries"))
		{
			for (auto libVal : ensureArray(data.value("java.libraries")))
			{
				QJsonObject libObj = ensureObject(libVal);
				// NOTE: parsing using the OneSix format here.
				auto lib = OneSixFormat::readRawLibraryPlus(libObj, filename);
				file->addLibs.append(lib);
			}
		}

		if (data.contains("java.mainClass"))
		{
			file->mainClass = ensureString(data.value("java.mainClass"));
		}

		if (data.contains("mc.appletClass"))
		{
			file->appletClass = ensureString(data.value("mc.appletClass"));
		}

		if (data.contains("mc.assets"))
		{
			file->assets = ensureString(data.value("mc.assets"));
		}

		if (data.contains("mc.arguments"))
		{
			file->addMinecraftArguments = ensureString(data.value("mc.arguments"));
		}

		if (data.contains("mc.tweakers"))
		{
			for (auto tweakerVal : ensureArray(data.value("mc.tweakers")))
			{
				file->addTweakers.append(ensureString(tweakerVal));
			}
		}
	}

	return file;
}
