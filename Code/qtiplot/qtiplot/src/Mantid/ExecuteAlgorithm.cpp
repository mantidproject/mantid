#include <vector>
#include <string>
#include <set>
#include <QMessageBox>
#include <QListWidgetItem>
#include <QComboBox>
#include <QStringList>
#include <QtGui>
#include <QObject>
#include <QPalette>
#include <QColor>

#include "ExecuteAlgorithm.h"
#include "../ApplicationWindow.h"
#include "../Matrix.h"
#include "LoadRawDlg.h"
#include "ImportWorkspaceDlg.h"
#include "MantidKernel/Property.h"

ExecuteAlgorithm::ExecuteAlgorithm(QWidget *parent) 
	: QDialog(parent)
{
	m_parent = parent;
}

ExecuteAlgorithm::~ExecuteAlgorithm()
{
	
}

void ExecuteAlgorithm::CreateLayout(Mantid::API::Algorithm* alg)
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
			
				connect(tempEdit, SIGNAL(editingFinished()), this, SLOT(textChanged()));
				connect(tempBtn, SIGNAL(clicked()), this, SLOT(browseClicked()));
			
				tempLbl->setBuddy(tempEdit);
				
				//Add validator
				QLabel *validLbl = new QLabel("*");
				QPalette pal = validLbl->palette();
				pal.setColor(QPalette::WindowText, Qt::darkRed);
				validLbl->setPalette(pal);
				validators[m_props[i]->name()] = validLbl;
		
				grid->addWidget(tempLbl, i, 0, 0);
				grid->addWidget(tempEdit, i, 1, 0);
				grid->addWidget(validLbl, i, 2, 0);
				grid->addWidget(tempBtn, i, 3, 0);
		
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
				
				//Add validator
				QLabel *validLbl = new QLabel("*");
				QPalette pal = validLbl->palette();
				pal.setColor(QPalette::WindowText, Qt::darkRed);
				validLbl->setPalette(pal);
				validators[m_props[i]->name()] = validLbl;
			
				grid->addWidget(tempLbl, i, 0, 0);
				grid->addWidget(tempCombo, i, 1, 0);
				grid->addWidget(validLbl, i, 2, 0);
			
				combos[tempCombo] = m_props[i]->name();
			}
			else
			{
				QLabel *tempLbl = new QLabel(QString::fromStdString(m_props[i]->name()));
				QLineEdit *tempEdit = new QLineEdit;
				tempLbl->setBuddy(tempEdit);
				
				//Add validator
				QLabel *validLbl = new QLabel("*");
				QPalette pal = validLbl->palette();
				pal.setColor(QPalette::WindowText, Qt::darkRed);
				validLbl->setPalette(pal);
				validators[m_props[i]->name()] = validLbl;
				
				connect(tempEdit, SIGNAL(editingFinished()), this, SLOT(textChanged()));
		
				grid->addWidget(tempLbl, i, 0, 0);
				grid->addWidget(tempEdit, i, 1, 0);
				grid->addWidget(validLbl, i, 2, 0);
		
				edits[tempEdit] = m_props[i]->name();
			}
		}
	}
	
	okButton = new QPushButton(tr("OK"));
	connect(okButton, SIGNAL(clicked()), this, SLOT(okClicked()));
	okButton->setDefault(true);
	//okButton->setEnabled(false);
	
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
	
	setPropertiesAndValidate();
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
	
	setPropertiesAndValidate();
}

void ExecuteAlgorithm::textChanged()
{
	//okButton->setEnabled(validateEntries());
	setPropertiesAndValidate();
}

bool ExecuteAlgorithm::setPropertiesAndValidate()
{
	bool propsOK = true;
	
	std::map<QLineEdit*, std::string>::iterator editItr = edits.begin();
	std::map<QComboBox*, std::string>::iterator comboItr = combos.begin();
		
	for (; editItr != edits.end(); ++editItr)
	{	
		std::string value = editItr->first->text().trimmed().toStdString();
		
		if (!setPropertyValue(editItr->second, value) || !validateProperty(editItr->second))
		{
			//Highlight that value is invalid
			showValidator(editItr->second);	
			propsOK = false;
		}
		else
		{			
			hideValidator(editItr->second);
		}
	}
	
	for (; comboItr != combos.end(); ++comboItr)
	{	
		std::string value = comboItr->first->currentText().trimmed().toStdString();
		
		//Only set a property if it is not nothing
		if (value != "")
		{
			if (!setPropertyValue(comboItr->second, value) || !validateProperty(comboItr->second))
			{
				//Highlight that value is invalid
				showValidator(comboItr->second);	
				propsOK = false;
			}
			else
			{
				hideValidator(comboItr->second);	
			}
		}
	}
	
	return propsOK;
}

bool ExecuteAlgorithm::setPropertyValue(const std::string& name, const std::string& value)
{
	try
	{
		//If the value enter is "" then use the default value if there is one
		//else flag it as a invalid entry.
		//Note: once the default value is overwritten it cannot be retrieved and
		//entering "" is invalid.
		if (value == "" )
		{
			std::vector< Mantid::Kernel::Property*>::const_iterator propItr = m_props.begin();
	
			for (; propItr != m_props.end(); ++propItr)
			{
				if ((*propItr)->name() == name)
				{
					return (*propItr)->isDefault();
				}
			}
		}
		
		m_alg->setPropertyValue(name, value);
		
		return true;
	}
	catch (std::invalid_argument err)
	{
		return false;
	}
}

bool ExecuteAlgorithm::validateProperty(const std::string& name)
{
	std::vector< Mantid::Kernel::Property*>::const_iterator propItr = m_props.begin();
	
	for (; propItr != m_props.end(); ++propItr)
	{
		if ((*propItr)->name() == name)
		{
			return (*propItr)->isValid();
		}
	}
	
	//Should never reach here
	return false;
}

void ExecuteAlgorithm::okClicked()
{
	if (execute())
	{	
		accept();
	}
}

bool ExecuteAlgorithm::execute()
{
	if (!setPropertiesAndValidate())
	{
		QMessageBox::warning(this, tr("Mantid Algorithm"),
			tr("At least one parameter entered is incorrect. "
			"Incorrect entries are marked with an asterisk."),
			QMessageBox::Ok);
		return false;
	}
				
	if (!m_alg->execute() == true)
	{
		//Algorithm did not execute properly
		QMessageBox::warning(this, tr("Mantid Algorithm"),
			tr("The algorithm failed to execute correctly. "
			"Please see the Mantid log for details."),
			QMessageBox::Ok);
		
		return false;
	}
	
	return true;
}

void ExecuteAlgorithm::showValidator(const std::string& propName)
{
	std::map<std::string, QLabel*>::iterator itr  = validators.find(propName);
	
	if (itr != validators.end())
	{
		itr->second->show();
	}
}

void ExecuteAlgorithm::hideValidator(const std::string& propName)
{
	std::map<std::string, QLabel*>::iterator itr  = validators.find(propName);
	
	if (itr != validators.end())
	{
		itr->second->hide();
	}
}

