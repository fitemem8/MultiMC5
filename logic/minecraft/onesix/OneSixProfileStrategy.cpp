#include "minecraft/onesix/OneSixProfileStrategy.h"
#include "minecraft/VersionBuildError.h"
#include "minecraft/onesix/OneSixInstance.h"
#include "minecraft/onesix/OneSixFormat.h"
#include "minecraft/wonko/WonkoFormat.h"
#include "CachedVersionList.h"
#include "Env.h"
#include <CachedVersion.h>
#include <MMCJson.h>

#include <pathutils.h>
#include <QDir>
#include <QUuid>
#include <QJsonDocument>
#include <QJsonArray>

OneSixProfileStrategy::OneSixProfileStrategy(OneSixInstance* instance)
{
	m_instance = instance;
}

void OneSixProfileStrategy::upgradeDeprecatedFiles()
{
	auto versionJsonPath = PathCombine(m_instance->instanceRoot(), "version.json");
	auto customJsonPath = PathCombine(m_instance->instanceRoot(), "custom.json");
	auto mcJson = PathCombine(m_instance->instanceRoot(), "patches" , "net.minecraft.json");

	// if custom.json exists
	if(QFile::exists(customJsonPath))
	{
		// can we create the patches folder to move it to?
		if(!ensureFilePathExists(mcJson))
		{
			throw VersionBuildError(QObject::tr("Unable to create path for %1").arg(mcJson));
		}
		// if version.json exists, remove it first
		if(QFile::exists(versionJsonPath))
		{
			if(!QFile::remove(versionJsonPath))
			{
				throw VersionBuildError(QObject::tr("Unable to remove obsolete %1").arg(versionJsonPath));
			}
		}
		// and then move the custom.json in place
		if(!QFile::rename(customJsonPath, mcJson))
		{
			throw VersionBuildError(QObject::tr("Unable to rename %1 to %2").arg(customJsonPath).arg(mcJson));
		}
	}
	// otherwise if version.json exists
	else if(QFile::exists(versionJsonPath))
	{
		// can we create the patches folder to move it to?
		if(!ensureFilePathExists(mcJson))
		{
			throw VersionBuildError(QObject::tr("Unable to create path for %1").arg(mcJson));
		}
		// and then move the custom.json in place
		if(!QFile::rename(versionJsonPath, mcJson))
		{
			throw VersionBuildError(QObject::tr("Unable to rename %1 to %2").arg(versionJsonPath).arg(mcJson));
		}
	}
}

void OneSixProfileStrategy::loadBuiltinPatch(QString uid, QString name, QString version)
{
	auto mcJson = PathCombine(m_instance->instanceRoot(), "patches" , QString("%1.json").arg(uid));
	// load up the base minecraft patch
	ProfilePatchPtr minecraftPatch;
	if(QFile::exists(mcJson))
	{
		auto file = ProfileUtils::parseJsonFile(QFileInfo(mcJson), false);
		file->fileId = uid;
		file->name = name;
		if(file->version.isEmpty())
		{
			file->version = QObject::tr("Custom");
		}
		minecraftPatch = std::dynamic_pointer_cast<ProfilePatch>(file);
	}
	else if(!version.isEmpty())
	{
		auto mc = std::dynamic_pointer_cast<CachedVersionList>(ENV.getVersionList(uid));
		auto path = mc->versionFilePath(version);
		if(!QFile::exists(path))
		{
			throw VersionIncomplete(uid);
		}
		QFile file(path);
		if (!file.open(QFile::ReadOnly))
		{
			throw JSONValidationError(QObject::tr("Unable to open the version file %1: %2.")
										.arg(file.fileName(), file.errorString()));
		}
		QJsonParseError error;
		QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
		if (error.error != QJsonParseError::NoError)
		{
			throw JSONValidationError(
				QObject::tr("Unable to process the version file %1: %2 at %3.")
					.arg(file.fileName(), error.errorString())
					.arg(error.offset));
		}
		minecraftPatch = WonkoFormat::fromJson(doc, file.fileName());
	}
	if(minecraftPatch)
		profile->appendPatch(minecraftPatch);
}

void OneSixProfileStrategy::loadDefaultBuiltinPatches()
{
	loadBuiltinPatch("net.minecraft", "Minecraft", m_instance->minecraftVersion());
	loadBuiltinPatch("org.lwjgl", "LWJGL", m_instance->lwjglVersion());
	loadBuiltinPatch("net.minecraftforge", "Forge", m_instance->forgeVersion());
	loadBuiltinPatch("com.mumfrey.liteloader", "LiteLoader", m_instance->liteloaderVersion());
}

