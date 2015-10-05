
#include <QValidator>
#include <QtGui>
#include <qfiledialog.h>

#include "LoadDAEDlg.h"
#include "InputHistory.h"

loadDAEDlg::loadDAEDlg(QWidget *parent) : QDialog(parent), m_hostName(""), m_workspaceName(""),m_spectrum_min(""),m_spectrum_max(""),m_spectrum_list(""),
m_updateInterval(0)
{
	QGridLayout *paramsLayout = new QGridLayout;
    QLabel *label = new QLabel(tr("DAE Name"));
	lineHost = new QLineEdit;
	label->setBuddy(lineHost);
	paramsLayout->addWidget(label,0,0);
	paramsLayout->addWidget(lineHost,0,1);
    QString propValue = InputHistory::Instance().algorithmProperty("LoadDAE","DAEname");
    if (!propValue.isEmpty())
    {
        lineHost->setText(propValue);
    }
	
	label = new QLabel(tr("Workspace Name"));
	lineName = new QLineEdit;
	label->setBuddy(lineName);
	paramsLayout->addWidget(label,1,0);
	paramsLayout->addWidget(lineName,1,1);
    propValue = InputHistory::Instance().algorithmProperty("LoadDAE","OutputWorkspace");
    if (!propValue.isEmpty())
    {
        lineName->setText(propValue);
    }
	
	QHBoxLayout *bottomRowLayout = new QHBoxLayout;
	QPushButton *loadButton = new QPushButton(tr("Load"));
	loadButton->setDefault(true);
	QPushButton *cancelButton = new QPushButton(tr("Cancel"));
	QPushButton *help = new QPushButton("?");
	help->setMaximumWidth(25);
	connect(help, SIGNAL(clicked()), this, SLOT(helpClicked()));
	
	bottomRowLayout->addWidget(help);
	bottomRowLayout->addStretch();
	bottomRowLayout->addWidget(cancelButton);
	bottomRowLayout->addWidget(loadButton);

	connect(cancelButton, SIGNAL(clicked()), this, SLOT(close()));
	connect(loadButton, SIGNAL(clicked()), this, SLOT(load()));
	
    QLabel *minSpLabel = new QLabel("Starting spectrum");
    minSpLineEdit = new QLineEdit;
    propValue = InputHistory::Instance().algorithmProperty("LoadRaw","spectrum_min");
    if (!propValue.isEmpty())
    {
        minSpLineEdit->setText(propValue);
    }
    paramsLayout->addWidget(minSpLabel,2,0);
    paramsLayout->addWidget(minSpLineEdit,2,1);
    QLabel *maxSpLabel = new QLabel("Ending spectrum");
    maxSpLineEdit = new QLineEdit;
    propValue = InputHistory::Instance().algorithmProperty("LoadDAE","spectrum_max");
    if (!propValue.isEmpty())
    {
        maxSpLineEdit->setText(propValue);
    }
    paramsLayout->addWidget(maxSpLabel,3,0);
    paramsLayout->addWidget(maxSpLineEdit,3,1);
    QLabel *listSpLabel = new QLabel("Spectrum List");
    listSpLineEdit = new QLineEdit;
    propValue = InputHistory::Instance().algorithmProperty("LoadDAE","spectrum_list");
    if (!propValue.isEmpty())
    {
        listSpLineEdit->setText(propValue);
    }
    paramsLayout->addWidget(listSpLabel,4,0);
    paramsLayout->addWidget(listSpLineEdit,4,1);

	QHBoxLayout *updateLayout = new QHBoxLayout;
    updateCheck = new QCheckBox("Update every");
    updateLineEdit = new QLineEdit;
    QIntValidator *ival = new QIntValidator(1,99999999,updateLineEdit);
    updateLineEdit->setValidator(ival);
    propValue = InputHistory::Instance().algorithmProperty("UpdateDAE","update_rate");
    if (!propValue.isEmpty())
    {
        updateLineEdit->setText(propValue);
        updateCheck->setCheckState(Qt::Checked);
    }
    label = new QLabel(" seconds");
    paramsLayout->addWidget(updateCheck,5,0);
    updateLayout->addWidget(updateLineEdit);
    updateLayout->addWidget(label);
    paramsLayout->addLayout(updateLayout,5,1);
    connect(updateCheck,SIGNAL(stateChanged(int)),this,SLOT(changeUpdateState(int)));
    connect(updateLineEdit,SIGNAL(textEdited(const QString & )),this,SLOT(updateIntervalEntered(const QString &)));


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
        {
            m_updateInterval = 0;
            InputHistory::Instance().updateAlgorithmProperty("UpdateDAE","update_rate","");
        }
		close();
	}
}

void loadDAEDlg::changeUpdateState(int state)
{
    if (state == Qt::Checked && updateLineEdit->text().isEmpty()) updateLineEdit->setText("10");
}

void loadDAEDlg::updateIntervalEntered(const QString & text )
{
    if (!text.isEmpty())
        updateCheck->setCheckState(Qt::Checked);
    else
        updateCheck->setCheckState(Qt::Unchecked);
}

void loadDAEDlg::helpClicked()
{
  QDesktopServices::openUrl(QUrl(QString("http://www.mantidproject.org/LoadDAE")));
}
