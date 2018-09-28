
#include "cascapplicationmanagerwrapper.h"
#include "cascapplicationmanagerwrapper_private.h"

#include <QMutexLocker>
#include <QTimer>
#include <QDir>
#include <QDateTime>
#include <QDesktopWidget>
#include <QJsonDocument>
#include <QJsonObject>

#include "cstyletweaks.h"
#include "defines.h"
#include "cfiledialog.h"
#include "utils.h"
#include "common/Types.h"
#include "ctabundockevent.h"
#include "clangater.h"

#ifdef _WIN32
#include "csplash.h"
#endif


#define APP_CAST(app) \
    CAscApplicationManagerWrapper & app = dynamic_cast<CAscApplicationManagerWrapper &>(getInstance());

using namespace NSEditorApi;

CAscApplicationManagerWrapper::CAscApplicationManagerWrapper(CAscApplicationManagerWrapper const&)
{

}

CAscApplicationManagerWrapper::CAscApplicationManagerWrapper()
    : CAscApplicationManager()
    , CCefEventsTransformer(nullptr)
    , QObject(nullptr)
    , m_private(new CAscApplicationManagerWrapper::CAscApplicationManagerWrapper_Private(this))
{
    CAscApplicationManager::SetEventListener(this);

    QObject::connect(this, &CAscApplicationManagerWrapper::coreEvent,
                        this, &CAscApplicationManagerWrapper::onCoreEvent);
}

CAscApplicationManagerWrapper::~CAscApplicationManagerWrapper()
{
    CSingleWindow * _sw = nullptr;
    for (auto const& w : m_vecEditors) {
        _sw = reinterpret_cast<CSingleWindow *>(w);

        if ( _sw ) {
#ifdef _WIN32
            delete _sw, _sw = NULL;
#else
            _sw->deleteLater();
#endif
        }
    }


    CMainWindow * _window = nullptr;
    for (auto const& w : m_vecWidows) {
        _window = reinterpret_cast<CMainWindow *>(w);

        if ( _window ) {
#ifdef _WIN32
            delete _window, _window = NULL;
#else
            _window->deleteLater();
#endif
        }
    }

    m_vecWidows.clear();
    m_vecEditors.clear();
}

int CAscApplicationManagerWrapper::GetPlatformKeyboardLayout()
{
    if (this->IsPlatformKeyboardSupport())
        return CAscApplicationManager::GetPlatformKeyboardLayout();

    return -1;
}

void CAscApplicationManagerWrapper::StartSaveDialog(const std::wstring& sName, unsigned int nId)
{
    CMainWindow * _window = mainWindowFromViewId(nId);
    if ( _window ) {
        QMetaObject::invokeMethod(_window->mainPanel(), "onDialogSave", Qt::QueuedConnection, Q_ARG(std::wstring, sName), Q_ARG(uint, nId));
    }
}

void CAscApplicationManagerWrapper::OnNeedCheckKeyboard()
{
    if ( !m_vecWidows.empty() ) {
        CMainWindow * _window = reinterpret_cast<CMainWindow *>(m_vecWidows.front());
        QMetaObject::invokeMethod(_window->mainPanel(), "onNeedCheckKeyboard", Qt::QueuedConnection);
    }
}

void CAscApplicationManagerWrapper::OnEvent(CAscCefMenuEvent * event)
{
    if ( event->m_nType == ASC_MENU_EVENT_TYPE_CEF_EXECUTE_COMMAND ) {
        CAscExecCommand * pData = reinterpret_cast<CAscExecCommand *>(event->m_pData);

        if ( pData->get_Command().compare(L"portal:open") == 0 ) {
            event->m_nType = ASC_MENU_EVENT_TYPE_CEF_PORTAL_OPEN;
        }
    }

    QMetaObject::invokeMethod(this, "onCoreEvent", Qt::QueuedConnection, Q_ARG(void *, event));
}

