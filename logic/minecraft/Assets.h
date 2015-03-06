#pragma once

#include <QString>
#include <QDateTime>

class Task;

namespace Minecraft
{
	struct Assets
	{
	public:
		Assets(QString id = QString())
		{
			m_id = id;
		}
		void apply (Assets & other)
		{
			if(!other.m_id.isNull())
			{
				m_id = other.m_id;
			}
		}
		void clear ()
		{
			m_id.clear();
		}
		void apply (QString & value)
		{
			if(!value.isNull())
			{
				m_id = value;
			}
		}
		void finalize()
		{
			// HACK: deny april fools. my head hurts enough already.
			QDate now = QDate::currentDate();
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
		Task * updateTask();
		Task * prelaunchTask();
		QString id()
		{
			return m_id;
		}
	private:
		QString m_id;
	};
}
