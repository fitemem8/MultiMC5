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

class MetaPackageList : public BaseVersionList
{
	Q_OBJECT
public:
	enum LoadStatus
	{
		NotLoaded,
		LocalLoaded,
		RemoteLoaded
	};
private:
	void sortInternal();
	void loadListFromJSON(QJsonDocument jsonDoc, LoadStatus source);
	void finalizeUpdate(QString version);
	void reindex();
public:
	friend class CachedListLoadTask;
	friend class CachedVersionUpdateTask;

	explicit MetaPackageList(QString baseUrl, QString uid, QObject *parent = 0);

	std::shared_ptr<Task> createUpdateTask(QString version);

	virtual Task *getLoadTask();
	virtual bool isLoaded();
	virtual const BaseVersionPtr at(int i) const;
	virtual int count() const;
	virtual void sort();

	virtual BaseVersionPtr getLatestStable() const;

	QString uid()
	{
		return m_uid;
	}
	QString indexUrl()
	{
		return QString("%1/%2.json").arg(m_baseUrl).arg(m_uid);
	}
	QString fileUrl(QString id)
	{
		return QString("%1/%2/%3.json").arg(m_baseUrl).arg(m_uid).arg(id);
	}
	QString versionFilePath(QString version);
protected:
	QList<BaseVersionPtr> m_vlist;
	QMap<QString, BaseVersionPtr> m_lookup;

	LoadStatus m_loaded = NotLoaded;

	QString m_latestReleaseID = "INVALID";
	QString m_latestSnapshotID = "INVALID";
	QString m_uid;
	QString m_baseUrl;

protected
slots:
	virtual void updateListData(QList<BaseVersionPtr> versions);
};
