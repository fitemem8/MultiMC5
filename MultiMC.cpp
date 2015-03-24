#include "MultiMC.h"
#include "BuildConfig.h"

#include <iostream>
#include <QDir>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QTranslator>
#include <QLibraryInfo>
#include <QMessageBox>
#include <QStringList>
#include <QDesktopServices>

#include "gui/dialogs/VersionSelectDialog.h"
#include "logic/InstanceList.h"
#include "logic/auth/MojangAccountList.h"
#include "logic/icons/IconList.h"
#include "logic/LwjglVersionList.h"
#include "logic/minecraft/MinecraftVersionList.h"
#include "logic/liteloader/LiteLoaderVersionList.h"

#include "logic/forge/ForgeVersionList.h"

#include "logic/net/HttpMetaCache.h"
#include "logic/net/URLConstants.h"
#include "logic/Env.h"

#include "logic/java/JavaUtils.h"

#include "logic/updater/UpdateChecker.h"

#include "logic/tools/JProfiler.h"
#include "logic/tools/JVisualVM.h"
#include "logic/tools/MCEditTool.h"

#include "pathutils.h"
#include "cmdutils.h"
#include <xdgicon.h>
#include "logic/settings/INISettingsObject.h"
#include "logic/settings/Setting.h"
#include "logger/QsLog.h"
#include "logger/QsLogDest.h"

#include "logic/trans/TranslationDownloader.h"

#ifdef Q_OS_WIN32
#include <windows.h>
static const int APPDATA_BUFFER_SIZE = 1024;
#endif

using namespace Util::Commandline;

