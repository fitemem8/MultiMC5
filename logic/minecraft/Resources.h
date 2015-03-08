#pragma once

#include "Library.h"
#include "JarMod.h"
#include "Assets.h"
#include "Libraries.h"
#include <tasks/SequentialTask.h>

#include <QSet>
#include <QList>
#include <QString>
#include <QStringList>

namespace Minecraft
{

struct Resources
{
	Task * updateTask()
	{
		auto sequence = new SequentialTask();
		if(auto librariesTask = libraries.updateTask())
		{
			sequence->addTask(std::shared_ptr<Task>(librariesTask));
		}
		if(auto assetsTask = assets.updateTask())
		{
			sequence->addTask(std::shared_ptr<Task>(assetsTask));
		}
		if(sequence->size() == 0)
		{
			delete sequence;
			return nullptr;
		}
		return sequence;
	}

	void clear()
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

	void finalize()
	{
		assets.finalize();
	}

	/// Assets type - "legacy" or a version ID
	Assets assets;
	/**
	 * arguments that should be used for launching minecraft
	 *
	 * ex: "--username ${auth_player_name} --session ${auth_session}
	 *      --version ${version_name} --gameDir ${game_directory} --assetsDir ${game_assets}"
	 */
	QString minecraftArguments;
	/**
	 * A list of all tweaker classes
	 */
	QStringList tweakers;
	/**
	 * The main class to load first
	 */
	QString mainClass;
	/**
	 * The applet class, for some very old minecraft releases
	 */
	QString appletClass;

	/// the list of libs - both active and inactive, native and java
	Libraries libraries;

	/// traits, collected from all the version files (version files can only add)
	QSet<QString> traits;

	/// A list of jar mods. version files can add those.
	QList<JarmodPtr> jarMods;
};
}