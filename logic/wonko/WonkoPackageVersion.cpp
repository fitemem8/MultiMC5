#include "WonkoPackageVersion.h"

#include "wonko/DownloadableResource.h"
#include "minecraft/Libraries.h"
#include "Json.h"

void WonkoPackageVersion::load(const QJsonObject &obj, const QString &uid)
{
	using namespace Json;

	//////////////// METADATA ////////////////////////

	m_uid = uid.isEmpty() ? ensureString(obj, "uid") : ensureString(obj, "uid", uid);
	m_id = ensureString(obj, "version");
	m_time = QDateTime::fromMSecsSinceEpoch(ensureDouble(obj, "time") * 1000);
	m_type = ensureString(obj, "type", "");

	if (obj.contains("requires"))
	{
		for (const QJsonObject &item : ensureIsArrayOf<QJsonObject>(obj, "requires"))
		{
			const QString uid = ensureString(item, "uid");
			const QString version = ensureString(obj, "version", QString());
			m_dependencies[uid] = version;
		}
	}

	//////////////// ACTUAL DATA ///////////////////////

#define FACTORY_FOR(CLAZZ) [] { return std::make_shared<CLAZZ>(); }

	QMap<QString, std::function<ResourcePtr()>> resourceFactories;
	resourceFactories["general.folders"] = FACTORY_FOR(FoldersResource);
	resourceFactories["java.libraries"] = FACTORY_FOR(Minecraft::Libraries);
	resourceFactories["java.natives"] = FACTORY_FOR(Minecraft::Libraries);
	resourceFactories["java.mainClass"] = FACTORY_FOR(StringResource);
	resourceFactories["mc.appletClass"] = FACTORY_FOR(StringResource);
	resourceFactories["mc.assets"] = FACTORY_FOR(StringResource);
	resourceFactories["mc.arguments"] = FACTORY_FOR(StringResource);
	resourceFactories["mc.tweakers"] = FACTORY_FOR(StringListResource);

	const QJsonObject resources = obj.contains("client") ? ensureObject(obj, "client")
														 : ensureObject(obj, "common", QJsonObject());
	QMap<QString, ResourcePtr> result;
	for (const QString &key : resources.keys())
	{
		if (resourceFactories.contains(key))
		{
			ResourcePtr ptr = resourceFactories[key]();
			ptr->load(ensureJsonValue(resources, key));
			result.insert(key, ptr);
		}
	}
	m_resources = result;
}
