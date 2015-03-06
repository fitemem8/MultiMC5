#pragma once

#include "Library.h"
#include "JarMod.h"
#include "Assets.h"
#include <QSet>
#include <QList>
#include <QString>
#include <QStringList>

namespace Minecraft
{

struct Resources
{
	void clear();

	void finalize();

	/// get all java libraries that belong to the classpath
	QList<LibraryPtr> getActiveNormalLibs();

	/// get all native libraries that need to be available to the process
	QList<LibraryPtr> getActiveNativeLibs();

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
	QList<LibraryPtr> libraries;

	/// traits, collected from all the version files (version files can only add)
	QSet<QString> traits;

	/// A list of jar mods. version files can add those.
	QList<JarmodPtr> jarMods;
};
}