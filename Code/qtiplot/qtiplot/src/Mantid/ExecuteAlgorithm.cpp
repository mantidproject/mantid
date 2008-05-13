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
}

ExecuteAlgorithm::~ExecuteAlgorithm()
{
	
}

void ExecuteAlgorithm::CreateLayout(std::vector<std::string>& properties)
{
	QGridLayout *grid = new QGridLayout(this);
	
	for (int i = 0; i < properties.size(); ++i)
	{
		QLabel *tempLbl = new QLabel(QString::fromStdString(properties[i]));
		QLineEdit *tempEdit = new QLineEdit;
		tempLbl->setBuddy(tempEdit);
		
		grid->addWidget(tempLbl, i, 0, 0);
		grid->addWidget(tempEdit, i, 1, 0);
		
		edits.append(tempEdit);
	}
	
	okButton = new QPushButton(tr("OK"));
	connect(okButton, SIGNAL(clicked()), this, SLOT(okClicked()));
	okButton->setDefault(true);
	
	exitButton = new QPushButton(tr("Cancel"));
	connect(exitButton, SIGNAL(clicked()), this, SLOT(close()));
	
	QHBoxLayout *buttonRowLayout = new QHBoxLayout;
	buttonRowLayout->addStretch();
	buttonRowLayout->addWidget(exitButton);
	buttonRowLayout->addWidget(okButton);
	
	grid->addLayout(buttonRowLayout,  properties.size() , 1, 0);
	
	setLayout(grid);
	
	setWindowTitle(tr("Enter Properties"));
	setFixedHeight(sizeHint().height());
}

void ExecuteAlgorithm::okClicked()
{
	for (int i = 0; i < edits.size(); ++i)
	{
		results.push_back(edits[i]->text().toStdString());
	}
	
	accept();
}

