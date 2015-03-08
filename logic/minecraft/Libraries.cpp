#include "Libraries.h"
#include <minecraft/VersionBuildError.h>
#include <tasks/Task.h>
#include <net/NetJob.h>
#include <modutils.h>
#include "Env.h"
#include "minecraft/Mod.h"

namespace Minecraft {

int findLibraryByName(QList<LibraryPtr> haystack, const GradleSpecifier &needle)
{
	int retval = -1;
	for (int i = 0; i < haystack.size(); ++i)
	{
		if (haystack.at(i)->rawName().matchName(needle))
		{
			// only one is allowed.
			if (retval != -1)
				return -1;
			retval = i;
		}
	}
	return retval;
}

QList<LibraryPtr> Libraries::getActiveNormalLibs()
{
	QList<LibraryPtr> output;
	for (auto lib : overwriteLibs)
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

QList<LibraryPtr> Libraries::getActiveNativeLibs()
{
	QList<LibraryPtr> output;
	for (auto lib : overwriteLibs)
	{
		if (lib->isActive() && lib->isNative())
		{
			output.append(lib);
		}
	}
	return output;
}


void Libraries::apply(Libraries &other)
{
	if (other.shouldOverwriteLibs)
	{
		overwriteLibs = other.overwriteLibs;
	}
	for (auto addedLibrary : other.addLibs)
	{
		switch (addedLibrary->insertType)
		{
		case Library::Apply:
		{
			// qDebug() << "Applying lib " << lib->name;
			int index = findLibraryByName(overwriteLibs, addedLibrary->rawName());
			if (index >= 0)
			{
				auto existingLibrary = overwriteLibs[index];
				if (!addedLibrary->m_base_url.isNull())
				{
					existingLibrary->setBaseUrl(addedLibrary->m_base_url);
				}
				if (!addedLibrary->m_hint.isNull())
				{
					existingLibrary->setHint(addedLibrary->m_hint);
				}
				if (!addedLibrary->m_absolute_url.isNull())
				{
					existingLibrary->setAbsoluteUrl(addedLibrary->m_absolute_url);
				}
				if (addedLibrary->applyExcludes)
				{
					existingLibrary->extract_excludes = addedLibrary->extract_excludes;
				}
				if (addedLibrary->isNative())
				{
					existingLibrary->m_native_classifiers = addedLibrary->m_native_classifiers;
				}
				if (addedLibrary->applyRules)
				{
					existingLibrary->setRules(addedLibrary->m_rules);
				}
			}
			else
			{
				qWarning() << "Couldn't find" << addedLibrary->rawName() << "(skipping)";
			}
			break;
		}
		case Library::Append:
		case Library::Prepend:
		{
			// find the library by name.
			const int index = findLibraryByName(overwriteLibs, addedLibrary->rawName());
			// library not found? just add it.
			if (index < 0)
			{
				if (addedLibrary->insertType == Library::Append)
				{
					overwriteLibs.append(addedLibrary);
				}
				else
				{
					overwriteLibs.prepend(addedLibrary);
				}
				break;
			}

			// otherwise apply differences, if allowed
			auto existingLibrary = overwriteLibs.at(index);
			const Util::Version addedVersion = addedLibrary->version();
			const Util::Version existingVersion = existingLibrary->version();
			// if the existing version is a hard dependency we can either use it or
			// fail, but we can't change it
			if (existingLibrary->dependType == Library::Hard)
			{
				// we need a higher version, or we're hard to and the versions aren't equal
				if (addedVersion > existingVersion || (addedLibrary->dependType == Library::Hard && addedVersion != existingVersion))
				{
					throw VersionBuildError(QObject::tr(
						"Error resolving library dependencies between %1 and %2.")
												.arg(existingLibrary->rawName(),
													 addedLibrary->rawName()));
				}
				else
				{
					// the library is already existing, so we don't have to do anything
				}
			}
			else if (existingLibrary->dependType == Library::Soft)
			{
				// if we are higher it means we should update
				if (addedVersion > existingVersion)
				{
					overwriteLibs.replace(index, addedLibrary);
				}
				else
				{
					// our version is smaller than the existing version, but we require
					// it: fail
					if (addedLibrary->dependType == Library::Hard)
					{
						throw VersionBuildError(QObject::tr(
							"Error resolving library dependencies between %1 and %2.")
													.arg(existingLibrary->rawName(),
														 addedLibrary->rawName()));
					}
				}
			}
			break;
		}
		case Library::Replace:
		{
			GradleSpecifier toReplace;
			if (addedLibrary->insertData.isEmpty())
			{
				toReplace = addedLibrary->rawName();
			}
			else
			{
				toReplace = addedLibrary->insertData;
			}
			// qDebug() << "Replacing lib " << toReplace << " with " << lib->name;
			int index = findLibraryByName(overwriteLibs, toReplace);
			if (index >= 0)
			{
				overwriteLibs.replace(index, addedLibrary);
			}
			else
			{
				qWarning() << "Couldn't find" << toReplace << "(skipping)";
			}
			break;
		}
		}
	}
	for (auto lib : removeLibs)
	{
		int index = findLibraryByName(overwriteLibs, lib);
		if (index >= 0)
		{
			// qDebug() << "Removing lib " << lib;
			overwriteLibs.removeAt(index);
		}
		else
		{
			qWarning() << "Couldn't find" << lib << "(skipping)";
		}
	}
}

class JarlibUpdate : public Task
{
	Q_OBJECT
public:
	explicit JarlibUpdate(Libraries & libs, QObject *parent = 0) : Task(parent), m_libs(libs) {}
	virtual void executeTask();

private
slots:
	void jarlibStart();
	void jarlibFinished();
	void jarlibFailed();

private:
	NetJobPtr jarlibDownloadJob;
	Libraries & m_libs;
};

void JarlibUpdate::executeTask()
{
	jarlibStart();
}

void JarlibUpdate::jarlibStart()
{
	setStatus(tr("Getting the library files from Mojang..."));
	qDebug() << "downloading libraries";

	auto job = new NetJob(tr("Libraries"));
	jarlibDownloadJob.reset(job);

	auto libs = m_libs.getActiveNativeLibs();
	libs.append(m_libs.getActiveNormalLibs());

	auto metacache = ENV.metacache();
	QList<LibraryPtr> brokenLocalLibs;

	for (auto lib : libs)
	{
		if (lib->hint() == "local")
		{
			// FIXME: instance-internal storage disregarded, copypasta, invisible coupling by the way of a magical FS path
			if (!lib->filesExist(QDir::current().absoluteFilePath("libraries")))
				brokenLocalLibs.append(lib);
			continue;
		}

		QString raw_storage = lib->storagePath();
		QString raw_dl = lib->downloadUrl();

		auto f = [&](QString storage, QString dl)
		{
			auto entry = metacache->resolveEntry("libraries", storage);
			if (entry->stale)
			{
				jarlibDownloadJob->addNetAction(CacheDownload::make(dl, entry));
			}
		};
		if (raw_storage.contains("${arch}"))
		{
			QString cooked_storage = raw_storage;
			QString cooked_dl = raw_dl;
			f(cooked_storage.replace("${arch}", "32"), cooked_dl.replace("${arch}", "32"));
			cooked_storage = raw_storage;
			cooked_dl = raw_dl;
			f(cooked_storage.replace("${arch}", "64"), cooked_dl.replace("${arch}", "64"));
		}
		else
		{
			f(raw_storage, raw_dl);
		}
	}
	if (!brokenLocalLibs.empty())
	{
		jarlibDownloadJob.reset();
		QStringList failed;
		for (auto brokenLib : brokenLocalLibs)
		{
			failed.append(brokenLib->files());
		}
		QString failed_all = failed.join("\n");
		emitFailed(tr("Some libraries marked as 'local' are missing their jar "
					  "files:\n%1\n\nYou'll have to correct this problem manually. If this is "
					  "an externally tracked instance, make sure to run it at least once "
					  "outside of MultiMC.").arg(failed_all));
		return;
	}

	connect(jarlibDownloadJob.get(), SIGNAL(succeeded()), SLOT(jarlibFinished()));
	connect(jarlibDownloadJob.get(), SIGNAL(failed()), SLOT(jarlibFailed()));
	connect(jarlibDownloadJob.get(), SIGNAL(progress(qint64, qint64)),
			SIGNAL(progress(qint64, qint64)));

	jarlibDownloadJob->start();
}

void JarlibUpdate::jarlibFinished()
{
	emitSucceeded();
}

void JarlibUpdate::jarlibFailed()
{
	QStringList failed = jarlibDownloadJob->getFailedFiles();
	QString failed_all = failed.join("\n");
	emitFailed(tr("Failed to download the following files:\n%1\n\nPlease try again.").arg(failed_all));
}


Task *Libraries::updateTask()
{
	return new JarlibUpdate(*this);
}

Task *Libraries::prelaunchTask()
{
	return nullptr;
}

}

#include "Libraries.moc"
