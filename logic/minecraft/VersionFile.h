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
	int order = 0;
	bool isVanilla = false;
	QString name;
	QString type;
	QString fileId;
	QString version;
	QString mcVersion;
	QString filename;
	QString mainClass;
	QString appletClass;
	QString overwriteMinecraftArguments;
	QString addMinecraftArguments;
	QString removeMinecraftArguments;

	/// the time this version was actually released by Mojang, as string and as QDateTime
	QString m_releaseTimeString;
	QDateTime m_releaseTime;

	/// asset group used by this ... thing.
	QString assets;

	bool shouldOverwriteTweakers = false;
	QStringList overwriteTweakers;
	QStringList addTweakers;
	QStringList removeTweakers;

	bool shouldOverwriteLibs = false;
	QList<RawLibraryPtr> overwriteLibs;
	QList<RawLibraryPtr> addLibs;
	QList<QString> removeLibs;

	QSet<QString> traits;

	QList<JarmodPtr> jarMods;
};


