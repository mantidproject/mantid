#include <vector>
#include <string>
#include <QMessageBox>
#include <QListWidgetItem>
#include <QtGui>

#include "ExecuteAlgorithm.h"
#include "../ApplicationWindow.h"
#include "../Matrix.h"
#include "LoadRawDlg.h"
#include "ImportWorkspaceDlg.h"

ExecuteAlgorithm::ExecuteAlgorithm(QWidget *parent) 
	: QDialog(parent)
{
	m_parent = parent;
	
	label = new QLabel(tr("Select Raw File to Load:"));
	//~ lineFile = new QLineEdit;
	//~ lineFile->setReadOnly(true);
	//~ label->setBuddy(lineFile);
		
	exitButton = new QPushButton(tr("Exit"));
	connect(exitButton, SIGNAL(clicked()), this, SLOT(close()));
	
	QHBoxLayout *bottomRowLayout = new QHBoxLayout;
	bottomRowLayout->addStretch();
	bottomRowLayout->addWidget(exitButton);
	
	setLayout(bottomRowLayout);
	setWindowTitle(tr("ExecuteAlgorithm"));
	setFixedHeight(sizeHint().height());
}

ExecuteAlgorithm::~ExecuteAlgorithm()
{
	
}

void ExecuteAlgorithm::PassPythonInterface(Mantid::PythonAPI::PythonInterface*)
{

}

