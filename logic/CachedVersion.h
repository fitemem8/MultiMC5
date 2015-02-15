#pragma once
#include "BaseVersion.h"

class CachedVersionList;
class CachedVersion;
typedef std::shared_ptr<CachedVersion> RemoteVersionPtr;

class CachedVersion: public BaseVersion
{
	friend class CachedVersionList;
public:
	virtual ~CachedVersion() {};
	virtual QString descriptor() override
	{
		return m_id;
	}
	virtual QString name()
	{
		return m_id;
	}
	virtual QString typeString() const
	{
		return m_type;
	}
	virtual uint64_t timestamp() const
	{
		return m_time;
	}
protected:
	QString m_id;
	QString m_type;
	uint64_t m_time;
};


