#include "MinecraftResources.h"
#include <QDateTime>
#include <QDebug>

void MinecraftResources::clear()
{
	assets.clear();
	minecraftArguments.clear();
	tweakers.clear();
	mainClass.clear();
	appletClass.clear();
	libraries.clear();
	traits.clear();
	jarMods.clear();
};

void MinecraftResources::finalize()
{
	// HACK: deny april fools. my head hurts enough already.
	QDate now = QDate::currentDate();
	bool isAprilFools = now.month() == 4 && now.day() == 1;
	if (assets.endsWith("_af") && !isAprilFools)
	{
		assets = assets.left(assets.length() - 3);
	}
	if (assets.isEmpty())
	{
		assets = "legacy";
	}
}

QList<RawLibraryPtr> MinecraftResources::getActiveNormalLibs()
{
	QList<RawLibraryPtr> output;
	for (auto lib : libraries)
	{
		if (lib->isActive() && !lib->isNative())
		{
			for (auto other : output)
			{
				if (other->rawName() == lib->rawName())
				{
					qWarning() << "Multiple libraries with name" << lib->rawName() << "in library list!";
					continue;
				}
			}
			output.append(lib);
		}
	}
	return output;
}

QList<RawLibraryPtr> MinecraftResources::getActiveNativeLibs()
{
	QList<RawLibraryPtr> output;
	for (auto lib : libraries)
	{
		if (lib->isActive() && lib->isNative())
		{
			output.append(lib);
		}
	}
	return output;
}