void CAscApplicationManagerWrapper::onCoreEvent(void * e)
{
    QMutexLocker locker( &m_oMutex );

    CAscCefMenuEvent * _event = static_cast<CAscCefMenuEvent *>(e);

    if ( m_private->processEvent(_event) ) return;

    CMainWindow * _window, * _target = nullptr, * _dest = nullptr;
    int _uid = _event->get_SenderId();

#if 0
    if ( _event->m_nType == ASC_MENU_EVENT_TYPE_CEF_LOCALFILE_OPEN ) {
        CAscLocalFileOpen * pData = (CAscLocalFileOpen*)_event->m_pData;
        QString inpath = QString().fromStdWString(pData->get_Directory());

        _window = mainWindowFromViewId(_event->get_SenderId());
#ifdef _WIN32
        CFileDialogWrapper dlg(_window->hWnd);
#else
        CFileDialogWrapper dlg(qobject_cast<QWidget *>(parent()));
#endif

        QString _path = !inpath.isEmpty() && QDir(inpath).exists() ?
                            inpath : Utils::lastPath(LOCAL_PATH_OPEN);

        if (!(_path = dlg.modalOpen(_path)).isEmpty()) {
            Utils::keepLastPath(LOCAL_PATH_OPEN, QFileInfo(_path).absolutePath());

            _event->Release();
            _event = new CAscCefMenuEvent(ASC_MENU_EVENT_TYPE_CEF_LOCALFILES_OPEN);
            _event->put_SenderId( _uid );

            CAscLocalOpenFileRecent_Recover * pData = new CAscLocalOpenFileRecent_Recover;
            pData->put_Path( _path.toStdWString() );

            _event->m_pData = pData;
        }
    }
#endif

    switch ( _event->m_nType ) {
    case ASC_MENU_EVENT_TYPE_CEF_ONOPENLINK: {
        locker.unlock();

        CAscOnOpenExternalLink * pData = (CAscOnOpenExternalLink *)_event->m_pData;
        Utils::openUrl( QString::fromStdWString(pData->get_Url()) );

        RELEASEINTERFACE(_event);
        return; }

    case ASC_MENU_EVENT_TYPE_CEF_EXECUTE_COMMAND: {
        CAscExecCommand * pData = static_cast<CAscExecCommand *>(_event->m_pData);
        std::wstring cmd = pData->get_Command();

        if ( cmd.compare(L"portal:logout") == 0 ) {
            broadcastEvent(_event);
            return;
        } else
        if ( cmd.compare(0, 8, L"settings") == 0 ) {
            if ( cmd.rfind(L"apply") != wstring::npos ) {
                applySettings(pData->get_Param());
            } else
            if ( cmd.rfind(L"get") != wstring::npos ) {
                if ( pData->get_Param() == L"username" ) {
                    QTimer::singleShot(0, [] {
                        AscAppManager::sendCommandTo(SEND_TO_ALL_START_PAGE, L"settings:username", Utils::systemUserName());
                    });
                }
            }

            return;
        }

        break; }

    case ASC_MENU_EVENT_TYPE_SSO_TOKEN: {
//        CAscSSOToken * pData = (CAscSSOToken *)_event->m_pData;
        return; }

    case ASC_MENU_EVENT_TYPE_REPORTER_CREATE: {
        CSingleWindow * pEditorWindow = createReporterWindow(_event->m_pData, _event->get_SenderId());
#ifdef __linux
        pEditorWindow->show();
#else
        pEditorWindow->show(false);
        pEditorWindow->toggleBorderless(false);
#endif

        RELEASEINTERFACE(_event);
        return; }

    case ASC_MENU_EVENT_TYPE_REPORTER_END: {
        // close editor window
        CAscTypeId * pData = (CAscTypeId *)_event->m_pData;
        CSingleWindow * pWindow = editorWindowFromViewId(pData->get_Id());
        if ( pWindow ) {
            pWindow->hide();
            AscAppManager::getInstance().DestroyCefView(pData->get_Id());
        }

        RELEASEINTERFACE(_event);
        return; }

    case ASC_MENU_EVENT_TYPE_REPORTER_MESSAGE_TO:
    case ASC_MENU_EVENT_TYPE_REPORTER_MESSAGE_FROM: {
        CAscReporterMessage * pData = (CAscReporterMessage *)_event->m_pData;
        CCefView * pView = GetViewById(pData->get_ReceiverId());
        if ( pView ) {
            pView->Apply(_event);
        }
        return; }
    case ASC_MENU_EVENT_TYPE_UI_THREAD_MESSAGE: {
        this->Apply(_event);
        return; }

    case ASC_MENU_EVENT_TYPE_PAGE_SELECT_OPENSSL_CERTIFICATE: {
#ifdef DOCUMENTSCORE_OPENSSL_SUPPORT
        CMainWindow * _window = mainWindowFromViewId(_event->get_SenderId());
        if ( _window ) {
            _window->sendSertificate(_event->get_SenderId());
        }
#endif
        return; }
    case ASC_MENU_EVENT_TYPE_CEF_DESTROYWINDOW: {
        CSingleWindow * pWindow = editorWindowFromViewId(_event->get_SenderId());
        if ( pWindow )
            closeEditorWindow(size_t(pWindow));
        break;
    }

    case ASC_MENU_EVENT_TYPE_SYSTEM_EXTERNAL_PLUGINS: {
        auto _send_plugins = [](IMenuEventDataBase * d){
            CAscSystemExternalPlugins * pData = static_cast<CAscSystemExternalPlugins *>(d);
            QJsonObject _json_obj;

            for (const CAscSystemExternalPlugins::CItem& item: pData->get_Items()) {
                _json_obj["name"] = QString::fromStdWString(item.name);
                _json_obj["id"] = QString::fromStdWString(item.id);
                _json_obj["url"] = QString::fromStdWString(item.url);

                AscAppManager::sendCommandTo(SEND_TO_ALL_START_PAGE, "panel:external", Utils::encodeJson(_json_obj));
            }
        };

        QTimer::singleShot(0, [=] {
            _send_plugins(_event->m_pData);
        });

        return; }

    default: break;
    }


    for (auto const& w : m_vecWidows) {
        _window = reinterpret_cast<CMainWindow *>(w);

        if ( _event->m_nType == ASC_MENU_EVENT_TYPE_CEF_LOCALFILE_RECENTOPEN ||
                _event->m_nType == ASC_MENU_EVENT_TYPE_CEF_LOCALFILE_RECOVEROPEN )
        {
            CAscLocalOpenFileRecent_Recover * pData = (CAscLocalOpenFileRecent_Recover *)_event->m_pData;
            QString path = QString::fromStdWString(pData->get_Path());

            if ( _window->mainPanel()->holdUrl(path, etLocalFile) ) {
                _target = _window;
                break;
            }
        } else
        if ( _event->m_nType == ASC_MENU_EVENT_TYPE_CEF_PORTAL_OPEN ) {
            CAscExecCommand * pData = (CAscExecCommand *)_event->m_pData;
            QString url = QString::fromStdWString(pData->get_Param());

            if ( _window->mainPanel()->holdUrl(url, etPortal) ) {
                _target = _window;
                break;
            }
        }

        if ( _window->holdView(_uid) ) {
            _dest = _window;
        }
    }

    if ( _target )
        _window = _target; else
        _window = _dest;

    if ( _window ) {
        if ( (_event->m_nType == ASC_MENU_EVENT_TYPE_PAGE_GOT_FOCUS) && (countMainWindow() > 1) ) {
#ifdef __linux
            QApplication::setActiveWindow(_window);
#else
            ::SetForegroundWindow(_window->hWnd);
#endif
        }

        CCefEventsTransformer::OnEvent(_window->mainPanel(), _event);
    } else {
        RELEASEINTERFACE(_event);
    }
}

