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
#include "MantidAPI/FileProperty.h"
#include "InputHistory.h"

using Mantid::Kernel::PropertyWithValue;

ExecuteAlgorithm::ExecuteAlgorithm(QWidget *parent, bool forScript) 
  : QDialog(parent), m_forScript(forScript)
{
	m_parent = parent;
    m_directory = "";
}

ExecuteAlgorithm::~ExecuteAlgorithm()
{
}

void ExecuteAlgorithm::CreateLayout(Mantid::API::IAlgorithm_sptr alg, const QString & message)
{
  m_alg = alg;
  m_props = m_alg->getProperties();

  if ( m_props.empty() ) return;

  QGridLayout *grid = new QGridLayout;
  grid->setName("PropertyArea");
  int row(0);
  std::vector<Mantid::Kernel::Property*>::const_iterator pEnd = m_props.end();
  QString lastValue("");
  QMap<QString, QString> savedProps = InputHistory::Instance().algorithmProperties(QString::fromStdString(alg->name()));
  for ( std::vector<Mantid::Kernel::Property*>::const_iterator pIter = m_props.begin();
	pIter != pEnd; ++pIter, ++row )
    {
      Mantid::Kernel::Property* prop = *pIter;
      // If this is an output property (other than a workspace) then skip
      if ( prop->direction() == Mantid::Kernel::Direction::Output &&
	   !dynamic_cast<Mantid::API::IWorkspaceProperty*>(prop)) continue;
           
      QLabel *tempLbl = new QLabel(QString::fromStdString(prop->name()));
      //Add validator
      QLabel *validLbl = new QLabel("*");
      QPalette pal = validLbl->palette();
      pal.setColor(QPalette::WindowText, Qt::darkRed);
      validLbl->setPalette(pal);
      validators[prop->name()] = validLbl;
      
      if (dynamic_cast<Mantid::API::FileProperty*>(prop))
	{
	  QLineEdit *tempEdit = new QLineEdit;
	  QPushButton *tempBtn = new QPushButton(tr("Browse"));
	  
	  
	  if( !m_forScript ) lastValue = savedProps[QString::fromStdString(prop->name())];
	  else
	  {
	    if( prop->isDefault() ) lastValue = "";
	    else lastValue = QString::fromStdString(alg->getPropertyValue(prop->name()));
	  }

	  if (!lastValue.isEmpty())
	    {
	      tempEdit->setText(lastValue);
	      if( m_forScript ) {
		m_directory = "";
		tempEdit->setEnabled(false);
		tempBtn->setEnabled(false);
	      }
	      else m_directory = InputHistory::Instance().getDirectoryFromFilePath(lastValue);
	    }

	  connect(tempEdit, SIGNAL(editingFinished()), this, SLOT(textChanged()));
	  connect(tempBtn, SIGNAL(clicked()), this, SLOT(browseClicked()));
			
	  tempLbl->setBuddy(tempEdit);
				
	  grid->addWidget(tempLbl, row, 0, 0);
	  grid->addWidget(tempEdit, row, 1, 0);
	  grid->addWidget(validLbl, row, 2, 0);
	  grid->addWidget(tempBtn, row, 3, 0);
		
	  edits[tempEdit] = prop->name();
	  buttonsToEdits[tempBtn] = tempEdit;
	}
      else if ( !prop->allowedValues().empty() )
	{		
	  //If the property has allowed values then use a combo box.			
	  QComboBox *tempCombo = new QComboBox;
	  tempLbl->setBuddy(tempCombo);
			
	  QStringList list;

	  std::set<std::string> temp = prop->allowedValues();
	  std::set<std::string>::const_iterator vals_iter = temp.begin();
			
	  for (;  vals_iter != temp.end(); ++vals_iter)
	    {
	      list << QString::fromStdString(*vals_iter);
	    }
				
	  tempCombo->addItems(list);
	  
	  if( !m_forScript ) lastValue = savedProps[QString::fromStdString(prop->name())];
	  else lastValue = QString::fromStdString(alg->getPropertyValue(prop->name()));
	  
	  if (!lastValue.isEmpty()) 
	  {
	    int index = list.indexOf(lastValue);
	    if (index >= 0) tempCombo->setCurrentIndex(index);
	    if( m_forScript && !prop->isDefault() )
	    {
	      tempCombo->setEnabled(false);
	    }
	  }
				
	  grid->addWidget(tempLbl, row, 0, 0);
	  grid->addWidget(tempCombo, row, 1, 0);
	  grid->addWidget(validLbl, row, 2, 0);
			
	  combos[tempCombo] = prop->name();
	}
      else if (PropertyWithValue<bool> *p = dynamic_cast<PropertyWithValue<bool>* >(prop))
	{
	  // Add a true/false
	  QComboBox *tempCombo = new QComboBox;
	  tempLbl->setBuddy(tempCombo);
	  tempCombo->addItem("No");
	  tempCombo->addItem("Yes");
	  // Set to show default value
	  tempCombo->setCurrentIndex(*p);

	  grid->addWidget(tempLbl, row, 0, 0);
	  grid->addWidget(tempCombo, row, 1, 0);
	  grid->addWidget(validLbl, row, 2, 0);
			
	  combos[tempCombo] = prop->name();
	}
      else
	{
	  QLineEdit *tempEdit = new QLineEdit;
	  tempLbl->setBuddy(tempEdit);

	  if( !m_forScript ) lastValue = savedProps[QString::fromStdString(prop->name())];
	  else
	  {
	    if( prop->isDefault() ) lastValue = "";
	    else lastValue = QString::fromStdString(alg->getPropertyValue(prop->name()));
	  }
	  if (!lastValue.isEmpty()) 
	    {
	      tempEdit->setText(lastValue);
	      if( m_forScript ) tempEdit->setEnabled(false);
	    }
				
	  connect(tempEdit, SIGNAL(editingFinished()), this, SLOT(textChanged()));
		
	  grid->addWidget(tempLbl, row, 0, 0);
	  grid->addWidget(tempEdit, row, 1, 0);
	  grid->addWidget(validLbl, row, 2, 0);
		
	  edits[tempEdit] = prop->name();
	}
    }

	
  okButton = new QPushButton(tr("OK"));
  connect(okButton, SIGNAL(clicked()), this, SLOT(okClicked()));
  okButton->setDefault(true);
  //okButton->setEnabled(false);
	
  exitButton = new QPushButton(tr("Cancel"));
  connect(exitButton, SIGNAL(clicked()), this, SLOT(close()));
	
  QVBoxLayout *mainLay = new QVBoxLayout(this);
  mainLay->setName("MainArea");

  if( !message.isEmpty() )
  {
    QLabel *inputMessage = new QLabel(this);
    inputMessage->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    inputMessage->setText(QString("Script information: ") + message);
    QHBoxLayout *msgArea = new QHBoxLayout;
    msgArea->setName("InformationArea");
    msgArea->addWidget(inputMessage);
    mainLay->addLayout(msgArea);
  }
  
  mainLay->addLayout(grid);
	
  QHBoxLayout *buttonRowLayout = new QHBoxLayout;
  buttonRowLayout->setName("ButtonArea");
  buttonRowLayout->addStretch();
  buttonRowLayout->addWidget(exitButton);
  buttonRowLayout->addWidget(okButton);
	
  mainLay->addLayout(buttonRowLayout);
	
  setLayout(mainLay);
	
  setWindowTitle(tr("Enter properties - "+QString::fromStdString(alg->name())));
  setFixedHeight(sizeHint().height());
	
  //setPropertiesAndValidate();
  validateProperties();
}

