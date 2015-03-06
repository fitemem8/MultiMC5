#pragma once
#include <QJsonObject>
#include "minecraft/OneSixRule.h"
#include "minecraft/Library.h"
#include <minecraft/JarMod.h>
#include <minecraft/Package.h>

#define CURRENT_WONKO_VERSION 0

class WonkoFormat
{
public:
	static PackagePtr fromJson(const QJsonDocument &doc, const QString &filename);
};
