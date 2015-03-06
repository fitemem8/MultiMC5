#pragma once
#include <QJsonObject>
#include "minecraft/OneSixRule.h"
#include "minecraft/Library.h"
#include <minecraft/JarMod.h>
#include <minecraft/Package.h>

#define CURRENT_MINIMUM_LAUNCHER_VERSION 14

class OneSixFormat
{
public:
	static QJsonDocument toJson(PackagePtr file, bool saveOrder);
	static QJsonObject toJson(LibraryPtr raw);
	static QJsonObject toJson(JarmodPtr jarmod);
	static QJsonObject toJson(std::shared_ptr<ImplicitRule> rule);
	static QJsonObject toJson(std::shared_ptr<OsRule> rule);

	static PackagePtr fromJson(const QJsonDocument &doc, const QString &filename, const bool requireOrder);
	static JarmodPtr fromJson(const QJsonObject &libObj, const QString &filename);

	static LibraryPtr readRawLibraryPlus(const QJsonObject &libObj, const QString &filename);
};
