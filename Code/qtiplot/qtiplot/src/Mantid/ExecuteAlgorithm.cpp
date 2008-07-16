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

void ExecuteAlgorithm::CreateLayout(QStringList& workspaces, Mantid::API::Algorithm* alg)
{
	QGridLayout *grid = new QGridLayout();
	
	m_alg = alg;
	m_props = m_alg->getProperties();

	if (m_props.size() > 0)
	{
		for (int i = 0; i < m_props.size(); ++i)
		{
			if (m_props[i]->getValidatorType() == "file")
			{
				QLabel *tempLbl = new QLabel(QString::fromStdString(m_props[i]->name()));
				QLineEdit *tempEdit = new QLineEdit;
				QPushButton *tempBtn = new QPushButton(tr("Browse"));
			
				connect(tempBtn, SIGNAL(clicked()), this, SLOT(browseClicked()));
			
				tempLbl->setBuddy(tempEdit);
		
				grid->addWidget(tempLbl, i, 0, 0);
				grid->addWidget(tempEdit, i, 1, 0);
				grid->addWidget(tempBtn, i, 2, 0);
		
				edits[tempEdit] = m_props[i]->name();
				buttonsToEdits[tempBtn] = tempEdit;
			}
			else if (m_props[i]->allowedValues().size() > 0)
			{		
				//If the property has allowed values then use a combo box.			
				QLabel *tempLbl = new QLabel(QString::fromStdString(m_props[i]->name()));
				QComboBox *tempCombo = new QComboBox;
				tempLbl->setBuddy(tempCombo);
			
				QStringList list;

				std::vector<std::string> temp = m_props[i]->allowedValues();
				std::vector<std::string>::const_iterator vals_iter = temp.begin();
			
				for (;  vals_iter != temp.end(); ++vals_iter)
				{
					list << QString::fromStdString(*vals_iter);
				}
				
				tempCombo->addItems(list);
			
				grid->addWidget(tempLbl, i, 0, 0);
				grid->addWidget(tempCombo, i, 1, 0);
			
				combos[tempCombo] = m_props[i]->name();
			}
			else
			{
				QLabel *tempLbl = new QLabel(QString::fromStdString(m_props[i]->name()));
				QLineEdit *tempEdit = new QLineEdit;
				tempLbl->setBuddy(tempEdit);
		
				grid->addWidget(tempLbl, i, 0, 0);
				grid->addWidget(tempEdit, i, 1, 0);
		
				edits[tempEdit] = m_props[i]->name();
			}
		}
	}
	
	okButton = new QPushButton(tr("OK"));
	connect(okButton, SIGNAL(clicked()), this, SLOT(okClicked()));
	okButton->setDefault(true);
	
	exitButton = new QPushButton(tr("Cancel"));
	connect(exitButton, SIGNAL(clicked()), this, SLOT(close()));
	
	QVBoxLayout *mainLay = new QVBoxLayout(this);
	mainLay->addLayout(grid);
	
	QHBoxLayout *buttonRowLayout = new QHBoxLayout;
	buttonRowLayout->addStretch();
	buttonRowLayout->addWidget(exitButton);
	buttonRowLayout->addWidget(okButton);
	
	mainLay->addLayout(buttonRowLayout);
	
	setLayout(mainLay);
	
	setWindowTitle(tr("Enter properties"));
	setFixedHeight(sizeHint().height());
}

void ExecuteAlgorithm::browseClicked()
{
	static QString curDir = "";
	
	//Get the line edit associated with the button
	QLineEdit *temp = buttonsToEdits[qobject_cast<QPushButton*>(sender())];
	
	//Get the name of the associate property
	std::string propName = edits[temp];
	
	//Get the property
	std::vector<Mantid::Kernel::Property*>::iterator itr = m_props.begin();
	
	for (; itr != m_props.end(); ++itr)
	{	
		//itr is a pointer to a pointer so it needs to be dereferenced first
		if ((*itr)->name() == propName) break;
	}
	
	//Get the allowed file extensions
	std::vector<std::string> exts = (*itr)->allowedValues();
	QString allowed;
	
	if (exts.size() > 0)
	{
		allowed = "Files (";
		
		std::vector<std::string>::iterator extItr = exts.begin();
		for (; extItr != exts.end(); ++extItr)
		{
			allowed.append("*.");
			allowed.append(QString::fromStdString(*extItr));
			allowed.append(" ");
		}
		
		allowed.trimmed();
		
		allowed.append(QString::fromStdString(")"));
		
	}
	else
	{
		allowed = "All Files (*.*)";
	}

	QString s( QFileDialog::getOpenFileName(this, tr("Select File"), curDir, allowed));
	if ( s.isEmpty() )  return;
		
	temp->setText(s);
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
	
	if (execute())
	{	
		accept();
	}
}

bool ExecuteAlgorithm::execute()
{
	std::map<std::string, std::string>::iterator resItr = results.begin();
				
	for (; resItr != results.end(); ++resItr)
	{				
		try
		{
			m_alg->setPropertyValue(resItr->first, resItr->second);
		}
		catch (std::invalid_argument err)
		{
			int ret = QMessageBox::warning(this, tr("Mantid Algorithm"),
					tr(QString::fromStdString(resItr->first) + " was invalid."),
					QMessageBox::Ok);
				
			return false;
		}
	}
				
	//Check properties valid
	if (!m_alg->validateProperties())
	{
		//Properties not valid
		int ret = QMessageBox::warning(this, tr("Mantid Algorithm"),
			tr("One or more of the property values entered was invalid. "
			"Please see the Mantid log for details."),
			QMessageBox::Ok);
					
		return false;
	}
				
	if (!m_alg->execute() == true)
	{
		//Algorithm did not execute properly
		int ret = QMessageBox::warning(this, tr("Mantid Algorithm"),
			tr("The algorithm failed to execute correctly. "
			"Please see the Mantid log for details."),
			QMessageBox::Ok);
		
		return false;
	}
	
	return true;
}

