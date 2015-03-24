#include "MinecraftVersion.h"
#include "MinecraftProfile.h"
#include "VersionBuildError.h"
#include "ProfileUtils.h"
#include "settings/SettingsObject.h"

bool MinecraftVersion::usesLegacyLauncher()
{
	return m_traits.contains("legacyLaunch") || m_traits.contains("aplhaLaunch");
}

QString MinecraftVersion::descriptor()
{
	return m_descriptor;
}

QString MinecraftVersion::name()
{
	return m_name;
}

QString MinecraftVersion::typeString() const
{
	if(m_type == "snapshot")
	{
		return QObject::tr("Snapshot");
	}
	else if (m_type == "release")
	{
		return QObject::tr("Regular release");
	}
	else if (m_type == "old_alpha")
	{
		return QObject::tr("Alpha");
	}
	else if (m_type == "old_beta")
	{
		return QObject::tr("Beta");
	}
	else
	{
		return QString();
	}
}

bool MinecraftVersion::hasJarMods()
{
	return false;
}

bool MinecraftVersion::isMinecraftVersion()
{
	return true;
}

// 1. assume the local file is good. load, check. If it's good, apply.
// 2. if discrepancies are found, fall out and fail (impossible to apply incomplete version).
void MinecraftVersion::applyFileTo(MinecraftProfile *version)
{
	getVersionFile()->applyTo(version);
}

VersionFilePtr MinecraftVersion::getVersionFile()
{
	QFileInfo versionFile(QString("versions/%1/%1.dat").arg(m_descriptor));

	return ProfileUtils::parseBinaryJsonFile(versionFile);
}


void MinecraftVersion::applyTo(MinecraftProfile *version)
{
	// do we have this one cached?
	if (m_versionSource == Local)
	{
		applyFileTo(version);
		return;
	}
	// if not builtin, do not proceed any further.
	if (m_versionSource != Builtin)
	{
		throw VersionIncomplete(QObject::tr(
			"Minecraft version %1 could not be applied: version files are missing.").arg(m_descriptor));
	}
	if (!m_descriptor.isNull())
	{
		version->id = m_descriptor;
	}
	if (!m_mainClass.isNull())
	{
		version->mainClass = m_mainClass;
	}
	if (!m_appletClass.isNull())
	{
		version->appletClass = m_appletClass;
	}
	if (!m_processArguments.isNull())
	{
		version->vanillaProcessArguments = m_processArguments;
		version->processArguments = m_processArguments;
	}
	if (!m_type.isNull())
	{
		version->type = m_type;
	}
	if (!m_releaseTimeString.isNull())
	{
		version->m_releaseTimeString = m_releaseTimeString;
		version->m_releaseTime = m_releaseTime;
	}
	if (!m_updateTimeString.isNull())
	{
		version->m_updateTimeString = m_updateTimeString;
		version->m_updateTime = m_updateTime;
	}
	version->traits.unite(m_traits);
}

int MinecraftVersion::getOrder()
{
	return order;
}

void MinecraftVersion::setOrder(int order)
{
	this->order = order;
}

QList<JarmodPtr> MinecraftVersion::getJarMods()
{
	return QList<JarmodPtr>();
}

QString MinecraftVersion::getPatchName()
{
	return "Minecraft";
}
QString MinecraftVersion::getPatchVersion()
{
	return m_descriptor;
}
QString MinecraftVersion::getPatchID()
{
	return "net.minecraft";
}
QString MinecraftVersion::getPatchFilename()
{
	return QString();
}

bool MinecraftVersion::needsUpdate()
{
	return m_versionSource == Remote || hasUpdate();
}

bool MinecraftVersion::hasUpdate()
{
	return m_versionSource == Remote || (m_versionSource == Local && upstreamUpdate);
}

bool MinecraftVersion::isCustom()
{
	// if we add any other source types, this will evaluate to false for them.
	return m_versionSource != Builtin && m_versionSource != Local && m_versionSource != Remote;
}
