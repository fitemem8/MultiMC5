#pragma once

#include <QSet>
#include <QString>
#include <QStringList>

#include "JarMod.h"
#include "RawLibrary.h"

class MinecraftProfile;

struct MinecraftResources
{
	void applyTo(MinecraftProfile *version);

	// game and java command line params
	QString mainClass;
	QString appletClass;
	QString overwriteMinecraftArguments;
	QString addMinecraftArguments;
	QString removeMinecraftArguments;

	// a special resource that hides the minecraft asset resource logic
	QString assets;

	// more game command line params, this time more special
	bool shouldOverwriteTweakers = false;
	QStringList overwriteTweakers;
	QStringList addTweakers;
	QStringList removeTweakers;

	// files of type - replace all of type, add of type, remove of type
	bool shouldOverwriteLibs = false;
	QList<RawLibraryPtr> overwriteLibs;
	QList<RawLibraryPtr> addLibs;
	QList<QString> removeLibs;

	QSet<QString> traits; // tags
	QList<JarmodPtr> jarMods; // files of type... again.
};