void CAscApplicationManagerWrapper::broadcastEvent(NSEditorApi::CAscCefMenuEvent * event)
{
    CMainWindow * _window;
    for ( auto const& w : m_vecWidows ) {
        _window = reinterpret_cast<CMainWindow *>(w);

        ADDREFINTERFACE(event);
        CCefEventsTransformer::OnEvent(_window->mainPanel(), event);
    }

    RELEASEINTERFACE(event);
}

CAscApplicationManager & CAscApplicationManagerWrapper::getInstance()
{
    static CAscApplicationManagerWrapper _manager;
    return _manager;
}

CAscApplicationManager * CAscApplicationManagerWrapper::createInstance()
{
    return new CAscApplicationManagerWrapper;
}

void CAscApplicationManagerWrapper::startApp()
{
    GET_REGISTRY_USER(reg_user)

    QRect _start_rect = reg_user.value("position").toRect();
    bool _is_maximized = reg_user.value("maximized", false).toBool();

    CMainWindow * _window = createMainWindow(_start_rect);

#ifdef __linux
    _window->show();
    if ( _is_maximized )
        _window->slot_windowChangeState(Qt::WindowMaximized);
#else
    _window->show(_is_maximized);
    _window->toggleBorderless(_is_maximized);

    if ( _is_maximized ) {
        WINDOWPLACEMENT wp{sizeof(WINDOWPLACEMENT)};
        if (GetWindowPlacement(_window->hWnd, &wp)) {
            wp.rcNormalPosition = {_start_rect.x(), _start_rect.y(), _start_rect.right(), _start_rect.bottom()};

            SetWindowPlacement(_window->hWnd, &wp);
        }
    }
#endif

#ifdef DOCUMENTSCORE_OPENSSL_SUPPORT
    APP_CAST(_app);
#endif
}

