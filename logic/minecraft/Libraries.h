#pragma once

#include "Library.h"
#include "wonko/DownloadableResource.h"

class Task;

namespace Minecraft
{

class Libraries : public DownloadableResource
{
public:
	void applyTo(const ResourcePtr &target) const override;
	void load(const QJsonValue &data) override;
	DownloadPtr createDownload() const override;
	Task *updateTask() const override;

	void clear()
	{
		shouldOverwriteLibs = false;
		overwriteLibs.clear();
		addLibs.clear();
		removeLibs.clear();
	}

	QList<LibraryPtr> getActiveLibs() const;

	/* DATA */
	bool shouldOverwriteLibs = false;
	QList<LibraryPtr> overwriteLibs;
	QList<LibraryPtr> addLibs;
	QList<QString> removeLibs;
};
}
