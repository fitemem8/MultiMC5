#include "ParseUtils.h"

bool parse_timestamp (const QString & raw, QString & save_here, QDateTime & parse_here)
{
	save_here = raw;
	if (save_here.isEmpty())
	{
		return false;
	}
	parse_here = QDateTime::fromString(save_here, Qt::ISODate);
	if (!parse_here.isValid())
	{
		return false;
	}
	return true;
}
