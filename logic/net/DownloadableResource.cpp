#include "DownloadableResource.h"

#include "net/CacheDownload.h"
#include "net/NetJob.h"
#include "net/HttpMetaCache.h"
#include "Env.h"
#include "Json.h"

void BaseDownload::load(const QJsonObject &data)
{
	using namespace Json;

	m_url = ensureUrl(data, "url", QUrl());
	m_size = ensureInteger(data, "size");
	m_hash = ensureByteArray(data, "sha256");
}

QList<NetActionPtr> BaseDownload::createNetActions() const
{
	MetaEntryPtr ptr =
		ENV.metacache()->resolveEntry("cache", "general/" + m_hash.left(2) + '/' + m_hash);
	return QList<NetActionPtr>() << CacheDownload::make(url(), ptr);
}

void DownloadableResource::load(const QJsonValue &data)
{
	QList<DownloadPtr> downloads;
	for (const QJsonObject &obj : Json::ensureIsArrayOf<QJsonObject>(data))
	{
		DownloadPtr dl = createDownload();
		dl->load(obj);
		downloads.append(dl);
	}
	m_downloads.swap(downloads);
}

Task *DownloadableResource::updateTask() const
{
	NetJob *job = new NetJob("Download resources");
	for (const DownloadPtr dl : m_downloads)
	{
		for (NetActionPtr ptr : dl->createNetActions())
		{
			job->addNetAction(ptr);
		}
	}
	return job;
}

void DownloadableResource::applyTo(const ResourcePtr &target) const
{
	QHash<QByteArray, DownloadPtr> downloads;
	for (DownloadPtr dl : m_downloads)
	{
		downloads.insert(dl->sha256(), dl);
	}

	for (DownloadPtr dl : std::dynamic_pointer_cast<DownloadableResource>(target)->m_downloads)
	{
		downloads.insert(dl->sha256(), dl);
	}

	std::dynamic_pointer_cast<DownloadableResource>(target)->m_downloads = downloads.values();
}
