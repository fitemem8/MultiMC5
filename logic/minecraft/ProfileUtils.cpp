#include "ProfileUtils.h"
#include "logic/minecraft/VersionFilterData.h"
#include "logic/MMCJson.h"
#include "logger/QsLog.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QRegularExpression>

namespace ProfileUtils
{

static const int currentOrderFileVersion = 1;

bool writeOverrideOrders(QString path, const PatchOrder &order)
{
	QJsonObject obj;
	obj.insert("version", currentOrderFileVersion);
	QJsonArray orderArray;
	for(auto str: order)
	{
		orderArray.append(str);
	}
	obj.insert("order", orderArray);
	QFile orderFile(path);
	if (!orderFile.open(QFile::WriteOnly))
	{
		QLOG_ERROR() << "Couldn't open" << orderFile.fileName()
					 << "for writing:" << orderFile.errorString();
		return false;
	}
	orderFile.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
	return true;
}

bool readOverrideOrders(QString path, PatchOrder &order)
{
	QFile orderFile(path);
	if (!orderFile.exists())
	{
		QLOG_WARN() << "Order file doesn't exist. Ignoring.";
		return false;
	}
	if (!orderFile.open(QFile::ReadOnly))
	{
		QLOG_ERROR() << "Couldn't open" << orderFile.fileName()
					 << " for reading:" << orderFile.errorString();
		QLOG_WARN() << "Ignoring overriden order";
		return false;
	}

	// and it's valid JSON
	QJsonParseError error;
	QJsonDocument doc = QJsonDocument::fromJson(orderFile.readAll(), &error);
	if (error.error != QJsonParseError::NoError)
	{
		QLOG_ERROR() << "Couldn't parse" << orderFile.fileName() << ":" << error.errorString();
		QLOG_WARN() << "Ignoring overriden order";
		return false;
	}

	// and then read it and process it if all above is true.
	try
	{
		auto obj = MMCJson::ensureObject(doc);
		// check order file version.
		auto version = MMCJson::ensureInteger(obj.value("version"), "version");
		if (version != currentOrderFileVersion)
		{
			throw JSONValidationError(QObject::tr("Invalid order file version, expected %1")
										  .arg(currentOrderFileVersion));
		}
		auto orderArray = MMCJson::ensureArray(obj.value("order"));
		for(auto item: orderArray)
		{
			order.append(MMCJson::ensureString(item));
		}
	}
	catch (JSONValidationError &err)
	{
		QLOG_ERROR() << "Couldn't parse" << orderFile.fileName() << ": bad file format";
		QLOG_WARN() << "Ignoring overriden order";
		order.clear();
		return false;
	}
	return true;
}

VersionFilePtr parseJsonFile(const QFileInfo &fileInfo, const bool requireOrder, bool isFTB)
{
	QFile file(fileInfo.absoluteFilePath());
	if (!file.open(QFile::ReadOnly))
	{
		throw JSONValidationError(QObject::tr("Unable to open the version file %1: %2.")
									  .arg(fileInfo.fileName(), file.errorString()));
	}
	QJsonParseError error;
	QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
	if (error.error != QJsonParseError::NoError)
	{
		throw JSONValidationError(
			QObject::tr("Unable to process the version file %1: %2 at %3.")
				.arg(fileInfo.fileName(), error.errorString())
				.arg(error.offset));
	}
	return VersionFile::fromJson(doc, file.fileName(), requireOrder, isFTB);
}

VersionFilePtr parseBinaryJsonFile(const QFileInfo &fileInfo)
{
	QFile file(fileInfo.absoluteFilePath());
	if (!file.open(QFile::ReadOnly))
	{
		throw JSONValidationError(QObject::tr("Unable to open the version file %1: %2.")
									  .arg(fileInfo.fileName(), file.errorString()));
	}
	QJsonDocument doc = QJsonDocument::fromBinaryData(file.readAll());
	file.close();
	if (doc.isNull())
	{
		file.remove();
		throw JSONValidationError(
			QObject::tr("Unable to process the version file %1.").arg(fileInfo.fileName()));
	}
	return VersionFile::fromJson(doc, file.fileName(), false, false);
}

void removeLwjglFromPatch(VersionFilePtr patch)
{
	auto filter = [](QList<RawLibraryPtr>& libs)
	{
		QList<RawLibraryPtr> filteredLibs;
		for (auto lib : libs)
		{
			if (!g_VersionFilterData.lwjglWhitelist.contains(lib->artifactPrefix()))
			{
				filteredLibs.append(lib);
			}
		}
		libs = filteredLibs;
	};
	filter(patch->addLibs);
	filter(patch->overwriteLibs);
}
}
