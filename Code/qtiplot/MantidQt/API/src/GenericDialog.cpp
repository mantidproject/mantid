//----------------------------------
// Includes
//----------------------------------
#include "MantidQtAPI/GenericDialog.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"

#include "MantidKernel/PropertyWithValue.h"
#include "MantidAPI/IWorkspaceProperty.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QSignalMapper>
#include <QFileInfo>
#include <QDir>

// Dialog stuff is defined here
using namespace MantidQt::API;

//----------------------------------
// Public member functions
//----------------------------------
/**
 * Default Constructor
 */
GenericDialog::GenericDialog(QWidget* parent) : AlgorithmDialog(parent),m_inputGrid(NULL)
{
  m_signalMapper = new QSignalMapper(this);
}

/**
 * Destructor
 */
GenericDialog::~GenericDialog()
{
}

//----------------------------------
// Protected member functions
//----------------------------------
/**
 * Create the layout for this dialog.
 */
void GenericDialog::initLayout()
{
    //Put everything in a vertical box
    // Making the dialog a parent of the layout automatically sets mainLay as the top-level layout
    QVBoxLayout *mainLay = new QVBoxLayout(this);

    // We need m_inputGrid only if the algorithm has any properties
    if (getAlgorithm()->getProperties().size() != 0)
    {
        //Put the property boxes in a grid
        m_inputGrid = new QGridLayout;

        //See if we have any previous input for this algorithm
        QHash<QString, QString> oldValues;
        AlgorithmInputHistory::Instance().hasPreviousInput(QString::fromStdString(getAlgorithm()->name()), oldValues);

        //Each property is on its own row
        int row(-1);
        std::vector<Mantid::Kernel::Property*>::const_iterator pEnd = getAlgorithm()->getProperties().end();
        for ( std::vector<Mantid::Kernel::Property*>::const_iterator pIter = getAlgorithm()->getProperties().begin();
            pIter != pEnd; ++pIter )
        {
            Mantid::Kernel::Property* prop = *pIter;
            QString propName = QString::fromStdString(prop->name());
            // Only produce allow input for output properties or workspace properties
            if ( prop->direction() == Mantid::Kernel::Direction::Output &&
                !dynamic_cast<Mantid::API::IWorkspaceProperty*>(prop)) continue;

             ++row;

            // The name and valid label
            QLabel *nameLbl = new QLabel(propName);
            QLabel *validLbl = getValidatorMarker(propName);

            //check if there are only certain allowed values for the property
            bool fileType = (prop->getValidatorType() == "file");
            bool boolType(false);
            if( dynamic_cast<Mantid::Kernel::PropertyWithValue<bool>* >(prop) ) boolType = true;
            if ( boolType || (!prop->allowedValues().empty() && !fileType ) )
            {
              //It is a choice of certain allowed values and can use a combination box
              QComboBox *optionsBox = new QComboBox;
              nameLbl->setBuddy(optionsBox);

              QString selectedValue("");
              if( isForScript() || !oldValues.contains(propName) )
              {
                selectedValue = QString::fromStdString(prop->value());
              }
              else
              {
                selectedValue = oldValues[propName];
              }
              if ( boolType )
              {
                optionsBox->addItem("No");
                optionsBox->setItemData(0, 0);
                optionsBox->addItem("Yes");
                optionsBox->setItemData(1, 1);

                if( selectedValue == "0" || selectedValue == "No" ) optionsBox->setCurrentIndex(0);
                else optionsBox->setCurrentIndex(1);
              }
              else
              {
                std::vector<std::string> items = prop->allowedValues();
                std::vector<std::string>::const_iterator vend = items.end();

                int index(0);
                for(std::vector<std::string>::const_iterator vitr = items.begin(); vitr != vend; 
                  ++vitr, ++index)
                {
                  optionsBox->addItem(QString::fromStdString(*vitr));
                  optionsBox->setItemData(index, QString::fromStdString(*vitr));
                  if( QString::fromStdString(*vitr) == selectedValue ) optionsBox->setCurrentIndex(index);
                }
              }
              if( isForScript() && ( prop->isValid() == "" ) )
              {
                if ( !prop->isDefault() && !isValueSuggested(propName) ) 
                {
                  optionsBox->setEnabled(false);
                }
              }
              m_inputGrid->addWidget(nameLbl, row, 0, 0);
              m_inputGrid->addWidget(optionsBox, row, 1, 0);
              m_inputGrid->addWidget(validLbl, row, 2, 0);
              
              nameLbl->setToolTip(  QString::fromStdString(prop->documentation()) );
              optionsBox->setToolTip(  QString::fromStdString(prop->documentation()) );
            }
            else 
            {
              QLineEdit *textBox = new QLineEdit;
              nameLbl->setBuddy(textBox);
              m_editBoxes[textBox] = propName;

              setOldLineEditInput(propName, textBox);

              //Add the widgets to the grid
              m_inputGrid->addWidget(nameLbl, row, 0, 0);
              m_inputGrid->addWidget(textBox, row, 1, 0);
              m_inputGrid->addWidget(validLbl, row, 2, 0);

              nameLbl->setToolTip(  QString::fromStdString(prop->documentation()) );
              textBox->setToolTip(  QString::fromStdString(prop->documentation()) );

              if( fileType )
              {
                QPushButton *browseBtn = new QPushButton(tr("Browse"));
                connect(browseBtn, SIGNAL(clicked()), m_signalMapper, SLOT(map()));
                m_signalMapper->setMapping(browseBtn, textBox);

                m_inputGrid->addWidget(browseBtn, row, 3, 0);

                if( isForScript() && ( prop->isValid() == "") )
                {
                  if ( !prop->isDefault() && !isValueSuggested(propName) )
                  {
                    browseBtn->setEnabled(false);
                  }
                }
              }
            }//end combo box/dialog box decision

        }

        //Wire up the signal mapping object
        connect(m_signalMapper, SIGNAL(mapped(QWidget*)), this, SLOT(browseClicked(QWidget*)));

        if( isMessageAvailable() )
        {
            QLabel *inputMessage = new QLabel(this);
            inputMessage->setFrameStyle(QFrame::Panel | QFrame::Sunken);
            inputMessage->setText(getOptionalMessage());
            QHBoxLayout *msgArea = new QHBoxLayout;
            msgArea->addWidget(inputMessage);
            mainLay->addLayout(msgArea);
        }

        //The property boxes
        mainLay->addLayout(m_inputGrid);

    }

    m_okButton = new QPushButton(tr("Run"));
    connect(m_okButton, SIGNAL(clicked()), this, SLOT(accept()));
    m_okButton->setDefault(true);

    m_exitButton = new QPushButton(tr("Cancel"));
    connect(m_exitButton, SIGNAL(clicked()), this, SLOT(close()));

    QHBoxLayout *buttonRowLayout = new QHBoxLayout;
    buttonRowLayout->addStretch();
    buttonRowLayout->addWidget(m_okButton);
    buttonRowLayout->addWidget(m_exitButton);
    mainLay->addLayout(buttonRowLayout);

}

