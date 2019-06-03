/*
 * (c) Copyright Ascensio System SIA 2010-2019
 *
 * This program is a free software product. You can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License (AGPL)
 * version 3 as published by the Free Software Foundation. In accordance with
 * Section 7(a) of the GNU AGPL its Section 15 shall be amended to the effect
 * that Ascensio System SIA expressly excludes the warranty of non-infringement
 * of any third-party rights.
 *
 * This program is distributed WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR  PURPOSE. For
 * details, see the GNU AGPL at: http://www.gnu.org/licenses/agpl-3.0.html
 *
 * You can contact Ascensio System SIA at 20A-12 Ernesta Birznieka-Upisha
 * street, Riga, Latvia, EU, LV-1050.
 *
 * The  interactive user interfaces in modified source and object code versions
 * of the Program must display Appropriate Legal Notices, as required under
 * Section 5 of the GNU AGPL version 3.
 *
 * Pursuant to Section 7(b) of the License you must retain the original Product
 * logo when distributing the program. Pursuant to Section 7(e) we decline to
 * grant you any rights under trademark law for use of our trademarks.
 *
 * All the Product's GUI elements, including illustrations and icon sets, as
 * well as technical writing content are licensed under the terms of the
 * Creative Commons Attribution-ShareAlike 4.0 International. See the License
 * terms at http://creativecommons.org/licenses/by-sa/4.0/legalcode
 *
*/

#ifndef CEDITORWINDOW_P_H
#define CEDITORWINDOW_P_H

#include "ccefeventsgate.h"
#include "ceditorwindow.h"
#include "cascapplicationmanagerwrapper.h"
#include "cascapplicationmanagerwrapper_private.h"
#include "applicationmanager_events.h"
#include "utils.h"
#include "common/Types.h"
#include "cmessage.h"
#include "qascprinter.h"
#include "ceditortools.h"

#include <QPrinterInfo>
#include <QDesktopWidget>
#include <QJsonDocument>

#ifdef _WIN32
#include "win/cprintdialog.h"
#else
#endif


using namespace NSEditorApi;

class CEditorWindowPrivate : public CCefEventsGate
{
    struct sPrintData {
        sPrintData() : _print_range(QPrintDialog::PrintRange::AllPages)
        {}

        QPrinterInfo _printer_info;
        QPrintDialog::PrintRange _print_range;
    };

    sPrintData m_printData;

public:
    CEditorWindowPrivate(CEditorWindow * w) : window(w)
    {}

    void onEditorConfig(int, std::wstring cfg)
    {
//        if ( id == window->holdView(id) )

        QJsonParseError jerror;
        QJsonDocument jdoc = QJsonDocument::fromJson(QString::fromStdWString(cfg).toLatin1(), &jerror);

        if( jerror.error == QJsonParseError::NoError ) {
            QJsonObject objRoot = jdoc.object();

            if ( objRoot.contains("user") ) {
                iconuser->setToolTip(objRoot["user"].toObject().value("name").toString());
            }

            if ( objRoot.contains("extraleft") ) {
                titleLeftOffset = objRoot["extraleft"].toInt();

                int diffW = (titleLeftOffset - (TOOLBTN_WIDTH * 5)) * window->m_dpiRatio; // 5 right tool buttons: close, min, max, drop, user icon
                QString _label_styles = diffW > 0 ? QString("padding:0 %1px 0 0;").arg(diffW) : QString("padding:0 0 0 %1px;").arg(abs(diffW));
                window->m_labelTitle->setStyleSheet(_label_styles);
            }
        }
    }

    void onDocumentName(void * data) override
    {
        CCefEventsGate::onDocumentName(data);

        window->setWindowTitle(m_panel->data()->title());
        window->m_boxTitleBtns->repaint();
    }

    void onDocumentChanged(int id, bool state) override
    {
        if ( panel()->data()->hasChanges() != state ) {
            CCefEventsGate::onDocumentChanged(id, state);

            window->setWindowTitle(m_panel->data()->title());
            window->m_boxTitleBtns->repaint();
        }
    }

    void onDocumentSave(int id, bool cancel = false) override
    {
        CCefEventsGate::onDocumentSave(id, cancel);

        if ( m_panel->data()->closed() ) {
            if ( !((CCefViewEditor *)m_panel->cef())->CheckCloudCryptoNeedBuild() ) {
                AscAppManager::getInstance().DestroyCefView(m_panel->cef()->GetId());
                window->hide();
            }
        } else AscAppManager::cancelClose();
    }

    void onDocumentSaveInnerRequest(int) override
    {
        CMessage mess(parentWindow(), CMessageOpts::moButtons::mbYesDefNo);
        int reply = mess.confirm(QObject::tr("Document must be saved to continue.<br>Save the document?"));

        CAscEditorSaveQuestion * pData = new CAscEditorSaveQuestion;
        pData->put_Value((reply == MODAL_RESULT_CUSTOM + 0) ? true : false);

        CAscMenuEvent * pEvent = new CAscMenuEvent(ASC_MENU_EVENT_TYPE_DOCUMENTEDITORS_SAVE_YES_NO);
        pEvent->m_pData = pData;

        m_panel->cef()->Apply(pEvent);
    }