void ExecuteAlgorithm::browseClicked()
{
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
	std::set<std::string> exts = (*itr)->allowedValues();
	QString allowed;
	
	if ( !exts.empty() )
	{
		allowed = "Files (";
		
		std::set<std::string>::iterator extItr = exts.begin();
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

	QString s( QFileDialog::getOpenFileName(this, tr("Select File"), m_directory, allowed));
	if ( s.isEmpty() )  return;
		
	temp->setText(s);
	
	validateProperties();
}

void ExecuteAlgorithm::textChanged()
{
	//okButton->setEnabled(validateEntries());
	//setPropertiesAndValidate();
	validateProperties();
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
		// Ugly way of turning boolean yes/no into the 1/0 required
		if ( value == "Yes" ) value = "1";
		if ( value == "No" ) value = "0";
		
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

bool ExecuteAlgorithm::validateProperties()
{
	bool propsOK = true;
	
	std::map<QLineEdit*, std::string>::iterator editItr = edits.begin();
	std::map<QComboBox*, std::string>::iterator comboItr = combos.begin();
		
	for (; editItr != edits.end(); ++editItr)
	{	
		std::string value = editItr->first->text().trimmed().toStdString();
		
		if (!validateProperty(editItr->second))
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
		// Ugly way of turning boolean yes/no into the 1/0 required
		if ( value == "Yes" ) value = "1";
		if ( value == "No" ) value = "0";
		
		//Only set a property if it is not nothing
		if (value != "")
		{
			if (!validateProperty(comboItr->second))
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
			return (*propItr)->isValid() == "";
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

