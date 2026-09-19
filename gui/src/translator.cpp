/*
 * chiaki-ng 简化字汉化补丁 - 运行时翻译器管理
 */
#include <translator.h>

#include <QCoreApplication>
#include <QDir>
#include <QLibraryInfo>
#include <QLocale>

AppTranslator &AppTranslator::instance()
{
	static AppTranslator translator;
	return translator;
}

QStringList AppTranslator::searchPaths()
{
	QStringList paths;

	// 1) 程序目录下的 translations/（windeployqt 也会把 Qt 自带翻译放这里）
	const QString deployed = QCoreApplication::applicationDirPath() + QStringLiteral("/translations");
	if (QDir(deployed).exists())
		paths.append(deployed);

	// 2) Qt 自带的翻译目录（多数平台为 Qt 安装目录/translations）
	const QString qt_path = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
	if (!qt_path.isEmpty() && !paths.contains(qt_path))
		paths.append(qt_path);

	// 3) 编译进可执行文件的内置资源
	paths.append(QStringLiteral(":/i18n"));

	return paths;
}

QLocale AppTranslator::resolveLocale(const QString &lang)
{
	if (lang.isEmpty())
		return QLocale::system();
	return QLocale(lang);
}

QVariantList AppTranslator::availableLanguages()
{
	QVariantList list;
	const QString system_label =
		QCoreApplication::translate("QmlSettings", "Follow System");

	// 空 code = 跟随系统
	list.append(QVariantMap{ { QStringLiteral("code"), QString() },
				 { QStringLiteral("label"),
				   QStringLiteral("%1 (%2)").arg(system_label,
								 QLocale::system().nativeLanguageName()) } });
	// 英文为源码语言，无需翻译文件
	list.append(QVariantMap{ { QStringLiteral("code"), QStringLiteral("en") },
				 { QStringLiteral("label"), QLocale(QStringLiteral("en")).nativeLanguageName() } });
	// 已随本补丁发布的翻译
	list.append(QVariantMap{ { QStringLiteral("code"), QStringLiteral("zh_CN") },
				 { QStringLiteral("label"), QLocale(QStringLiteral("zh_CN")).nativeLanguageName() } });
	return list;
}

void AppTranslator::install()
{
	const QLocale locale = resolveLocale(language_);
	const QStringList paths = searchPaths();

	// 先撤下旧的，避免叠加
	if (app_loaded_)
		QCoreApplication::removeTranslator(&app_translator_);
	for (QTranslator *t : qt_translators_)
		QCoreApplication::removeTranslator(t);
	for (QTranslator *t : qt_translators_)
		delete t;
	qt_translators_.clear();
	app_loaded_ = false;

	// Qt 自带翻译（文件对话框、标准按钮等）。
	// 注意：qt_<locale>.qm 只是记录依赖清单的“元目录”，QTranslator 直接装载会失败，
	// 因此按真正含词条的目录名逐个尝试；每个名字只取第一个命中的目录。
	static const char *const qt_catalogs[] = { "qtbase", "qtdeclarative", "qt" };
	for (const char *catalog : qt_catalogs) {
		for (const QString &dir : paths) {
			auto *t = new QTranslator();
			if (t->load(locale, QString::fromLatin1(catalog), QStringLiteral("_"), dir)) {
				qt_translators_.append(t);
				break;
			}
			delete t;
		}
	}

	// chiaki-ng 自身的翻译
	for (const QString &dir : paths) {
		if (app_translator_.load(locale, QStringLiteral("chiaki"), QStringLiteral("_"), dir)) {
			app_loaded_ = true;
			break;
		}
	}

	// 后安装者优先查找，因此先装 Qt 再装本程序
	for (QTranslator *t : qt_translators_)
		QCoreApplication::installTranslator(t);
	if (app_loaded_)
		QCoreApplication::installTranslator(&app_translator_);
}

void AppTranslator::setLanguage(const QString &lang)
{
	// 首次调用必须装载，即便 lang 与默认的空串相同
	if (initialized_ && lang == language_)
		return;
	initialized_ = true;
	language_ = lang;
	install();
	emit languageChanged();
}
