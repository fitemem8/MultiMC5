#include "Library.h"

#include "net/HttpMetaCache.h"
#include "net/CacheDownload.h"
#include "wonko/Rules.h"
#include "Env.h"
#include "Json.h"

QStringList Library::files() const
{
	QStringList retval;
	QString storage = storagePath();
	if (storage.contains("${arch}"))
	{
		QString cooked_storage = storage;
		cooked_storage.replace("${arch}", "32");
		retval.append(cooked_storage);
		cooked_storage = storage;
		cooked_storage.replace("${arch}", "64");
		retval.append(cooked_storage);
	}
	else
		retval.append(storage);
	return retval;
}

bool Library::filesExist(const QDir &base) const
{
	auto libFiles = files();
	for (auto file : libFiles)
	{
		QFileInfo info(base, file);
		qWarning() << info.absoluteFilePath() << "doesn't exist";
		if (!info.exists())
			return false;
	}
	return true;
}

QUrl Library::url() const
{
	if (!m_absolute_url.isEmpty())
	{
		return m_absolute_url;
	}

	if (m_base_url.isEmpty())
	{
		return QString("https://" + URLConstants::LIBRARY_BASE) + storagePath();
	}

	return m_base_url.resolved(storagePath());
}

bool Library::isActive() const
{
	bool result = true;
	if (!m_rules)
	{
		result = true;
	}
	else
	{
		BaseRule::RuleAction ruleResult = m_rules->result();
		result = result && (ruleResult == BaseRule::Allow);
	}
	if (isNative())
	{
		result = result && m_native_classifiers.contains(OpSys::currentSystem());
	}
	return result;
}

void Library::applyTo(const LibraryPtr &other)
{
	if (m_base_url.isValid())
	{
		other->setBaseUrl(m_base_url);
	}
	if (m_absolute_url.isValid())
	{
		other->setAbsoluteUrl(m_absolute_url);
	}
	if (!m_hint.isNull())
	{
		other->setHint(m_hint);
	}
	if (applyExcludes)
	{
		other->extract_excludes = extract_excludes;
	}
	if (isNative())
	{
		other->m_native_classifiers = m_native_classifiers;
	}
	if (applyRules)
	{
		other->setRules(m_rules);
	}
}

QList<NetActionPtr> Library::createNetActions() const
{
	if (hint() == "local")
	{
		// FIXME: instance-internal storage disregarded, copypasta, invisible coupling by the
		// way of a magical FS path
		if (!filesExist(QDir::current().absoluteFilePath("libraries")))
		{
			throw Exception("Local file doesn't exist");
		}
		return {};
	}

	const QString raw_storage = storagePath();
	const QString raw_dl = url().toString();

	QList<NetActionPtr> out;

	auto f = [](QString storage, QString dl) -> CacheDownloadPtr
	{
		auto entry = ENV.metacache()->resolveEntry("libraries", storage);
		if (entry->stale)
		{
			return CacheDownload::make(dl, entry);
		}
		else
		{
			return {};
		}
	};
	if (raw_storage.contains("${arch}"))
	{
		out << f(QString(raw_storage).replace("${arch}", "32"),
				 QString(raw_dl).replace("${arch}", "32"));
		out << f(QString(raw_storage).replace("${arch}", "64"),
				 QString(raw_dl).replace("${arch}", "64"));
	}
	else
	{
		out << f(raw_storage, raw_dl);
	}
	out.removeAll(nullptr);

	return out;
}

QString Library::storagePath() const
{
	// non-native? use only the gradle specifier
	if (!isNative())
	{
		return m_name.toPath();
	}

	// otherwise native, override classifiers. Mojang HACK!
	GradleSpecifier nativeSpec = m_name;
	if (m_native_classifiers.contains(OpSys::currentSystem()))
	{
		nativeSpec.setClassifier(m_native_classifiers[OpSys::currentSystem()]);
	}
	else
	{
		nativeSpec.setClassifier("INVALID");
	}
	return nativeSpec.toPath();
}

void Library::load(const QJsonObject &data)
{
	BaseDownload::load(data);
	m_absolute_url = BaseDownload::url();
	m_base_url = Json::ensureUrl(data, "mavenBaseUrl", QUrl());
	m_name = GradleSpecifier(Json::ensureString(data, "name"));
}
