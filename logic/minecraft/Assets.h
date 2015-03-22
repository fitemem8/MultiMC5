#pragma once

#include <QString>
#include <QDateTime>

#include "wonko/BaseResource.h"

class Task;

namespace Minecraft
{
class Assets : public BaseResource
{
public:
	explicit Assets(const QString &id = QString())
	{
		m_id = id;
	}
	void applyTo(const ResourcePtr &target) const override
	{
		auto other = std::dynamic_pointer_cast<Assets>(target);
		if (!m_id.isNull())
		{
			other->m_id = m_id;
		}
	}
	void load(const QJsonValue &data) override;
	void clear() override
	{
		m_id.clear();
	}
	void apply(QString &value)
	{
		if (!value.isNull())
		{
			m_id = value;
		}
	}
	void finalize()
	{
		// HACK: deny april fools. my head hurts enough already.
		const QDate now = QDate::currentDate();
		bool isAprilFools = now.month() == 4 && now.day() == 1;
		if (m_id.endsWith("_af") && !isAprilFools)
		{
			m_id = m_id.left(m_id.length() - 3);
		}
		if (m_id.isEmpty())
		{
			m_id = "legacy";
		}
	}
	QString storageFolder();
	Task *updateTask() const override;
	Task *prelaunchTask() const override;
	QString id() const
	{
		return m_id;
	}

private:
	QString m_id;
};
}
