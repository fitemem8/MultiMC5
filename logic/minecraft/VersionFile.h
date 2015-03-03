#pragma once

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <memory>
#include "minecraft/OpSys.h"
#include "minecraft/OneSixRule.h"
#include "minecraft/MinecraftResources.h"
#include "MMCError.h"
#include "RawLibrary.h"
#include "JarMod.h"

class MinecraftProfile;
class VersionFile;

typedef std::shared_ptr<VersionFile> VersionFilePtr;
class VersionFile
{
public: /* methods */
	virtual void applyTo(MinecraftProfile *version)
	{
		resources.applyTo(version);
	}
	virtual int getOrder()
	{
		return order;
	}
	virtual void setOrder(int order)
	{
		this->order = order;
	}
	virtual QString getPatchID()
	{
		return fileId;
	}
	virtual QString getPatchName()
	{
		return name;
	}
	virtual QString getPatchVersion()
	{
		return version;
	}
	virtual QString getPatchFilename()
	{
		return filename;
	}
	void setPatchFilename(QString _filename)
	{
		filename = _filename;
	}
	virtual bool isMoveable()
	{
		return true;
	}

public: /* data */
	// display-only fluff
	QString name;

private:
	/*
	 * WTF do we do with this?
	 *    some sort of hidden mixin?
	 *       OneSix-specific subclass that adds this?
	 */
	int order = 0;
	/*
	 * FIXME: replace with a generic 'source'.
	 * Sources should do reference counting and self-destruct when they reach zero.
	 */
	QString filename;

public:
	// patch metadata
	QString type;
	QString fileId;
	QString version;
	QMap<QString, QString> dependencies;
	QString m_releaseTimeString;
	QDateTime m_releaseTime;

	MinecraftResources resources;
};


