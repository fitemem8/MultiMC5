#pragma once
#include <QString>
#include <QDateTime>

/**
 * parse the S3 timestamp in 'raw' and fill the forwarded variables.
 * return true/false for success/failure
 */
bool parse_timestamp (const QString &raw, QString &save_here, QDateTime &parse_here);
