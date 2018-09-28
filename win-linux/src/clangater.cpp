#include "clangater.h"
#include "defines.h"
#include "defines_p.h"

#include <QApplication>
#include <QFile>
#include <QSettings>
#include <QRegularExpression>
#include <QTranslator>

#include <list>

#include <QDebug>

extern QStringList g_cmdArgs;

class CLangater::CLangaterIntf
{
public:
    QTranslator * createTranslator()
    {
        m_list.push_back(new QTranslator);
        return m_list.back();
    }

    ~CLangaterIntf()
    {
        if ( !m_list.empty() ) {
            for (auto p: m_list) {
                QTranslator * t = p;
                if ( !t->parent() )
                    delete t;
            }
        }
    }

    QString langName(const QString& code) {
        return m_langs.value(code);
    }

private:
    std::list<QTranslator *> m_list;

    QMap<QString, QString> m_langs{
        {"en", "English"},
        {"ru", "Русский"},
        {"de", "Deutsch"},
        {"fr", "French"},
        {"es", "Spanish"},
        {"sk", "Slovak"},
        {"it_IT", "Italian"},
        {"pt_BR", "Brazil"}
    };
};

CLangater::CLangater()
    : m_intf(new CLangater::CLangaterIntf)
{
}

CLangater::~CLangater()
{
    delete m_intf, m_intf = nullptr;
}

CLangater * CLangater::getInstance()
{
    static CLangater _instance;
    return &_instance;
}

void CLangater::init()
{
    GET_REGISTRY_USER(reg_user)
    GET_REGISTRY_SYSTEM(reg_system)

    QString _lang,
            _cmd_args = g_cmdArgs.join(',');

    QRegularExpression _re(reCmdLang);
    QRegularExpressionMatch _re_match = _re.match(_cmd_args);
    if ( _re_match.hasMatch() ) {
        _lang = _re_match.captured(2);

        if ( !_re_match.captured(1).isEmpty() && !_lang.isEmpty() ) {
            reg_user.setValue("locale", _lang);
        }
    }

    if ( _lang.isEmpty() )
        _lang = reg_user.value("locale").value<QString>();

#ifdef __linux
//    if ( _lang.isEmpty() ) {
//        _lang = QLocale::system().name();
//    }

    if ( APP_DEFAULT_SYSTEM_LOCALE && _lang.isEmpty() ) {
        QString _env_name = qgetenv("LANG");
        _re.setPattern("^(\\w{2,5})\\.?");
        _re_match = _re.match(_env_name);

        if ( _re_match.hasMatch() ) {
            _lang = _re_match.captured(1);
        }
    }
#else
    // read setup language and set application locale
    _lang.isEmpty() &&
        !((_lang = reg_system.value("locale").value<QString>()).size()) && (_lang = "en").size();
#endif

    QString _lang_path = ":/i18n/langs/";
    if ( !QFile(_lang_path + _lang + ".qm").exists() ) {
        if ( QFile("./langs/" + _lang + ".qm").exists() ) {
            _lang_path = "./langs";
        } else
        if ( QFile(_lang_path + _lang.left(2) + ".qm").exists() ) {
            _lang = _lang.left(2);
        } else
        if ( QFile("./langs/" + _lang.left(2) + ".qm").exists() ) {
            _lang = _lang.left(2);
            _lang_path = "./langs";
        } else
        // check if lang file has an alias
        if ( QFile(":/i18n/" + _lang + ".qm").exists() ) {
            _lang_path = ":/i18n/";
        } else
            _lang = APP_DEFAULT_LOCALE;
    }

    QTranslator * tr = getInstance()->m_intf->createTranslator();
    if ( tr->load(_lang, _lang_path) ) {
        getInstance()->m_lang = _lang;
    }

    QCoreApplication::installTranslator(tr);
}

QString CLangater::getCurrentLangCode()
{
    return getInstance()->m_lang;
}

QString CLangater::getLangName(const QString& code)
{
    if ( code.isEmpty() ) {
        return getInstance()->m_lang;
    }

    return getInstance()->m_intf->langName(code);
}

QJsonObject CLangater::availableLangsToJson()
{
    QJsonObject _out_obj;

    QMap<QString, QString>::const_iterator i = getInstance()->m_intf->m_langs.constBegin();
    while ( i != getInstance()->m_intf->m_langs.constEnd() ) {
        _out_obj.insert(i.key(), i.value());
        ++i;
    }

    return _out_obj;
}

void CLangater::addTranslation(const QString& dir, const QString& lang)
{
    QTranslator * tr = getInstance()->m_intf->createTranslator();
    if ( tr->load(lang, dir) ) {
        QCoreApplication::installTranslator(tr);
    }
}

void CLangater::addTranslation(const QString& dir)
{
    addTranslation(dir, getInstance()->m_lang);
}