    void onDocumentFragmented(int id, bool needbuild) override
    {
        if ( needbuild ) {
//            static const bool _skip_user_warning = !Utils::appArgsContains("--warning-doc-fragmented");
//            if ( _skip_user_warning )
                m_panel->cef()->Apply(new CAscMenuEvent(ASC_MENU_EVENT_TYPE_ENCRYPTED_CLOUD_BUILD));
        } else
            onDocumentFragmentedBuild(id, 0);

    }

    void onDocumentFragmentedBuild(int id, int error) override
    {
        CCefEventsGate::onDocumentFragmentedBuild(id, error);

        if ( m_panel->data()->closed() ) {
            AscAppManager::getInstance().DestroyCefView(m_panel->cef()->GetId());
            window->hide();
        } else AscAppManager::cancelClose();
    }

    void onDocumentPrint(int currentpage, uint pagescount) override
    {
        if ( isPrinting ) return;
        isPrinting = true;

        if ( !(pagescount < 1) ) {
            CAscMenuEvent * pEvent;
            QAscPrinterContext * pContext = m_printData._printer_info.isNull() ?
                        new QAscPrinterContext() : new QAscPrinterContext(m_printData._printer_info);

            QPrinter * printer = pContext->getPrinter();
            printer->setOutputFileName("");
            printer->setFromTo(1, pagescount);

#ifdef _WIN32
            CPrintDialogWinWrapper wrapper(printer, parentWindow());
            QPrintDialog * dialog = wrapper.q_dialog();
#else
            QPrintDialog * dialog =  new QPrintDialog(printer, this);
#endif // _WIN32

            dialog->setWindowTitle(tr("Print Document"));
            dialog->setEnabledOptions(QPrintDialog::PrintPageRange | QPrintDialog::PrintCurrentPage | QPrintDialog::PrintToFile);
            if (!(currentpage < 0))
                currentpage++, dialog->setOptions(dialog->options() | QPrintDialog::PrintCurrentPage);
            dialog->setPrintRange(m_printData._print_range);

            int start = -1, finish = -1;
            if ( dialog->exec() == QDialog::Accepted ) {
                m_printData._printer_info = QPrinterInfo::printerInfo(printer->printerName());
                m_printData._print_range = dialog->printRange();

                switch(dialog->printRange()) {
                case QPrintDialog::AllPages: start = 1, finish = pagescount; break;
                case QPrintDialog::PageRange:
                    start = dialog->fromPage(), finish = dialog->toPage(); break;
                case QPrintDialog::Selection: break;
                case QPrintDialog::CurrentPage: start = currentpage, finish = currentpage; break;
                }

                CEditorTools::print({m_panel->cef(), pContext, start, finish, parentWindow()});
            }

            pContext->Release();

            pEvent = new CAscMenuEvent(ASC_MENU_EVENT_TYPE_CEF_PRINT_END);
            m_panel->cef()->Apply(pEvent);
    //        RELEASEOBJECT(pEvent)

#ifndef _WIN32
            RELEASEOBJECT(dialog)
#endif
        }

        isPrinting = false;
    }


    void onEditorAllowedClose(int) override
    {
        AscAppManager::closeEditorWindow(size_t(window));
    }

    void onLocalFileSaveAs(void * d) override
    {
        window->onLocalFileSaveAs(d);
    }

    void onScreenScalingFactor(uint f)
    {
        int diffW = (titleLeftOffset - (TOOLBTN_WIDTH * 5)) * f; // 5 tool buttons: min+max+close+drop+usericon
        QString _label_styles = diffW > 0 ? QString("padding:0 %1px 0 0;").arg(diffW) : QString("padding:0 0 0 %1px;").arg(abs(diffW));
        window->m_labelTitle->setStyleSheet(_label_styles);

        if ( iconuser ) {
            iconuser->setPixmap(f > 1 ? QPixmap(":/user_2x.png") : QPixmap(":/user.png"));
            iconuser->setFixedSize(QSize(TOOLBTN_WIDTH*f, 16*f));
        }

        if ( btndock )
            btndock->setFixedSize(QSize(TOOLBTN_WIDTH*f, TOOLBTN_HEIGHT*f));
    }

