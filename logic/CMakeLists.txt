project(MultiMC-Logic)

SET(LOGIC_SOURCES
	# LOGIC - Base classes and infrastructure
	BaseVersionList.h
	BaseVersionList.cpp
	InstanceList.h
	InstanceList.cpp
	BaseVersion.h
	BaseProcess.h
	BaseProcess.cpp
	BaseInstance.h
	BaseInstance.cpp
	FileStore.h
	FileStore.cpp
	NullInstance.h
	MMCError.h
	CachedVersionList.cpp
	CachedVersionList.h
	ParseUtils.cpp
	ParseUtils.h

	# WARNING: globals live here
	Env.h
	Env.cpp

	# JSON parsing helpers
	MMCJson.h
	MMCJson.cpp

	# RW lock protected map
	RWStorage.h

	# A variable that has an implicit default value and keeps track of changes
	DefaultVariable.h

	# a smart pointer wrapper intended for safer use with Qt signal/slot mechanisms
	QObjectPtr.h

	# network stuffs
	net/NetAction.h
	net/MD5EtagDownload.h
	net/MD5EtagDownload.cpp
	net/ByteArrayDownload.h
	net/ByteArrayDownload.cpp
	net/CacheDownload.h
	net/CacheDownload.cpp
	net/NetJob.h
	net/NetJob.cpp
	net/HttpMetaCache.h
	net/HttpMetaCache.cpp
	net/PasteUpload.h
	net/PasteUpload.cpp
	net/URLConstants.h
	net/URLConstants.cpp

	# Yggdrasil login stuff
	auth/AuthSession.h
	auth/AuthSession.cpp
	auth/MojangAccountList.h
	auth/MojangAccountList.cpp
	auth/MojangAccount.h
	auth/MojangAccount.cpp
	auth/YggdrasilTask.h
	auth/YggdrasilTask.cpp
	auth/flows/AuthenticateTask.h
	auth/flows/AuthenticateTask.cpp
	auth/flows/RefreshTask.cpp
	auth/flows/RefreshTask.cpp
	auth/flows/ValidateTask.h
	auth/flows/ValidateTask.cpp

	# Update system
	updater/GoUpdate.h
	updater/GoUpdate.cpp
	updater/UpdateChecker.h
	updater/UpdateChecker.cpp
	updater/DownloadTask.h
	updater/DownloadTask.cpp

	# Notifications - short warning messages
	notifications/NotificationChecker.h
	notifications/NotificationChecker.cpp

	# News System
	news/NewsChecker.h
	news/NewsChecker.cpp
	news/NewsEntry.h
	news/NewsEntry.cpp

	# Status system
	status/StatusChecker.h
	status/StatusChecker.cpp

	# Minecraft - OneSix support
	minecraft/onesix/OneSixFormat.h
	minecraft/onesix/OneSixRead.cpp
	minecraft/onesix/OneSixWrite.cpp
	minecraft/onesix/OneSixUtils.cpp
	minecraft/onesix/OneSixProfileStrategy.cpp
	minecraft/onesix/OneSixProfileStrategy.h
	minecraft/onesix/OneSixUpdate.h
	minecraft/onesix/OneSixUpdate.cpp
	minecraft/onesix/OneSixInstance.h
	minecraft/onesix/OneSixInstance.cpp

	# Minecraft - Wonko
	minecraft/wonko/WonkoFormat.h
	minecraft/wonko/WonkoRead.cpp

	minecraft/SkinUtils.h
	minecraft/SkinUtils.cpp
	minecraft/JarUtils.h
	minecraft/JarUtils.cpp
	minecraft/GradleSpecifier.h
	minecraft/MinecraftProfile.cpp
	minecraft/MinecraftProfile.h
	minecraft/JarMod.cpp
	minecraft/JarMod.h
	minecraft/MinecraftInstance.cpp
	minecraft/MinecraftInstance.h
	minecraft/MinecraftProcess.cpp
	minecraft/MinecraftProcess.h
	minecraft/MinecraftResources.cpp
	minecraft/MinecraftResources.h
	minecraft/NullProfileStrategy.h
	minecraft/OneSixRule.h
	minecraft/OpSys.cpp
	minecraft/OpSys.h
	minecraft/ProfileUtils.cpp
	minecraft/ProfileUtils.h
	minecraft/ProfileStrategy.h
	minecraft/RawLibrary.cpp
	minecraft/RawLibrary.h
	minecraft/VersionBuildError.h
	minecraft/VersionFile.h
	minecraft/VersionFilterData.h
	minecraft/VersionFilterData.cpp
	minecraft/Mod.h
	minecraft/Mod.cpp
	minecraft/ModList.h
	minecraft/ModList.cpp
	minecraft/AssetsUtils.h
	minecraft/AssetsUtils.cpp

	# A Recursive file system watcher
	RecursiveFileSystemWatcher.h
	RecursiveFileSystemWatcher.cpp

	# the screenshots feature
	screenshots/Screenshot.h
	screenshots/ImgurUpload.h
	screenshots/ImgurUpload.cpp
	screenshots/ImgurAlbumCreation.h
	screenshots/ImgurAlbumCreation.cpp

	# Icons
	icons/MMCIcon.h
	icons/MMCIcon.cpp
	icons/IconList.h
	icons/IconList.cpp

	# Tasks
	tasks/ProgressProvider.h
	tasks/Task.h
	tasks/Task.cpp
	tasks/ThreadTask.h
	tasks/ThreadTask.cpp
	tasks/SequentialTask.h
	tasks/SequentialTask.cpp

	# Settings
	settings/INIFile.cpp
	settings/INIFile.h
	settings/INISettingsObject.cpp
	settings/INISettingsObject.h
	settings/OverrideSetting.cpp
	settings/OverrideSetting.h
	settings/Setting.cpp
	settings/Setting.h
	settings/SettingsObject.cpp
	settings/SettingsObject.h

	# Java related code
	java/JavaChecker.h
	java/JavaChecker.cpp
	java/JavaUtils.h
	java/JavaUtils.cpp
	java/JavaVersionList.h
	java/JavaVersionList.cpp
	java/JavaCheckerJob.h
	java/JavaCheckerJob.cpp

	# Translations
	trans/TranslationDownloader.h
	trans/TranslationDownloader.cpp
)
################################ COMPILE ################################

# Add common library
add_library(MultiMC_logic STATIC ${LOGIC_SOURCES})

# Link
target_link_libraries(MultiMC_logic iconfix libUtil LogicalGui ${QUAZIP_LIBRARIES}
	Qt5::Core Qt5::Xml Qt5::Widgets Qt5::Network Qt5::Concurrent Qt5::WebKitWidgets
		${MultiMC_LINK_ADDITIONAL_LIBS})

add_dependencies(MultiMC_logic QuaZIP)