void CAscApplicationManagerWrapper::initializeApp()
{
    APP_CAST(_app);
    _app.m_private->initializeApp();

#ifdef _WIN32
    CSplash::showSplash();
    QApplication::processEvents();
#endif

    /* prevent drawing of focus rectangle on a button */
    QApplication::setStyle(new CStyleTweaks);

    GET_REGISTRY_SYSTEM(reg_system)
    GET_REGISTRY_USER(reg_user)
    reg_user.setFallbacksEnabled(false);

    // read installation time and clean cash folders if expired
    if ( reg_system.contains("timestamp") ) {
        QString user_data_path = Utils::getUserPath() + APP_DATA_PATH;

        QDateTime time_install, time_clear;
        time_install.setMSecsSinceEpoch(reg_system.value("timestamp", 0).toULongLong());

        bool clean = true;
        if ( reg_user.contains("timestamp") ) {
            time_clear.setMSecsSinceEpoch(reg_user.value("timestamp", 0).toULongLong());

            clean = time_install > time_clear;
        }

        if ( clean ) {
            reg_user.setValue("timestamp", QDateTime::currentDateTime().toMSecsSinceEpoch());
            QDir(user_data_path + "/fonts").removeRecursively();
        }
    }

    _app.m_vecStyles.push_back(":styles/res/styles/styles.qss");
    _app.m_vecStyles2x.push_back(":styles@2x/styles.qss");
    _app.m_private->applyStylesheets();

    // TODO: merge stylesheets and apply for the whole app
    qApp->setStyleSheet( Utils::readStylesheets(":styles/res/styles/styles.qss") );

    // Font
    QFont mainFont = QApplication::font();
    mainFont.setStyleStrategy( QFont::PreferAntialias );
    QApplication::setFont( mainFont );

}

CMainWindow * CAscApplicationManagerWrapper::createMainWindow(QRect& rect)
{
    APP_CAST(_app)

    QMutexLocker locker( &_app.m_oMutex );

    CMainWindow * _window = new CMainWindow(rect);
    _app.m_vecWidows.push_back( size_t(_window) );

    return _window;
}

CSingleWindow * CAscApplicationManagerWrapper::createReporterWindow(void * data, int parentid)
{
//    QMutexLocker locker( &m_oMutex );

    CAscReporterCreate * pData = (CAscReporterCreate *)data;
    CAscReporterData * pCreateData = reinterpret_cast<CAscReporterData *>(pData->get_Data());
    pData->put_Data(NULL);

    QCefView * pView = new QCefView(NULL);
    pView->CreateReporter(this, pCreateData);

    QRect _windowRect{100,100,1000,700};
    if ( QApplication::desktop()->screenCount() > 1 ) {
        CMainWindow * w = mainWindowFromViewId(parentid);
        if ( w ) {
            int _scrNum = QApplication::desktop()->screenNumber(w->windowRect().topLeft());
            QRect _scrRect = QApplication::desktop()->screenGeometry(QApplication::desktop()->screenCount()-_scrNum-1);

            _windowRect.setSize(QSize(1000,700));
            _windowRect.moveCenter(_scrRect.center());
        }
    }

    CSingleWindow * _window = new CSingleWindow(_windowRect, "ONLYOFFICE Reporter Window", pView);

    m_vecEditors.push_back( size_t(_window) );

    return _window;
}