    void onFullScreen(bool apply)
    {
        if ( apply == panel()->windowState().testFlag(Qt::WindowFullScreen) )
            return;

        auto _break_demonstration = [&] {
            CAscExecCommandJS * pCommand = new CAscExecCommandJS;
            pCommand->put_Command(L"editor:stopDemonstration");

            CAscMenuEvent * pEvent = new CAscMenuEvent(ASC_MENU_EVENT_TYPE_CEF_EDITOR_EXECUTE_COMMAND);
            pEvent->m_pData = pCommand;
            panel()->cef()->Apply(pEvent);
        };

        CTabPanel * _fs_widget = panel();
        static QMetaObject::Connection cefConnection;
        if (!apply) {
            _break_demonstration();

            window->show(false);

            _fs_widget->showNormal();
            window->m_pMainPanel->layout()->addWidget(_fs_widget);
            window->recalculatePlaces();

            disconnect(cefConnection);
        } else {
#ifdef _WIN32
            _fs_widget->clearMask();
            _fs_widget->setWindowIcon(Utils::appIcon());
            _fs_widget->setParent(nullptr);
            window->hide();
#else
//                m_dataFullScreen->parent = qobject_cast<QWidget *>(parent());
//                QWidget * grandpa = qobject_cast<QWidget *>(m_dataFullScreen->parent->parent());
//                if (grandpa) {
//                    fsWidget->setParent(grandpa);
//                    m_dataFullScreen->parent->hide();
                }
#endif
            _fs_widget->showFullScreen();
            _fs_widget->cef()->focus();

            cefConnection = connect(_fs_widget->view(), &QCefView::closeWidget, [=](QCloseEvent * e){
                _break_demonstration();

                e->ignore();
                window->closeWindow();
            });

            QPoint pt = _fs_widget->mapToGlobal(_fs_widget->pos());
#ifdef _WIN32
            _fs_widget->setGeometry(QApplication::desktop()->screenGeometry(pt));
#else

//            QRect _scr_rect = QApplication::desktop()->screenGeometry(pt);
//            fsWidget->setGeometry(QRect(QPoint(0,0), _scr_rect.size()));
#endif
        }
    }

    void onFullScreen(int, bool apply) override
    {
        onFullScreen(apply);
    }

    void onPortalLogout(wstring portal) override
    {
        if ( m_panel && !portal.empty() ) {
            if ( !m_panel->data()->closed() &&
                    m_panel->data()->url().find(portal) != wstring::npos )
            {
                window->closeWindow();
            }
        }
    }

    void onFileLocation(int, QString param)
    {
        if ( m_panel->data()->local() ) {
            const wstring& path = m_panel->data()->url();
            if (!path.empty()) {
                Utils::openFileLocation(QString::fromStdWString(path));
            } else
                CMessage::info(parentWindow(), QObject::tr("Document must be saved firstly."));
        } else {
            QRegularExpression _re("^((?:https?:\\/{2})?[^\\s\\/]+)", QRegularExpression::CaseInsensitiveOption);
            QRegularExpressionMatch _re_match = _re.match(param);

            if ( _re_match.hasMatch() ) {
                QString _domain = _re_match.captured(1);
                QString _folder = param;

                if ( !_folder.contains("desktop=true") ) {
                    if ( _folder.contains("?") )
                        _folder.append("&desktop=true");
                    else {
                        int pos = _folder.indexOf(QRegularExpression("#\\d+"));
                        !(pos < 0) ? _folder.insert(pos, "?desktop=true&") : _folder.append("?desktop=true");
                    }
                }

                // emulate 'open portal' command
                QJsonObject _json_obj{{"portal", _folder}};

                CAscExecCommand * pData = new CAscExecCommand;
                pData->put_Command(L"portal:open");
                pData->put_Param(QString(QJsonDocument(_json_obj).toJson(QJsonDocument::Compact)).toStdWString());

                CAscCefMenuEvent * event = new CAscCefMenuEvent(ASC_MENU_EVENT_TYPE_CEF_EXECUTE_COMMAND);
                event->m_nSenderId = 1; // id of 'start' page
                event->m_pData = pData;

                CAscApplicationManagerWrapper& app = static_cast<CAscApplicationManagerWrapper &>(AscAppManager::getInstance());
                app.OnEvent(event);
            }
        }
    }

    QLabel * const iconUser()
    {
        if ( !iconuser ) {
            iconuser = new QLabel(window->m_boxTitleBtns);
            iconuser->setPixmap(window->m_dpiRatio > 1 ? QPixmap(":/user_2x.png") : QPixmap(":/user.png"));
            iconuser->setFixedSize(QSize(TOOLBTN_WIDTH*window->m_dpiRatio,16*window->m_dpiRatio));
            iconuser->setAlignment(Qt::AlignCenter);
        }

        return iconuser;
    }

    QPushButton * const buttonDock()
    {
        if ( !btndock ) {
            btndock = window->createToolButton(window->m_boxTitleBtns);
            btndock->setObjectName("toolButtonDock");
        }

        return btndock;
    }

#ifdef Q_OS_WIN
    HWND parentWindow() const
    {
        return (HWND)window->m_pMainPanel->winId();
    }
#else
    QWidget * parentWindow() const
    {
        return window->m_pMainPanel;
    }
#endif

public:
    int titleLeftOffset = 168;

private:
    CEditorWindow * window = nullptr;
    QLabel * iconuser = nullptr;
    QPushButton * btndock = nullptr;
    bool isPrinting = false;
};

#endif // CEDITORWINDOW_P_H
