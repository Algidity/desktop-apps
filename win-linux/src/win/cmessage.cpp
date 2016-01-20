#include "cmessage.h"
#include "../defines.h"

#include <QDialogButtonBox>
#include <QPushButton>

#include <QLabel>
#include "qcefview.h"

extern BYTE g_dpi_ratio;
extern QString g_lang;

CMessage::CMessage(HWND hParentWnd)
    : QWinWidget(hParentWnd)
    , m_pDlg(this)
    , m_result(MODAL_RESULT_CANCEL)
    , m_fLayout(new QFormLayout)
{
    m_pDlg.setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint
                          | Qt::WindowCloseButtonHint | Qt::MSWindowsFixedSizeDialogHint);

    QVBoxLayout * layout = new QVBoxLayout;
    QHBoxLayout * h_layout2 = new QHBoxLayout;
    QHBoxLayout * h_layout1 = new QHBoxLayout;
    layout->addLayout(h_layout2, 1);
    layout->addLayout(h_layout1, 0);

    m_typeIcon = new QLabel;
    m_typeIcon->setProperty("class","msg-icon");
    m_typeIcon->setFixedSize(35*g_dpi_ratio, 35*g_dpi_ratio);

    m_message = new QLabel;
//    m_message->setWordWrap(true);
    m_message->setStyleSheet(QString("margin-bottom: %1px;").arg(8*g_dpi_ratio));
//    question->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    m_fLayout->addWidget(m_message);
    m_fLayout->setSpacing(0);
    m_fLayout->setContentsMargins(10,0,5,0);
    h_layout2->addWidget(m_typeIcon, 0, Qt::AlignTop);
    h_layout2->addLayout(m_fLayout, 1);

    QPushButton * btn_yes       = new QPushButton(tr("&OK"));
//    QPushButton * btn_no        = new QPushButton("&No");
//    QPushButton * btn_cancel    = new QPushButton("&Cancel");
    m_boxButtons = new QWidget;
    m_boxButtons->setLayout(new QHBoxLayout);
    m_boxButtons->layout()->addWidget(btn_yes);
//    box->layout()->addWidget(btn_no);
//    box->layout()->addWidget(btn_cancel);
    m_boxButtons->layout()->setContentsMargins(0,8*g_dpi_ratio,0,0);
    h_layout1->addWidget(m_boxButtons, 0, Qt::AlignCenter);

    m_pDlg.setLayout(layout);
    m_pDlg.setMinimumWidth(350*g_dpi_ratio);
    m_pDlg.setWindowTitle(APP_TITLE);

    connect(btn_yes, &QPushButton::clicked, this, &CMessage::onYesClicked);
//    connect(btn_no, SIGNAL(clicked()), this, SLOT(onNoClicked()));
//    connect(btn_cancel, SIGNAL(clicked()), this, SLOT(onCancelClicked()));
}

void CMessage::error(const QString& title, const QString& text)
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle( title );
    msgBox.setText( text );
    msgBox.setWindowModality(Qt::ApplicationModal);
    msgBox.setIcon(QMessageBox::Critical);

    msgBox.setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint
                          | Qt::WindowCloseButtonHint | Qt::MSWindowsFixedSizeDialogHint);
    msgBox.exec();
}

int CMessage::showModal(const QString& mess, QMessageBox::Icon icon)
{
    m_message->setText(mess);
    if (icon == QMessageBox::Critical) {
        m_typeIcon->setProperty("type","msg-error");
    } else
    if (icon == QMessageBox::Information) {
        m_typeIcon->setProperty("type","msg-info");
    }

    m_pDlg.adjustSize();

#if defined(_WIN32)
    RECT rc;
    ::GetWindowRect(parentWindow(), &rc);

    int x = rc.left + (rc.right - rc.left - m_pDlg.width())/2;
    int y = (rc.bottom - rc.top - m_pDlg.height())/2;

    m_pDlg.move(x, y);
#endif

    m_pDlg.exec();

    return m_result;
}

void CMessage::onYesClicked()
{
    m_result = MODAL_RESULT_YES;
    m_pDlg.accept();
}

void CMessage::setButtons(const QString& cbtn1, const QString& cbtn2)
{
    foreach (QWidget * w, m_boxButtons->findChildren<QWidget*>()) {
        delete w;
    }

    QPushButton * _btn = new QPushButton(cbtn1);
    m_boxButtons->layout()->addWidget(_btn);
    connect(_btn, &QPushButton::clicked, [=](){
        m_result = 201;
        m_pDlg.accept();
    });

    if (cbtn2.size()) {
        _btn = new QPushButton(cbtn2);
        m_boxButtons->layout()->addWidget(_btn);
        connect(_btn, &QPushButton::clicked, [=](){
            m_result = 202;
            m_pDlg.accept();
        });
    }
}
