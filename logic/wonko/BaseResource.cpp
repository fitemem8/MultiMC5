#include "BaseResource.h"

#include "Json.h"

void StringResource::load(const QJsonValue &data)
{
	m_data = Json::ensureString(data);
}

void StringListResource::load(const QJsonValue &data)
{
	m_data = Json::ensureIsArrayOf<QString>(data);
}

void FoldersResource::load(const QJsonValue &data)
{
	using namespace Json;

	const QJsonObject obj = ensureObject(data);
	for (const QString &key : obj.keys())
	{
		m_folders[key] = ensureIsArrayOf<QString>(obj, key);
	}
}
