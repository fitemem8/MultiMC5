/* Copyright 2013-2015 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <QObject>
#include <QList>
#include <QSet>

#include "BaseVersionList.h"
#include "tasks/Task.h"
#include "net/NetJob.h"

class FileStore;
class CachedListLoadTask;
class CachedVersionUpdateTask;

struct WonkoPackageMetadata
{
	QString uid;

	std::shared_ptr<class WonkoVersionList> versions()
	{
		return versionList.lock();
	}
	std::weak_ptr<class WonkoVersionList> versionList;
};
using WonkoPackageMetadataPtr = std::shared_ptr<WonkoPackageMetadata>;

/** The versions list also represents a package (since a package is basically a list of
 * versions, with some extra metadata attached)
 */
class WonkoVersionList : public BaseVersionList,
						 public std::enable_shared_from_this<WonkoVersionList>
{
	Q_OBJECT
public:
	enum LoadStatus
	{
		NotLoaded,   //< The list has not been loaded yet
		LocalLoaded, //< The list has been loaded using local (cached) data
		RemoteLoaded //< The list has been loaded from a remote source
	};

private:
	friend class CachedListLoadTask;
	void loadListFromJSON(const QJsonDocument &jsonDoc, const LoadStatus source);
	void sortInternal(); //< Helper
	void reindex();		 //< Rebuilds the m_lookup table

public:
	explicit WonkoVersionList(const QString &baseUrl, const QString &uid,
							  QObject *parent = nullptr);

	void loadFromIndex(const QJsonObject &obj, const LoadStatus source);

	std::shared_ptr<Task> createUpdateTask(const QString &version);

	Task *getLoadTask() override;
	bool isLoaded() override;
	const BaseVersionPtr at(int i) const override;
	int count() const override;
	void sort() override;

	BaseVersionPtr getLatestStable() const override;

	WonkoPackageMetadataPtr package() const
	{
		return m_package;
	}
	QString uid() const
	{
		return m_uid;
	}
	QString indexUrl() const
	{
		return QString("%1/%2.json").arg(m_baseUrl).arg(m_uid);
	}
	QString fileUrl(QString id) const
	{
		return QString("%1/%2/%3.json").arg(m_baseUrl).arg(m_uid).arg(id);
	}
	QString versionFilePath(QString version) const;

private slots:
	void initialLoad();

protected:
	QList<BaseVersionPtr> m_vlist;
	QMap<QString, BaseVersionPtr> m_lookup;

	LoadStatus m_loaded = NotLoaded;
	bool m_isFull = false;

	WonkoPackageMetadataPtr m_package;
	QString m_latestReleaseID = "INVALID";
	QString m_latestSnapshotID = "INVALID";
	QString m_baseUrl;
	QString m_uid;

protected slots:
	virtual void updateListData(QList<BaseVersionPtr> versions);
};
