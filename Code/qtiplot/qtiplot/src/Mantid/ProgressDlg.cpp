#include "ProgressDlg.h"

#include <QtGui>

ProgressDlg::ProgressDlg(QWidget *parent):QDialog(parent)
{
    QVBoxLayout *topLayout = new QVBoxLayout;
    QLabel *label = new QLabel("Algorithm progress");
    m_progressBar = new QProgressBar;
    topLayout->addWidget(label);
    topLayout->addWidget(m_progressBar);

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
}

void ProgressDlg::cancelClicked()
{
    emit canceled();
}

void ProgressDlg::backgroundClicked()
{
    emit toBackground();
}

void ProgressDlg::setValue(int p)
{
    m_progressBar->setValue(p);
}