MultiMC::MultiMC(int &argc, char **argv, bool test_mode) : QApplication(argc, argv)
{
	setOrganizationName("MultiMC");
	setApplicationName("MultiMC5");

	setAttribute(Qt::AA_UseHighDpiPixmaps);
	// Don't quit on hiding the last window
	this->setQuitOnLastWindowClosed(false);

	// Commandline parsing
	QHash<QString, QVariant> args;
	{
		Parser parser(FlagStyle::GNU, ArgumentStyle::SpaceAndEquals);

		// --help
		parser.addSwitch("help");
		parser.addShortOpt("help", 'h');
		parser.addDocumentation("help", "display this help and exit.");
		// --version
		parser.addSwitch("version");
		parser.addShortOpt("version", 'V');
		parser.addDocumentation("version", "display program version and exit.");
		// --dir
		parser.addOption("dir", applicationDirPath());
		parser.addShortOpt("dir", 'd');
		parser.addDocumentation("dir", "use the supplied directory as MultiMC root instead of "
									   "the binary location (use '.' for current)");

		// parse the arguments
		try
		{
			args = parser.parse(arguments());
		}
		catch (ParsingError e)
		{
			std::cerr << "CommandLineError: " << e.what() << std::endl;
			std::cerr << "Try '%1 -h' to get help on MultiMC's command line parameters."
					  << std::endl;
			m_status = MultiMC::Failed;
			return;
		}

		// display help and exit
		if (args["help"].toBool())
		{
			std::cout << qPrintable(parser.compileHelp(arguments()[0]));
			m_status = MultiMC::Succeeded;
			return;
		}

		// display version and exit
		if (args["version"].toBool())
		{
			std::cout << "Version " << BuildConfig.VERSION_STR.toStdString() << std::endl;
			std::cout << "Git " << BuildConfig.GIT_COMMIT.toStdString() << std::endl;
			m_status = MultiMC::Succeeded;
			return;
		}
	}

	QString origcwdPath = QDir::currentPath();
	QString binPath = applicationDirPath();
	QString adjustedBy;
	// change directory
	QString dirParam = args["dir"].toString();
	if (!dirParam.isEmpty())
	{
		// the dir param. it makes multimc data path point to whatever the user specified
		// on command line
		adjustedBy += "Command line " + dirParam;
		dataPath = dirParam;
	}
	else
	{
		dataPath = applicationDirPath();
		adjustedBy += "Fallback to binary path " + dataPath;
	}

	if (!ensureFolderPathExists(dataPath) || !QDir::setCurrent(dataPath))
	{
		// BAD STUFF. WHAT DO?
		initLogger();
		QLOG_ERROR() << "Failed to set work path. Will exit. NOW.";
		m_status = MultiMC::Failed;
		return;
	}

	// in test mode, root path is the same as the binary path.
	if (test_mode)
	{
		rootPath = binPath;
	}
	else
	{
#ifdef Q_OS_LINUX
		QDir foo(PathCombine(binPath, ".."));
		rootPath = foo.absolutePath();
#elif defined(Q_OS_WIN32)
		rootPath = binPath;
#elif defined(Q_OS_MAC)
		QDir foo(PathCombine(binPath, "../.."));
		rootPath = foo.absolutePath();
#endif
	}

// static data paths... mostly just for translations
#ifdef Q_OS_LINUX
	QDir foo(PathCombine(binPath, ".."));
	staticDataPath = foo.absolutePath();
#elif defined(Q_OS_WIN32)
	staticDataPath = binPath;
#elif defined(Q_OS_MAC)
	QDir foo(PathCombine(rootPath, "Contents/Resources"));
	staticDataPath = foo.absolutePath();
#endif

	// init the logger
	initLogger();

	QLOG_INFO() << "MultiMC 5, (c) 2013-2015 MultiMC Contributors";
	QLOG_INFO() << "Version                    : " << BuildConfig.VERSION_STR;
	QLOG_INFO() << "Git commit                 : " << BuildConfig.GIT_COMMIT;
	if (adjustedBy.size())
	{
		QLOG_INFO() << "Work dir before adjustment : " << origcwdPath;
		QLOG_INFO() << "Work dir after adjustment  : " << QDir::currentPath();
		QLOG_INFO() << "Adjusted by                : " << adjustedBy;
	}
	else
	{
		QLOG_INFO() << "Work dir                   : " << QDir::currentPath();
	}
	QLOG_INFO() << "Binary path                : " << binPath;
	QLOG_INFO() << "Application root path      : " << rootPath;
	QLOG_INFO() << "Static data path           : " << staticDataPath;

	// load settings
	initGlobalSettings(test_mode);

	// load translations
	initTranslations();

	// initialize the updater
	m_updateChecker.reset(new UpdateChecker(BuildConfig.CHANLIST_URL, BuildConfig.VERSION_BUILD));

	m_translationChecker.reset(new TranslationDownloader());

	// and instances
	auto InstDirSetting = m_settings->getSetting("InstanceDir");
	// instance path: check for problems with '!' in instance path and warn the user in the log
	// and rememer that we have to show him a dialog when the gui starts (if it does so)
	QString instDir = m_settings->get("InstanceDir").toString();
	QLOG_INFO() << "Instance path              : " << instDir;
	if (checkProblemticPathJava(QDir(instDir)))
	{
		QLOG_WARN()
			<< "Your instance path contains \'!\' and this is known to cause java problems";
	}
	m_instances.reset(new InstanceList(InstDirSetting->get().toString(), this));
	QLOG_INFO() << "Loading Instances...";
	m_instances->loadList();
	connect(InstDirSetting.get(), SIGNAL(SettingChanged(const Setting &, QVariant)),
			m_instances.get(), SLOT(on_InstFolderChanged(const Setting &, QVariant)));

	// and accounts
	m_accounts.reset(new MojangAccountList(this));
	QLOG_INFO() << "Loading accounts...";
	m_accounts->setListFilePath("accounts.json", true);
	m_accounts->loadList();

	// init the http meta cache
	ENV.initHttpMetaCache(rootPath, staticDataPath);

	// create the global network manager
	ENV.m_qnam.reset(new QNetworkAccessManager(this));

	// init proxy settings
	{
		QString proxyTypeStr = settings()->get("ProxyType").toString();
		QString addr = settings()->get("ProxyAddr").toString();
		int port = settings()->get("ProxyPort").value<qint16>();
		QString user = settings()->get("ProxyUser").toString();
		QString pass = settings()->get("ProxyPass").toString();
		ENV.updateProxySettings(proxyTypeStr, addr, port, user, pass);
	}

	m_translationChecker->downloadTranslations();

	//FIXME: what to do with these?
	m_profilers.insert("jprofiler",
					   std::shared_ptr<BaseProfilerFactory>(new JProfilerFactory()));
	m_profilers.insert("jvisualvm",
					   std::shared_ptr<BaseProfilerFactory>(new JVisualVMFactory()));
	for (auto profiler : m_profilers.values())
	{
		profiler->registerSettings(m_settings);
	}

	//FIXME: what to do with these?
	m_tools.insert("mcedit", std::shared_ptr<BaseDetachedToolFactory>(new MCEditFactory()));
	for (auto tool : m_tools.values())
	{
		tool->registerSettings(m_settings);
	}

	connect(this, SIGNAL(aboutToQuit()), SLOT(onExit()));
	m_status = MultiMC::Initialized;
}

