#pragma once
#include <QJsonObject>
#include "minecraft/OneSixRule.h"
#include "minecraft/RawLibrary.h"
#include <minecraft/JarMod.h>
#include <minecraft/VersionFile.h>

#define CURRENT_MINIMUM_LAUNCHER_VERSION 14

class OneSixFormat
{
public:
	static QJsonDocument toJson(VersionFilePtr file, bool saveOrder);
	static QJsonObject toJson(RawLibraryPtr raw);
	static QJsonObject toJson(JarmodPtr jarmod);
	static QJsonObject toJson(std::shared_ptr<ImplicitRule> rule);
	static QJsonObject toJson(std::shared_ptr<OsRule> rule);

	static VersionFilePtr fromJson(const QJsonDocument &doc, const QString &filename, const bool requireOrder);
	static JarmodPtr fromJson(const QJsonObject &libObj, const QString &filename);

	static RawLibraryPtr readRawLibraryPlus(const QJsonObject &libObj, const QString &filename);
};
