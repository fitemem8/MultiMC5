#pragma once
#include "BaseVersion.h"

class MetaPackageList;
class MetaPackage;
typedef std::shared_ptr<MetaPackage> MetaPackagePtr;

class MetaPackage: public BaseVersion
{
	friend class MetaPackageList;
public:
	virtual ~MetaPackage() {};
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