void OneSixProfileStrategy::loadUserPatches()
{
	// load all patches, put into map for ordering, apply in the right order
	ProfileUtils::PatchOrder userOrder;
	ProfileUtils::readOverrideOrders(PathCombine(m_instance->instanceRoot(), "order.json"), userOrder);
	QDir patches(PathCombine(m_instance->instanceRoot(),"patches"));

	// first, load things by sort order.
	for (auto id : userOrder)
	{
		// ignore builtins
		if (id == "net.minecraft")
			continue;
		if (id == "org.lwjgl")
			continue;
		if (id == "net.minecraftforge")
			continue;
		if (id == "com.mumfrey.liteloader")
			continue;
		// parse the file
		QString filename = patches.absoluteFilePath(id + ".json");
		QFileInfo finfo(filename);
		if(!finfo.exists())
		{
			qDebug() << "Patch file " << filename << " was deleted by external means...";
			continue;
		}
		qDebug() << "Reading" << filename << "by user order";
		auto file = ProfileUtils::parseJsonFile(finfo, false);
		// sanity check. prevent tampering with files.
		if (file->fileId != id)
		{
			throw VersionBuildError(
				QObject::tr("load id %1 does not match internal id %2").arg(id, file->fileId));
		}
		profile->appendPatch(file);
	}
	// now load the rest by internal preference.
	QMap<int, QPair<QString, VersionFilePtr>> files;
	for (auto info : patches.entryInfoList(QStringList() << "*.json", QDir::Files))
	{
		// parse the file
		qDebug() << "Reading" << info.fileName();
		auto file = ProfileUtils::parseJsonFile(info, true);
		// ignore builtins
		if (file->fileId == "net.minecraft")
			continue;
		if (file->fileId == "org.lwjgl")
			continue;
		if (file->fileId == "net.minecraftforge")
			continue;
		if (file->fileId == "com.mumfrey.liteloader")
			continue;
		// do not load what we already loaded in the first pass
		if (userOrder.contains(file->fileId))
			continue;
		if (files.contains(file->order))
		{
			// FIXME: do not throw?
			throw VersionBuildError(QObject::tr("%1 has the same order as %2")
										.arg(file->fileId, files[file->order].second->fileId));
		}
		files.insert(file->order, qMakePair(info.fileName(), file));
	}
	for (auto order : files.keys())
	{
		auto &filePair = files[order];
		profile->appendPatch(filePair.second);
	}
}


void OneSixProfileStrategy::load()
{
	profile->clearPatches();

	upgradeDeprecatedFiles();
	loadDefaultBuiltinPatches();
	loadUserPatches();

	profile->finalize();
}

bool OneSixProfileStrategy::saveOrder(ProfileUtils::PatchOrder order)
{
	return ProfileUtils::writeOverrideOrders(PathCombine(m_instance->instanceRoot(), "order.json"), order);
}

bool OneSixProfileStrategy::removePatch(ProfilePatchPtr patch)
{
	bool ok = true;
	// first, remove the patch file. this ensures it's not used anymore
	auto fileName = patch->getPatchFilename();


	auto preRemoveJarMod = [&](JarmodPtr jarMod) -> bool
	{
		QString fullpath = PathCombine(m_instance->jarModsDir(), jarMod->name);
		QFileInfo finfo (fullpath);
		if(finfo.exists())
		{
			return QFile::remove(fullpath);
		}
		return true;
	};

	for(auto &jarmod: patch->getJarMods())
	{
		ok &= preRemoveJarMod(jarmod);
	}
	return ok;
}

bool OneSixProfileStrategy::installJarMods(QStringList filepaths)
{
	QString patchDir = PathCombine(m_instance->instanceRoot(), "patches");
	if(!ensureFolderPathExists(patchDir))
	{
		return false;
	}

	if (!ensureFolderPathExists(m_instance->jarModsDir()))
	{
		return false;
	}

	for(auto filepath:filepaths)
	{
		QFileInfo sourceInfo(filepath);
		auto uuid = QUuid::createUuid();
		QString id = uuid.toString().remove('{').remove('}');
		QString target_filename = id + ".jar";
		QString target_id = "org.multimc.jarmod." + id;
		QString target_name = sourceInfo.completeBaseName() + " (jar mod)";
		QString finalPath = PathCombine(m_instance->jarModsDir(), target_filename);

		QFileInfo targetInfo(finalPath);
		if(targetInfo.exists())
		{
			return false;
		}

		if (!QFile::copy(sourceInfo.absoluteFilePath(),QFileInfo(finalPath).absoluteFilePath()))
		{
			return false;
		}

		auto f = std::make_shared<VersionFile>();
		auto jarMod = std::make_shared<Jarmod>();
		jarMod->name = target_filename;
		f->jarMods.append(jarMod);
		f->name = target_name;
		f->fileId = target_id;
		QString patchFileName = PathCombine(patchDir, target_id + ".json");
		f->filename = patchFileName;

		QFile file(patchFileName);
		if (!file.open(QFile::WriteOnly))
		{
			qCritical() << "Error opening" << file.fileName()
						<< "for reading:" << file.errorString();
			return false;
		}
		file.write(OneSixFormat::toJson(f, true).toJson());
		file.close();
		profile->appendPatch(f);
	}
	profile->saveCurrentOrder();
	profile->reapply();
	return true;
}

