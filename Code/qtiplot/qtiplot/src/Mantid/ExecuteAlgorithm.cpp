#include <vector>
#include <string>
#include <set>
#include <QMessageBox>
#include <QListWidgetItem>
#include <QComboBox>
#include <QStringList>
#include <QtGui>
#include <QObject>

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

void ExecuteAlgorithm::CreateLayout(QStringList& workspaces, std::vector<Mantid::Kernel::Property*>& properties)
{
	QGridLayout *grid = new QGridLayout(this);
	
	for (int i = 0; i < properties.size(); ++i)
	{

		if (properties[i]->allowedValues().size() > 0)
		{		
			//If the property has allowed values then use a combo box.			
			QLabel *tempLbl = new QLabel(QString::fromStdString(properties[i]->name()));
			QComboBox *tempCombo = new QComboBox;
			tempLbl->setBuddy(tempCombo);
			
			QStringList list;

			std::vector<std::string> temp = properties[i]->allowedValues();
			std::vector<std::string>::const_iterator vals_iter = temp.begin();
			
			for (;  vals_iter != temp.end(); ++vals_iter)
			{
				list << QString::fromStdString(*vals_iter);
			}
			
			tempCombo->addItems(list);
			
			grid->addWidget(tempLbl, i, 0, 0);
			grid->addWidget(tempCombo, i, 1, 0);
			
			combos[tempCombo] = properties[i]->name();
		}
		else
		{
			QLabel *tempLbl = new QLabel(QString::fromStdString(properties[i]->name()));
			QLineEdit *tempEdit = new QLineEdit;
			tempLbl->setBuddy(tempEdit);
		
			grid->addWidget(tempLbl, i, 0, 0);
			grid->addWidget(tempEdit, i, 1, 0);
		
			edits[tempEdit] = properties[i]->name();
		}
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
	std::map<QLineEdit*, std::string>::iterator editItr = edits.begin();
	std::map<QComboBox*, std::string>::iterator comboItr = combos.begin();
	
	
	for (; editItr != edits.end(); ++editItr)
	{	
		std::string value = editItr->first->text().trimmed().toStdString();
		
		//Only set a property if it is not nothing
		if (value != "")
		{
			results[editItr->second] = value;
		}
	}
	
	for (; comboItr != combos.end(); ++comboItr)
	{	
		std::string value = comboItr->first->currentText().trimmed().toStdString();
		
		//Only set a property if it is not nothing
		if (value != "")
		{
			results[comboItr->second] = value;
		}
	}
	accept();
}

