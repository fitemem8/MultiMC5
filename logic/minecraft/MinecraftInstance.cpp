#include "MinecraftInstance.h"
#include "MultiMC.h"
#include "logic/settings/SettingsObject.h"
#include <pathutils.h>
#include "logic/minecraft/MinecraftVersionList.h"

MinecraftInstance::MinecraftInstance(const QString &rootDir, SettingsObject *settings, QObject *parent)
	: BaseInstance(rootDir, settings, parent)
{
	auto globalSettings = MMC->settings();

	// Java Settings
	m_settings->registerSetting("OverrideJava", false);
	m_settings->registerSetting("OverrideJavaLocation", false);
	m_settings->registerSetting("OverrideJavaArgs", false);
	m_settings->registerOverride(globalSettings->getSetting("JavaPath"));
	m_settings->registerOverride(globalSettings->getSetting("JvmArgs"));

	// Custom Commands
	m_settings->registerSetting({"OverrideCommands","OverrideLaunchCmd"}, false);
	m_settings->registerOverride(globalSettings->getSetting("PreLaunchCommand"));
	m_settings->registerOverride(globalSettings->getSetting("PostExitCommand"));

	// Window Size
	m_settings->registerSetting("OverrideWindow", false);
	m_settings->registerOverride(globalSettings->getSetting("LaunchMaximized"));
	m_settings->registerOverride(globalSettings->getSetting("MinecraftWinWidth"));
	m_settings->registerOverride(globalSettings->getSetting("MinecraftWinHeight"));

	// Memory
	m_settings->registerSetting("OverrideMemory", false);
	m_settings->registerOverride(globalSettings->getSetting("MinMemAlloc"));
	m_settings->registerOverride(globalSettings->getSetting("MaxMemAlloc"));
	m_settings->registerOverride(globalSettings->getSetting("PermGen"));

	// Console
	m_settings->registerSetting("OverrideConsole", false);
	m_settings->registerOverride(globalSettings->getSetting("ShowConsole"));
	m_settings->registerOverride(globalSettings->getSetting("AutoCloseConsole"));
	m_settings->registerOverride(globalSettings->getSetting("LogPrePostOutput"));
}

QString MinecraftInstance::minecraftRoot() const
{
	QFileInfo mcDir(PathCombine(instanceRoot(), "minecraft"));
	QFileInfo dotMCDir(PathCombine(instanceRoot(), ".minecraft"));

	if (dotMCDir.exists() && !mcDir.exists())
		return dotMCDir.filePath();
	else
		return mcDir.filePath();
}

std::shared_ptr< BaseVersionList > MinecraftInstance::versionList() const
{
	return std::dynamic_pointer_cast<BaseVersionList>(MMC->minecraftlist());
}

