#pragma once
#include <QJsonObject>
#include "minecraft/OneSixRule.h"
#include "minecraft/RawLibrary.h"
#include <minecraft/JarMod.h>
#include <minecraft/VersionFile.h>

#define CURRENT_WONKO_VERSION 0

class WonkoFormat
{
public:
	static VersionFilePtr fromJson(const QJsonDocument &doc, const QString &filename);
};
