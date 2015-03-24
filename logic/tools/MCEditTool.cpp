#include "MCEditTool.h"

#include <QDir>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
// FIXME: mixing logic and UI!!!!
#include <QInputDialog>

#include "logic/settings/SettingsObject.h"
#include "logic/BaseInstance.h"
#include "logic/minecraft/MinecraftInstance.h"
#include "MultiMC.h"

MCEditTool::MCEditTool(InstancePtr instance, QObject *parent)
	: BaseDetachedTool(instance, parent)
{
}

QString MCEditTool::getSave() const
{
	// FIXME: mixing logic and UI!!!!
	auto mcInstance = std::dynamic_pointer_cast<MinecraftInstance>(m_instance);
	if(!mcInstance)
	{
		return QString();
	}
	QDir saves(mcInstance->minecraftRoot() + "/saves");
	QStringList worlds = saves.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	QMutableListIterator<QString> it(worlds);
	while (it.hasNext())
	{
		it.next();
		if (!QDir(saves.absoluteFilePath(it.value())).exists("level.dat"))
		{
			it.remove();
		}
	}
	bool ok = true;
	const QString save = QInputDialog::getItem(
		MMC->activeWindow(), tr("MCEdit"), tr("Choose which world to open:"),
		worlds, 0, false, &ok);
	if (ok)
	{
		return saves.absoluteFilePath(save);
	}
	return QString();
}

void MCEditTool::runImpl()
{
	const QString mceditPath = MMC->settings()->get("MCEditPath").toString();
	const QString save = getSave();
	if (save.isNull())
	{
		return;
	}
#ifdef Q_OS_OSX
	QProcess *process = new QProcess();
	connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), process, SLOT(deleteLater()));
	process->setProgram(mceditPath);
	process->setArguments(QStringList() << save);
	process->start();
#else
	QDir mceditDir(mceditPath);
	QString program;
	if (mceditDir.exists("mcedit.py"))
	{
		program = mceditDir.absoluteFilePath("mcedit.py");
	}
	else if (mceditDir.exists("mcedit.exe"))
	{
		program = mceditDir.absoluteFilePath("mcedit.exe");
	}
	QProcess::startDetached(program, QStringList() << save, mceditPath);
#endif
}

void MCEditFactory::registerSettings(std::shared_ptr<SettingsObject> settings)
{
	settings->registerSetting("MCEditPath");
}
BaseExternalTool *MCEditFactory::createTool(InstancePtr instance, QObject *parent)
{
	return new MCEditTool(instance, parent);
}
bool MCEditFactory::check(QString *error)
{
	return check(MMC->settings()->get("MCEditPath").toString(), error);
}
bool MCEditFactory::check(const QString &path, QString *error)
{
	if (path.isEmpty())
	{
		*error = QObject::tr("Path is empty");
		return false;
	}
	const QDir dir(path);
	if (!dir.exists())
	{
		*error = QObject::tr("Path does not exist");
		return false;
	}
	if (!dir.exists("mcedit.py") && !dir.exists("mcedit.exe") && !dir.exists("Contents"))
	{
		*error = QObject::tr("Path does not seem to be a MCEdit path");
		return false;
	}
	return true;
}
