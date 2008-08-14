
#include <QValidator>
#include <QtGui>
#include <qfiledialog.h>

#include "loadDAEDlg.h"

loadDAEDlg::loadDAEDlg(QWidget *parent) : QDialog(parent), m_hostName(""), m_workspaceName(""),m_spectrum_min(""),m_spectrum_max(""),m_spectrum_list(""),
m_updateInterval(0)
{
	QGridLayout *paramsLayout = new QGridLayout;
    QLabel *label = new QLabel(tr("DAE Name"));
	lineHost = new QLineEdit;
	label->setBuddy(lineHost);
	paramsLayout->addWidget(label,0,0);
	paramsLayout->addWidget(lineHost,0,1);
	
	label = new QLabel(tr("Workspace Name"));
	lineName = new QLineEdit;
	label->setBuddy(lineName);
	paramsLayout->addWidget(label,1,0);
	paramsLayout->addWidget(lineName,1,1);
	
	QHBoxLayout *bottomRowLayout = new QHBoxLayout;
	QPushButton *loadButton = new QPushButton(tr("Load"));
	QPushButton *cancelButton = new QPushButton(tr("Cancel"));
	bottomRowLayout->addStretch();
	bottomRowLayout->addWidget(cancelButton);
	bottomRowLayout->addWidget(loadButton);

	connect(cancelButton, SIGNAL(clicked()), this, SLOT(close()));
	connect(loadButton, SIGNAL(clicked()), this, SLOT(load()));
	
    QLabel *minSpLabel = new QLabel("Starting spectrum");
    minSpLineEdit = new QLineEdit;
    paramsLayout->addWidget(minSpLabel,2,0);
    paramsLayout->addWidget(minSpLineEdit,2,1);
    QLabel *maxSpLabel = new QLabel("Ending spectrum");
    maxSpLineEdit = new QLineEdit;
    paramsLayout->addWidget(maxSpLabel,3,0);
    paramsLayout->addWidget(maxSpLineEdit,3,1);
    QLabel *listSpLabel = new QLabel("Spectrum List");
    listSpLineEdit = new QLineEdit;
    paramsLayout->addWidget(listSpLabel,4,0);
    paramsLayout->addWidget(listSpLineEdit,4,1);

	QHBoxLayout *updateLayout = new QHBoxLayout;
    updateCheck = new QCheckBox("Update every");
    updateLineEdit = new QLineEdit;
    QIntValidator *ival = new QIntValidator(1,99999999,updateLineEdit);
    updateLineEdit->setValidator(ival);
    label = new QLabel(" seconds");
    paramsLayout->addWidget(updateCheck,5,0);
    updateLayout->addWidget(updateLineEdit);
    updateLayout->addWidget(label);
    paramsLayout->addLayout(updateLayout,5,1);
    connect(updateCheck,SIGNAL(stateChanged(int)),this,SLOT(changeUpdateState(int)));


    QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addLayout(paramsLayout);
	mainLayout->addLayout(bottomRowLayout);
	
	setLayout(mainLayout);
	setWindowTitle(tr("Load Workspace from DAE"));
	setFixedHeight(sizeHint().height());
}

loadDAEDlg::~loadDAEDlg()
{
	
}

void loadDAEDlg::load()
{
	if (!lineHost->text().isNull() && !lineHost->text().isEmpty() && !lineName->text().isNull() && !lineName->text().isEmpty())
	{
		m_hostName = lineHost->text();
		m_workspaceName = lineName->text();
        m_spectrum_min = minSpLineEdit->text();
        m_spectrum_max = maxSpLineEdit->text();
        m_spectrum_list = listSpLineEdit->text();
        if (updateCheck->checkState() == Qt::Checked) m_updateInterval = updateLineEdit->text().toInt();
        else
            m_updateInterval = 0;
		close();
	}
}

void loadDAEDlg::changeUpdateState(int state)
{
    if (state == Qt::Checked) updateLineEdit->setReadOnly(false);
    else
        updateLineEdit->setReadOnly(true);
}
