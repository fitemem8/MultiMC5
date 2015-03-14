#pragma once

#include <QUrl>

#include "BaseResource.h"

class QJsonObject;
using NetActionPtr = std::shared_ptr<class NetAction>;
using DownloadPtr = std::shared_ptr<class BaseDownload>;

class BaseDownload
{
public:
	virtual ~BaseDownload()
	{
	}

	virtual QUrl url() const
	{
		return m_url;
	}
	int size() const
	{
		return m_size;
	}
	QByteArray sha256() const
	{
		return m_hash;
	}

	virtual void load(const QJsonObject &data);
	virtual QList<NetActionPtr> createNetActions() const;

private:
	QUrl m_url;
	int m_size;
	QByteArray m_hash;
};

class DownloadableResource : public BaseResource
{
public:
	virtual ~DownloadableResource()
	{
	}

	virtual DownloadPtr createDownload() const
	{
		return std::make_shared<BaseDownload>();
	}
	virtual void load(const QJsonValue &data) override;
	virtual Task *updateTask() const override;
	virtual void applyTo(const ResourcePtr &target) const override;

	QList<DownloadPtr> downloads() const
	{
		return m_downloads;
	}

private:
	QList<DownloadPtr> m_downloads;
};
