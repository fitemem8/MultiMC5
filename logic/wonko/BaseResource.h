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

	virtual void clear() {}
	virtual void load(const QJsonValue &data) = 0;
	virtual Task *updateTask() const
	{
		return nullptr;
	}
	virtual Task *prelaunchTask() const
	{
		return nullptr;
	}

	virtual void applyTo(const ResourcePtr &target) const
	{
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
	void applyTo(const ResourcePtr &target) const override
	{
		std::dynamic_pointer_cast<StringResource>(target)->m_data = m_data;
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
	void applyTo(const ResourcePtr &target) const override
	{
		std::dynamic_pointer_cast<StringListResource>(target)->m_data = m_data;
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

	void applyTo(const ResourcePtr &target) const override
	{
		std::dynamic_pointer_cast<FoldersResource>(target)->m_folders.unite(m_folders);
	}

private:
	QMap<QString, QStringList> m_folders;
};