void CAscApplicationManagerWrapper::closeMainWindow(const size_t p)
{
    APP_CAST(_app)

    QMutexLocker locker( &_app.m_oMutex );
    size_t _size = _app.m_vecWidows.size();

    if ( _size > 1 ) {
        vector<size_t>::iterator it = _app.m_vecWidows.begin();
        while ( it != _app.m_vecWidows.end() ) {
            if ( *it == p && _app.m_vecWidows.size() ) {
                CMainWindow * _w = reinterpret_cast<CMainWindow*>(*it);

                delete _w, _w = nullptr;

                _app.m_vecWidows.erase(it);
                break;
            }

            ++it;
        }
    } else
    if ( _size == 1 && _app.m_vecWidows[0] == p ) {
        AscAppManager::getInstance().DestroyCefView(-1);
    }
}

void CAscApplicationManagerWrapper::closeEditorWindow(const size_t p)
{
    APP_CAST(_app)

//    QMutexLocker locker( &_app.m_oMutex );

    vector<size_t>::iterator it = _app.m_vecEditors.begin();
    while ( it != _app.m_vecEditors.end() ) {
        if ( *it == p && _app.m_vecEditors.size() ) {
            CSingleWindow * _w = reinterpret_cast<CSingleWindow *>(*it);

            delete _w, _w = nullptr;

            _app.m_vecEditors.erase(it);
            break;
        }

        ++it;
    }
}

uint CAscApplicationManagerWrapper::countMainWindow()
{
    APP_CAST(_app)

    return _app.m_vecWidows.size();
}

CMainWindow * CAscApplicationManagerWrapper::mainWindowFromViewId(int uid) const
{
    CMainWindow * _window = nullptr;

    for (auto const& w : m_vecWidows) {
        _window = reinterpret_cast<CMainWindow *>(w);

        if ( _window->holdView(uid) )
            return _window;
    }

    // TODO: remove for multi-windowed mode
    if ( !m_vecWidows.empty() ) {
        return reinterpret_cast<CMainWindow *>(m_vecWidows.at(0));
    }

    return 0;
}

CSingleWindow * CAscApplicationManagerWrapper::editorWindowFromViewId(int uid) const
{
    CSingleWindow * _window = nullptr;

    for (auto const& w : m_vecEditors) {
        _window = reinterpret_cast<CSingleWindow *>(w);

        if ( _window->holdView(uid) )
            return _window;
    }

    return 0;
}

void CAscApplicationManagerWrapper::processMainWindowMoving(const size_t s, const QPoint& c)
{
#define GET_CURRENT_PANEL -1
    APP_CAST(_app);

    if ( _app.m_vecWidows.size() > 1 ) {
        CMainWindow * _window = nullptr,
                    * _source = reinterpret_cast<CMainWindow *>(s);
        for (auto const& w : _app.m_vecWidows) {
            if ( w != s ) {
                _window = reinterpret_cast<CMainWindow *>(w);

                if ( _window->pointInTabs(c) ) {
                    _source->hide();
                    _window->attachEditor(
                            _source->editorPanel(GET_CURRENT_PANEL), c );

                    closeMainWindow(s);
                    break;
                }
            }
        }
    }
}

CMainWindow * CAscApplicationManagerWrapper::topWindow()
{
    APP_CAST(_app);

    if ( _app.m_vecWidows.size() > 0 )
        return reinterpret_cast<CMainWindow *>(_app.m_vecWidows.at(0));
    else return nullptr;
}

