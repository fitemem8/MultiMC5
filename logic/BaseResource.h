#pragma once

#include <memory>
#include <QString>
#include <QStringList>
#include <QMap>

class QJsonValue;
class Task;
using ResourcePtr = std::shared_ptr<class BaseResource>;

class BaseResource
{
public:
	virtual ~BaseResource()
	{
	}

	virtual void load(const QJsonValue &data) = 0;
	virtual Task *updateTask() const
	{
		return nullptr;
	}
	/** Merge next into original, and return the result. The default overwrites the original.
	 * original.get() == this
	 */
	virtual ResourcePtr mergeWith(const ResourcePtr &original, const ResourcePtr &next)
	{
		return next;
	}
};

class StringResource : public BaseResource
{
public:
	void load(const QJsonValue &data) override;
	QString data() const
	{
		return m_data;
	}

private:
	QString m_data;
};
class StringListResource : public BaseResource
{
public:
	void load(const QJsonValue &data) override;
	QStringList data() const
	{
		return m_data;
	}

private:
	QStringList m_data;
};

class FoldersResource : public BaseResource
{
public:
	void load(const QJsonValue &data) override;

	QStringList folderPaths() const
	{
		return m_folders.keys();
	}
	QStringList folderPathTypes(const QString &path) const
	{
		return m_folders[path];
	}

private:
	QMap<QString, QStringList> m_folders;
};
