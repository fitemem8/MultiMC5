#include "WonkoPackageVersion.h"

#include "wonko/DownloadableResource.h"
#include "minecraft/Libraries.h"
#include "Json.h"
#include "Rules.h"

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

	if (obj.contains("data"))
	{
		QJsonObject commonObj;
		QJsonObject clientObj;
		for (const QJsonObject &group : ensureIsArrayOf<QJsonObject>(obj, "data"))
		{
			Rules rules;
			rules.load(group.contains("rules") ? group.value("rules") : QJsonArray());
			if (rules.result(RuleContext{"client"}) == BaseRule::Allow)
			{
				clientObj = group;
			}
			else if (rules.result(RuleContext{""}) == BaseRule::Allow)
			{
				commonObj = group;
			}
		}

		QMap<QString, ResourcePtr> result;
		auto loadGroup = [&result, resourceFactories](const QJsonObject &resources)
		{
			qDebug() << resources;
			for (const QString &key : resources.keys())
			{
				if (resourceFactories.contains(key))
				{
					ResourcePtr ptr = resourceFactories[key]();
					ptr->load(ensureJsonValue(resources, key));
					if (result.contains(key))
					{
						ptr->applyTo(result.value(key));
					}
					else
					{
						result.insert(key, ptr);
					}
				}
			}
		};
		loadGroup(commonObj);
		loadGroup(clientObj);
		m_resources = result;
	}
}
