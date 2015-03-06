#pragma once

#include <QSet>
#include <QString>
#include <QStringList>

#include "JarMod.h"
#include "Library.h"
#include "Assets.h"

namespace Minecraft
{
class Resources;

struct Patch
{
	void applyTo(Resources *resoruces);

	// game and java command line params
	QString mainClass;
	QString appletClass;
	QString overwriteMinecraftArguments;
	QString addMinecraftArguments;
	QString removeMinecraftArguments;

	// a special resource that hides the minecraft asset resource logic
	Assets assets;

	// more game command line params, this time more special
	bool shouldOverwriteTweakers = false;
	QStringList overwriteTweakers;
	QStringList addTweakers;
	QStringList removeTweakers;

	// files of type - replace all of type, add of type, remove of type
	bool shouldOverwriteLibs = false;
	QList<LibraryPtr> overwriteLibs;
	QList<LibraryPtr> addLibs;
	QList<QString> removeLibs;

	QSet<QString> traits; // tags
	QList<JarmodPtr> jarMods; // files of type... again.
};
}