/*
 * chiaki-ng 简化字汉化补丁 - 运行时翻译器管理
 *
 * 负责按“设置中的语言”或系统语言装载 chiaki 自身的翻译与 Qt 内置翻译，
 * 并在语言变化时通知 QQmlEngine 重新求值所有 qsTr() 绑定。
 */
#pragma once

#include <QList>
#include <QLocale>
#include <QObject>
#include <QString>
#include <QTranslator>
#include <QVariantList>

class AppTranslator : public QObject
{
	Q_OBJECT
public:
	static AppTranslator &instance();

	// lang 为空字符串表示跟随系统语言，否则为 "zh_CN" / "en" 之类的 locale 名。
	// 由 Settings::GetLanguage() 传入；切换后即刻生效。
	void setLanguage(const QString &lang);
	QString language() const { return language_; }

	// 供设置界面下拉框使用的候选语言，元素为 { "code": String, "label": String }。
	// label 取该语言的自称（如“中文”），因此无需翻译。
	static QVariantList availableLanguages();

signals:
	void languageChanged();

private:
	AppTranslator() = default;
	Q_DISABLE_COPY(AppTranslator)

	// 按 language_ 重新装载并安装翻译器
	void install();
	// 依次在 <程序目录>/translations 与内置资源 :/i18n 中查找
	static QStringList searchPaths();
	// 把 locale 规整成 QLocale（空串 -> 系统语言）
	static QLocale resolveLocale(const QString &lang);

	QTranslator app_translator_;
	// Qt 自带翻译分散在多个目录文件里（qtbase / qtdeclarative ...），
	// 每个 QTranslator 只能装一个目录，故用列表管理。
	QList<QTranslator *> qt_translators_;
	bool app_loaded_ = false;
	// language_ 默认为空串（= 跟随系统），因此不能用它自身判断“是否已初始化”，
	// 否则首次以空串调用 setLanguage() 会被当成“无变化”而跳过装载。
	bool initialized_ = false;
	QString language_;
};
