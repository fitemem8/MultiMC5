#pragma once
#include <minecraft/Library.h>

class Task;

namespace Minecraft
{

struct Libraries
{
	void apply(Libraries &other);
	void clear()
	{
		shouldOverwriteLibs = false;
		overwriteLibs.clear();
		addLibs.clear();
		removeLibs.clear();
	}
	void finalize(){}

	Task *updateTask();
	Task *prelaunchTask();

	/// get all java libraries that belong to the classpath
	QList<LibraryPtr> getActiveNormalLibs();

	/// get all native libraries that need to be available to the process
	QList<LibraryPtr> getActiveNativeLibs();

	/* DATA */
	bool shouldOverwriteLibs = false;
	QList<LibraryPtr> overwriteLibs;
	QList<LibraryPtr> addLibs;
	QList<QString> removeLibs;
};
}