void CAscApplicationManagerWrapper::sendCommandTo(QCefView * target, const QString& cmd, const QString& args)
{
    sendCommandTo(target, cmd.toStdWString(), args.toStdWString() );
}

void CAscApplicationManagerWrapper::sendCommandTo(QCefView * target, const wstring& cmd, const wstring& args)
{
    CAscExecCommandJS * pCommand = new CAscExecCommandJS;
    pCommand->put_Command(cmd);
    if ( !args.empty() )
        pCommand->put_Param(args);

    CAscMenuEvent * pEvent = new CAscMenuEvent(ASC_MENU_EVENT_TYPE_CEF_EXECUTE_COMMAND_JS);
    pEvent->m_pData = pCommand;

    if ( target )
        target->GetCefView()->Apply(pEvent); else
        AscAppManager::getInstance().SetEventToAllMainWindows(pEvent);

//    delete pCommand;
//    delete pEvent;
}

void CAscApplicationManagerWrapper::sendEvent(int type, void * data)
{
    CAscMenuEvent * pEvent = new CAscMenuEvent(type);
    pEvent->m_pData = static_cast<IMenuEventDataBase *>(data);

    AscAppManager::getInstance().Apply(pEvent);

//    delete pEvent;
}

QString CAscApplicationManagerWrapper::getWindowStylesheets(uint dpifactor)
{
    APP_CAST(_app);
    return Utils::readStylesheets(&_app.m_vecStyles, &_app.m_vecStyles2x, dpifactor);
}

bool CAscApplicationManagerWrapper::event(QEvent *event)
{
    if ( event->type() == CTabUndockEvent::type() ) {
        CTabUndockEvent * e = (CTabUndockEvent *)event;
        if ( e->panel() ) {
            e->accept();

            QWidget * _panel = e->panel();
            QTimer::singleShot(0, this, [=]{
                CMainWindow * _main_window = nullptr;

#ifdef _WIN32
                CWinPanel * _wp = dynamic_cast<CWinPanel *>(_panel->window());
                if ( _wp ) _main_window = _wp->parent();
#else
                _main_window = dynamic_cast<CMainWindow *>(_panel->window());
#endif

                if ( _main_window ) {
                    QRect _win_rect = _main_window->windowRect();
                    _win_rect.moveTo(QCursor::pos() - QPoint(BUTTON_MAIN_WIDTH + 50, 20));
                    CMainWindow * window = createMainWindow(_win_rect);

                    bool _is_maximized = _main_window->isMaximized();
#ifdef Q_OS_WIN
                    window->show(_is_maximized);
                    window->toggleBorderless(_is_maximized);
#else
                    window->show();
                    if ( _is_maximized )
                        window->slot_windowChangeState(Qt::WindowMaximized);
#endif
                    window->attachEditor( _panel );
                }
            });
        }

        return true;
    }

    return QObject::event(event);
}

bool CAscApplicationManagerWrapper::applySettings(const wstring& wstrjson)
{
    QJsonParseError jerror;
    QJsonDocument jdoc = QJsonDocument::fromJson(QString::fromStdWString(wstrjson).toUtf8(), &jerror);

    if( jerror.error == QJsonParseError::NoError ) {
        QJsonObject objRoot = jdoc.object();

        QString _user_newname = objRoot["username"].toString();
        if ( _user_newname.isEmpty() )
            _user_newname = QString::fromStdWString(Utils::systemUserName());

        QString _lang_id = CLangater::getCurrentLangCode();
        if ( objRoot.contains("langid") ) {
            _lang_id = objRoot.value("langid").toString();

            GET_REGISTRY_USER(_reg_user)
            _reg_user.setValue("locale", _lang_id);
        }

        wstring params = QString("lang=%1&username=%3&location=%2")
                            .arg(_lang_id, Utils::systemLocationCode(), QUrl::toPercentEncoding(_user_newname)).toStdWString();

        if ( objRoot["docopenmode"].toString() == "view" )
            params.append(L"&mode=view");

        AscAppManager::getInstance().InitAdditionalEditorParams( params );
    } else {
        /* parse settings error */
    }

    return true;
}
