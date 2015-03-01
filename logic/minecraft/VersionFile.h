#pragma once

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <memory>
#include "minecraft/OpSys.h"
#include "minecraft/OneSixRule.h"
#include "ProfilePatch.h"
#include "MMCError.h"
#include "RawLibrary.h"
#include "JarMod.h"

class MinecraftProfile;
class VersionFile;

typedef std::shared_ptr<VersionFile> VersionFilePtr;
class VersionFile : public ProfilePatch
{
public: /* methods */
	virtual void applyTo(MinecraftProfile *version) override;
	virtual bool hasJarMods() override;
	virtual int getOrder() override
	{
		return order;
	}
	virtual void setOrder(int order) override
	{
		this->order = order;
	}
	virtual QList<JarmodPtr> getJarMods() override
	{
		return jarMods;
	}
	virtual QString getPatchID() override
	{
		return fileId;
	}
	virtual QString getPatchName() override
	{
		return name;
	}
	virtual QString getPatchVersion() override
	{
		return version;
	}
	virtual QString getPatchFilename() override
	{
		return filename;
	}

public: /* data */
	// display-only fluff
	QString name;

	// clutter
	int order = 0;
	QString filename;

	// patch metadata
	QString type;
	QString fileId;
	QString version;
	QMap<QString, QString> dependencies;
	QString m_releaseTimeString;
	QDateTime m_releaseTime;

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


