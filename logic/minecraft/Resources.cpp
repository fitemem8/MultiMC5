#include "Resources.h"
#include <QDebug>
namespace Minecraft
{
void Resources::clear()
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

void Resources::finalize()
{
	assets.finalize();
}

QList<LibraryPtr> Resources::getActiveNormalLibs()
{
	QList<LibraryPtr> output;
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

QList<LibraryPtr> Resources::getActiveNativeLibs()
{
	QList<LibraryPtr> output;
	for (auto lib : libraries)
	{
		if (lib->isActive() && lib->isNative())
		{
			output.append(lib);
		}
	}
	return output;
}
}