/**
* Parses the input from the box and adds the propery (name, value) pairs to 
* the map stored in the base class
*/
void GenericDialog::parseInput()
{
    if (!m_inputGrid) return; // algorithm dont have properties
    int nRows = m_inputGrid->rowCount();
    for( int row = 0; row < nRows; ++row )
    {
        QLabel *propName = static_cast<QLabel*>(m_inputGrid->itemAtPosition(row, 0)->widget());
        QWidget *buddy = propName->buddy();

        if( qobject_cast<QLineEdit*>(buddy) )
        {
            addPropertyValueToMap(propName->text(), qobject_cast<QLineEdit*>(buddy)->text());
        }
        else
        {
            QComboBox *box = qobject_cast<QComboBox*>(buddy);
            addPropertyValueToMap(propName->text(), box->itemData(box->currentIndex()).toString());
        }
    }
}

/**
 * This slot is called when a browse button is clicked
 * @param widget The widget that is associated with the button that was clicked. In this case they are always QLineEdit widgets
 */
void GenericDialog::browseClicked(QWidget* widget)
{
  //I mapped this to a QLineEdit, so cast it
  QLineEdit *pathBox = qobject_cast<QLineEdit*>(widget);
  
  QString propName("");
  if( m_editBoxes.contains(pathBox) ) propName = m_editBoxes[pathBox];
  else return;

  if( !pathBox->text().isEmpty() )
  {
    AlgorithmInputHistory::Instance().setPreviousDirectory(QFileInfo(pathBox->text()).absoluteDir().path());
  }  
  QString filepath = this->openLoadFileDialog(propName);
  if( !filepath.isEmpty() ) 
  {
    pathBox->clear();
    pathBox->setText(filepath.trimmed());
  }
}