MultiMC::~MultiMC()
{
	if (m_mmc_translator)
	{
		removeTranslator(m_mmc_translator.get());
	}
	if (m_qt_translator)
	{
		removeTranslator(m_qt_translator.get());
	}
}

void MultiMC::initTranslations()
{
	QLocale locale(m_settings->get("Language").toString());
	QLocale::setDefault(locale);
	QLOG_INFO() << "Your language is" << locale.bcp47Name();
	m_qt_translator.reset(new QTranslator());
	if (m_qt_translator->load("qt_" + locale.bcp47Name(),
							  QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
	{
		QLOG_DEBUG() << "Loading Qt Language File for"
					 << locale.bcp47Name().toLocal8Bit().constData() << "...";
		if (!installTranslator(m_qt_translator.get()))
		{
			QLOG_ERROR() << "Loading Qt Language File failed.";
			m_qt_translator.reset();
		}
	}
	else
	{
		m_qt_translator.reset();
	}

	m_mmc_translator.reset(new QTranslator());
	if (m_mmc_translator->load("mmc_" + locale.bcp47Name(), staticDataPath + "/translations"))
	{
		QLOG_DEBUG() << "Loading MMC Language File for"
					 << locale.bcp47Name().toLocal8Bit().constData() << "...";
		if (!installTranslator(m_mmc_translator.get()))
		{
			QLOG_ERROR() << "Loading MMC Language File failed.";
			m_mmc_translator.reset();
		}
	}
	else
	{
		m_mmc_translator.reset();
	}
}

void moveFile(const QString &oldName, const QString &newName)
{
	QFile::remove(newName);
	QFile::copy(oldName, newName);
	QFile::remove(oldName);
}
void MultiMC::initLogger()
{
	static const QString logBase = "MultiMC-%0.log";

	moveFile(logBase.arg(3), logBase.arg(4));
	moveFile(logBase.arg(2), logBase.arg(3));
	moveFile(logBase.arg(1), logBase.arg(2));
	moveFile(logBase.arg(0), logBase.arg(1));

	// init the logging mechanism
	QsLogging::Logger &logger = QsLogging::Logger::instance();
	logger.setLoggingLevel(QsLogging::TraceLevel);
	m_fileDestination = QsLogging::DestinationFactory::MakeFileDestination(logBase.arg(0));
	m_debugDestination = QsLogging::DestinationFactory::MakeDebugOutputDestination();
	logger.addDestination(m_fileDestination.get());
	logger.addDestination(m_debugDestination.get());
	// log all the things
	logger.setLoggingLevel(QsLogging::TraceLevel);
}

void MultiMC::initGlobalSettings(bool test_mode)
{
	m_settings.reset(new INISettingsObject("multimc.cfg", this));
	// Updates
	m_settings->registerSetting("UpdateChannel", BuildConfig.VERSION_CHANNEL);
	m_settings->registerSetting("AutoUpdate", true);
	m_settings->registerSetting("IconTheme", QString("multimc"));

	// Notifications
	m_settings->registerSetting("ShownNotifications", QString());

	// Remembered state
	m_settings->registerSetting("LastUsedGroupForNewInstance", QString());

	QString defaultMonospace;
	int defaultSize = 11;
#ifdef Q_OS_WIN32
	defaultMonospace = "Courier";
	defaultSize = 10;
#elif defined(Q_OS_MAC)
	defaultMonospace = "Menlo";
#else
	defaultMonospace = "Monospace";
#endif
	if(!test_mode)
	{
		// resolve the font so the default actually matches
		QFont consoleFont;
		consoleFont.setFamily(defaultMonospace);
		consoleFont.setStyleHint(QFont::Monospace);
		consoleFont.setFixedPitch(true);
		QFontInfo consoleFontInfo(consoleFont);
		QString resolvedDefaultMonospace = consoleFontInfo.family();
		QFont resolvedFont(resolvedDefaultMonospace);
		QLOG_DEBUG() << "Detected default console font:" << resolvedDefaultMonospace
			<< ", substitutions:" << resolvedFont.substitutions().join(',');
		m_settings->registerSetting("ConsoleFont", resolvedDefaultMonospace);
	}
	else
	{
		// in test mode, we don't have UI, so we don't do any font resolving
		m_settings->registerSetting("ConsoleFont", defaultMonospace);
	}
	m_settings->registerSetting("ConsoleFontSize", defaultSize);

	// FTB
	m_settings->registerSetting("TrackFTBInstances", false);
	QString ftbDataDefault;
#ifdef Q_OS_LINUX
	QString ftbDefault = ftbDataDefault = QDir::home().absoluteFilePath(".ftblauncher");
#elif defined(Q_OS_WIN32)
	wchar_t buf[APPDATA_BUFFER_SIZE];
	wchar_t newBuf[APPDATA_BUFFER_SIZE];
	QString ftbDefault, newFtbDefault, oldFtbDefault;
	if (!GetEnvironmentVariableW(L"LOCALAPPDATA", newBuf, APPDATA_BUFFER_SIZE))
	{
		QLOG_FATAL() << "Your LOCALAPPDATA folder is missing! If you are on windows, this "
						"means your system is broken. If you aren't on windows, how the **** "
						"are you running the windows build????";
	}
	else
	{
		newFtbDefault = QDir(QString::fromWCharArray(newBuf)).absoluteFilePath("ftblauncher");
	}
	if (!GetEnvironmentVariableW(L"APPDATA", buf, APPDATA_BUFFER_SIZE))
	{
		QLOG_FATAL() << "Your APPDATA folder is missing! If you are on windows, this means "
						"your system is broken. If you aren't on windows, how the **** are you "
						"running the windows build????";
	}
	else
	{
		oldFtbDefault = QDir(QString::fromWCharArray(buf)).absoluteFilePath("ftblauncher");
	}
	if (QFile::exists(QDir(newFtbDefault).absoluteFilePath("ftblaunch.cfg")))
	{
		QLOG_INFO() << "Old FTB setup";
		ftbDefault = ftbDataDefault = oldFtbDefault;
	}
	else
	{
		QLOG_INFO() << "New FTB setup";
		ftbDefault = oldFtbDefault;
		ftbDataDefault = newFtbDefault;
	}
#elif defined(Q_OS_MAC)
	QString ftbDefault = ftbDataDefault =
		PathCombine(QDir::homePath(), "Library/Application Support/ftblauncher");
#endif
	m_settings->registerSetting("FTBLauncherDataRoot", ftbDataDefault);
	m_settings->registerSetting("FTBLauncherRoot", ftbDefault);
	QLOG_INFO() << "FTB Launcher paths:" << m_settings->get("FTBLauncherDataRoot").toString()
				<< "and" << m_settings->get("FTBLauncherRoot").toString();

	m_settings->registerSetting("FTBRoot");
	if (m_settings->get("FTBRoot").isNull())
	{
		QString ftbRoot;
		QFile f(QDir(m_settings->get("FTBLauncherRoot").toString())
					.absoluteFilePath("ftblaunch.cfg"));
		QLOG_INFO() << "Attempting to read" << f.fileName();
		if (f.open(QFile::ReadOnly))
		{
			const QString data = QString::fromLatin1(f.readAll());
			QRegularExpression exp("installPath=(.*)");
			ftbRoot = QDir::cleanPath(exp.match(data).captured(1));
#ifdef Q_OS_WIN32
			if (!ftbRoot.isEmpty())
			{
				if (ftbRoot.at(0).isLetter() && ftbRoot.size() > 1 && ftbRoot.at(1) == '/')
				{
					ftbRoot.remove(1, 1);
				}
			}
#endif
			if (ftbRoot.isEmpty())
			{
				QLOG_INFO() << "Failed to get FTB root path";
			}
			else
			{
				QLOG_INFO() << "FTB is installed at" << ftbRoot;
				m_settings->set("FTBRoot", ftbRoot);
			}
		}
		else
		{
			QLOG_WARN() << "Couldn't open" << f.fileName() << ":" << f.errorString();
			QLOG_WARN() << "This is perfectly normal if you don't have FTB installed";
		}
	}

	// Folders
	m_settings->registerSetting("InstanceDir", "instances");
	m_settings->registerSetting({"CentralModsDir", "ModsDir"}, "mods");
	m_settings->registerSetting({"LWJGLDir", "LwjglDir"}, "lwjgl");
	m_settings->registerSetting("IconsDir", "icons");

	// Editors
	m_settings->registerSetting("JsonEditor", QString());

	// Language
	m_settings->registerSetting("Language", QLocale(QLocale::system().language()).bcp47Name());

	// Console
	m_settings->registerSetting("ShowConsole", true);
	m_settings->registerSetting("RaiseConsole", true);
	m_settings->registerSetting("AutoCloseConsole", true);
	m_settings->registerSetting("LogPrePostOutput", true);

	// Console Colors
	//	m_settings->registerSetting("SysMessageColor", QColor(Qt::blue));
	//	m_settings->registerSetting("StdOutColor", QColor(Qt::black));
	//	m_settings->registerSetting("StdErrColor", QColor(Qt::red));

	// Window Size
	m_settings->registerSetting({"LaunchMaximized", "MCWindowMaximize"}, false);
	m_settings->registerSetting({"MinecraftWinWidth", "MCWindowWidth"}, 854);
	m_settings->registerSetting({"MinecraftWinHeight", "MCWindowHeight"}, 480);

	// Proxy Settings
	m_settings->registerSetting("ProxyType", "None");
	m_settings->registerSetting({"ProxyAddr", "ProxyHostName"}, "127.0.0.1");
	m_settings->registerSetting("ProxyPort", 8080);
	m_settings->registerSetting({"ProxyUser", "ProxyUsername"}, "");
	m_settings->registerSetting({"ProxyPass", "ProxyPassword"}, "");

	// Memory
	m_settings->registerSetting({"MinMemAlloc", "MinMemoryAlloc"}, 512);
	m_settings->registerSetting({"MaxMemAlloc", "MaxMemoryAlloc"}, 1024);
	m_settings->registerSetting("PermGen", 128);

	// Java Settings
	m_settings->registerSetting("JavaPath", "");
	m_settings->registerSetting("LastHostname", "");
	m_settings->registerSetting("JavaDetectionHack", "");
	m_settings->registerSetting("JvmArgs", "");

	// Custom Commands
	m_settings->registerSetting({"PreLaunchCommand", "PreLaunchCmd"}, "");
	m_settings->registerSetting({"PostExitCommand", "PostExitCmd"}, "");

	// The cat
	m_settings->registerSetting("TheCat", false);

	m_settings->registerSetting("InstSortMode", "Name");
	m_settings->registerSetting("SelectedInstance", QString());

	// Window state and geometry
	m_settings->registerSetting("MainWindowState", "");
	m_settings->registerSetting("MainWindowGeometry", "");

	m_settings->registerSetting("ConsoleWindowState", "");
	m_settings->registerSetting("ConsoleWindowGeometry", "");

	m_settings->registerSetting("SettingsGeometry", "");

	m_settings->registerSetting("PagedGeometry", "");
}

std::shared_ptr<IconList> MultiMC::icons()
{
	if (!m_icons)
	{

		auto setting = MMC->settings()->getSetting("IconsDir");
		m_icons.reset(new IconList(setting->get().toString()));
		connect(setting.get(), &Setting::SettingChanged,[&](const Setting &, QVariant value)
		{
			m_icons->directoryChanged(value.toString());
		});
	}
	return m_icons;
}

std::shared_ptr<LWJGLVersionList> MultiMC::lwjgllist()
{
	if (!m_lwjgllist)
	{
		m_lwjgllist.reset(new LWJGLVersionList());
	}
	return m_lwjgllist;
}

std::shared_ptr<ForgeVersionList> MultiMC::forgelist()
{
	if (!m_forgelist)
	{
		m_forgelist.reset(new ForgeVersionList());
	}
	return m_forgelist;
}

std::shared_ptr<LiteLoaderVersionList> MultiMC::liteloaderlist()
{
	if (!m_liteloaderlist)
	{
		m_liteloaderlist.reset(new LiteLoaderVersionList());
	}
	return m_liteloaderlist;
}

std::shared_ptr<MinecraftVersionList> MultiMC::minecraftlist()
{
	if (!m_minecraftlist)
	{
		m_minecraftlist.reset(new MinecraftVersionList());
	}
	return m_minecraftlist;
}

std::shared_ptr<JavaVersionList> MultiMC::javalist()
{
	if (!m_javalist)
	{
		m_javalist.reset(new JavaVersionList());
	}
	return m_javalist;
}

void MultiMC::installUpdates(const QString updateFilesDir, UpdateFlags flags)
{
	// if we are going to update on exit, save the params now
	if (flags & OnExit)
	{
		m_updateOnExitPath = updateFilesDir;
		m_updateOnExitFlags = flags & ~OnExit;
		return;
	}
	// otherwise if there already were some params for on exit update, clear them and continue
	else if (m_updateOnExitPath.size())
	{
		m_updateOnExitFlags = None;
		m_updateOnExitPath.clear();
	}
	QLOG_INFO() << "Installing updates.";
#ifdef WINDOWS
	QString finishCmd = applicationFilePath();
	QString updaterBinary = PathCombine(applicationDirPath(), "updater.exe");
#elif LINUX
	QString finishCmd = PathCombine(root(), "MultiMC");
	QString updaterBinary = PathCombine(applicationDirPath(), "updater");
#elif OSX
	QString finishCmd = applicationFilePath();
	QString updaterBinary = PathCombine(applicationDirPath(), "updater");
#else
#error Unsupported operating system.
#endif

	QStringList args;
	// ./updater --install-dir $INSTALL_DIR --package-dir $UPDATEFILES_DIR --script
	// $UPDATEFILES_DIR/file_list.xml --wait $PID --mode main
	args << "--install-dir" << root();
	args << "--package-dir" << updateFilesDir;
	args << "--script" << PathCombine(updateFilesDir, "file_list.xml");
	args << "--wait" << QString::number(applicationPid());
	if (flags & DryRun)
		args << "--dry-run";
	if (flags & RestartOnFinish)
	{
		args << "--finish-cmd" << finishCmd;
		args << "--finish-dir" << dataPath;
	}
	QLOG_INFO() << "Running updater with command" << updaterBinary << args.join(" ");
	QFile::setPermissions(updaterBinary, (QFileDevice::Permission)0x7755);

	if (!QProcess::startDetached(updaterBinary, args /*, root()*/))
	{
		QLOG_ERROR() << "Failed to start the updater process!";
		return;
	}

	ENV.destroy();
	// Now that we've started the updater, quit MultiMC.
	quit();
}

void MultiMC::setIconTheme(const QString& name)
{
	XdgIcon::setThemeName(name);
}

QIcon MultiMC::getThemedIcon(const QString& name)
{
	return XdgIcon::fromTheme(name);
}

void MultiMC::onExit()
{
	if (m_updateOnExitPath.size())
	{
		installUpdates(m_updateOnExitPath, m_updateOnExitFlags);
	}
	ENV.destroy();
}

bool MultiMC::openJsonEditor(const QString &filename)
{
	const QString file = QDir::current().absoluteFilePath(filename);
	if (m_settings->get("JsonEditor").toString().isEmpty())
	{
		return QDesktopServices::openUrl(QUrl::fromLocalFile(file));
	}
	else
	{
		return QProcess::startDetached(m_settings->get("JsonEditor").toString(), QStringList()
																					 << file);
	}
}

#include "MultiMC.moc"
