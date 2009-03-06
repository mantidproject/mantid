#include "ProgressDlg.h"

#include <QtGui>

ProgressDlg::ProgressDlg(Mantid::API::IAlgorithm_sptr alg,QWidget *parent):QDialog(parent),m_alg(alg)
{
    QVBoxLayout *topLayout = new QVBoxLayout;
    QLabel *label = new QLabel("Algorithm progress");
    m_progressBar = new QProgressBar;
    m_message = new QLabel("");
    topLayout->addWidget(label);
    topLayout->addWidget(m_progressBar);
    topLayout->addWidget(m_message);

    QPushButton *backgroundButton = new QPushButton("Run in background");
    QPushButton *cancelButton = new QPushButton("Cancel algorithm");
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(backgroundButton);
    buttonLayout->addWidget(cancelButton);
    connect(cancelButton,SIGNAL(clicked()),this,SLOT(cancelClicked()));
    connect(backgroundButton,SIGNAL(clicked()),this,SLOT(backgroundClicked()));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addLayout(topLayout);
    layout->addLayout(buttonLayout);

	setLayout(layout);
	setWindowTitle("Mantid - Algorithm progress");
	setFixedHeight(sizeHint().height());

    observeProgress(m_alg);
    observeFinish(m_alg);
    observeError(m_alg);
}

void ProgressDlg::cancelClicked()
{
    //emit canceled();
    m_alg->cancel();
    close();
}

void ProgressDlg::backgroundClicked()
{
    //emit toBackground();
    close();
}

void ProgressDlg::setValue(int p,const QString& msg)
{
    m_progressBar->setValue(p);
    m_message->setText(msg);
}

void ProgressDlg::progressHandle(const Mantid::API::IAlgorithm* alg,double p,const std::string& msg)
{
    setValue(int(p*100),QString::fromStdString(msg)); 
}

void ProgressDlg::finishHandle(const Mantid::API::IAlgorithm* alg)
{
    close();
}

void ProgressDlg::errorHandle(const Mantid::API::IAlgorithm* alg,const std::string& what)
{
    close();
}
