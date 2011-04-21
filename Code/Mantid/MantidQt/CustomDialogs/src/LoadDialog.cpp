//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidQtCustomDialogs/LoadDialog.h"
#include "MantidQtMantidWidgets/MWRunFiles.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
// Qt
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QUrl>
#include <QDesktopServices>
#include <QFileInfo>
// Mantid
#include "MantidKernel/Property.h"
#include "MantidKernel/MaskedProperty.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/AlgorithmManager.h"

namespace MantidQt
{
  namespace CustomDialogs
  {
    // Declare the dialog. Name must match the class name
    DECLARE_DIALOG(LoadDialog);

    //--------------------------------------------------------------------------
    // Public methods
    //---------------------------------------------------------------------------

    /// Default constructor
    LoadDialog:: LoadDialog(QWidget *parent) 
      : API::AlgorithmDialog(parent), m_fileWidget(NULL), m_wkspaceWidget(NULL), 
      m_wkspaceLayout(NULL), m_dialogLayout(NULL), m_loaderLayout(NULL), 
      m_currentFile()       
    {
      QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
      setSizePolicy(sizePolicy);
    }

    //--------------------------------------------------------------------------
    // Private methods (slot)
    //---------------------------------------------------------------------------

    /**
    * Activated when the file has been changed
    */
    void LoadDialog::createDynamicWidgets()
    {
      m_fileWidget->blockSignals(true);
      if( m_loaderLayout ) m_loaderLayout->setEnabled(false);
      createDynamicLayout();
      if( m_loaderLayout ) m_loaderLayout->setEnabled(true);
      m_fileWidget->blockSignals(false);
    }

    /// Override the help button clicked method
    void LoadDialog::helpClicked()
    {
      const std::string & loaderName = getAlgorithm()->getPropertyValue("LoaderName");
      QString helpPage = (loaderName.empty()) ? QString("Load") : QString::fromStdString(loaderName);
      QDesktopServices::openUrl(QUrl(QString("http://www.mantidproject.org/") + helpPage));
    }

    /**
    * Suggest a workspace name from the file
    */
    void LoadDialog::suggestWSName()
    {
      QString suggestion;
      if( m_fileWidget->isValid() )
      {
        suggestion = QFileInfo(m_fileWidget->getFirstFilename()).baseName();	
      }
      m_wkspaceWidget->setText(suggestion);
    }

    /**
    * Connect/Disconnect the signal that updates the workspace name with a suggested value
    * @param on :: If true then a workspace name will be suggested
    */
    void LoadDialog::enableNameSuggestion(const bool on)
    {
      if( on )
      {
        connect(m_fileWidget, SIGNAL(filesFound()), this, SLOT(suggestWSName()));
      }
      else
      {
        disconnect(m_fileWidget, SIGNAL(filesFound()), this, SLOT(suggestWSName()));
      }
    }

    //--------------------------------------------------------------------------
    // Private methods (non-slot)
    //---------------------------------------------------------------------------

    /// Initialize the layout
    void LoadDialog::initLayout()
    {
      m_dialogLayout = new QVBoxLayout(this);
      createStaticWidgets(m_dialogLayout);
      m_dialogLayout->addLayout(createDefaultButtonLayout());

      // Now we have the static widgets in place, connect the file editing finished signal and
      // then tie the widget so that this will trigger the dynamic form generation if
      // there is any input history

      // Connect the file finder's file found signal to the dynamic property create method.
      // When the file text is set the Load algorithm finds the concrete loader and then we
      // know what extra properties to create
      connect(m_fileWidget, SIGNAL(filesFound()), this, SLOT(createDynamicWidgets()));
      tieStaticWidgets(true);
    }

    /**
    * Save the input after OK is clicked
    */
    void LoadDialog::saveInput()
    {
      m_fileWidget->saveSettings("Mantid/Algorithms/Load");
      AlgorithmDialog::saveInput();
      //Ensure the filename is store as the full file
      API::AlgorithmInputHistory::Instance().storeNewValue("Load", 
							   QPair<QString, QString>("Filename", m_currentFile));
    }

    /**
    * Create the widgets and layouts that are static, i.e do not depend on 
    * the specific load algorithm
    * @param layout The layout to hold the widgets
    */
    void LoadDialog::createStaticWidgets(QBoxLayout *widgetLayout)
    {
      QVBoxLayout *staticLayout = new QVBoxLayout();

      // Add the helpful summary message
      if( isMessageAvailable() )
      {
        this->addOptionalMessage(staticLayout);
      }

      // Filename widget
      m_fileWidget = new MantidWidgets::MWRunFiles(this);
      m_fileWidget->setLabelText("File");
      m_fileWidget->isForRunFiles(true);
      m_fileWidget->allowMultipleFiles(false);
      m_fileWidget->isOptional(false);
      m_fileWidget->doMultiEntry(false);
      m_fileWidget->setAlgorithmProperty("Load|Filename");
      m_fileWidget->extsAsSingleOption(false);
      m_fileWidget->readSettings("Mantid/Algorithms/Load");
      QHBoxLayout *propLine = new QHBoxLayout;
      propLine->addWidget(m_fileWidget);
      staticLayout->addLayout(propLine);

      // Workspace property
      m_wkspaceLayout = new QHBoxLayout;
      m_wkspaceLayout->addWidget(new QLabel("Workspace"));
      m_wkspaceWidget = new QLineEdit(this);
      m_wkspaceLayout->addWidget(m_wkspaceWidget);
      staticLayout->addLayout(m_wkspaceLayout);

      // Guess at an output workspace name but only if the user hasn't changed anything
      enableNameSuggestion(true);
      connect(m_wkspaceWidget, SIGNAL(textEdited(const QString&)), this, SLOT(enableNameSuggestion()));

      // Add to the static layout
      widgetLayout->addLayout(staticLayout);
    }

    /**
    * Tie static widgets to their properties
    * @param readHistory :: If true then the history will be re read.
    */
    void LoadDialog::tieStaticWidgets(const bool readHistory)
    {
      tie(m_wkspaceWidget, "OutputWorkspace", m_wkspaceLayout, readHistory);
      tie(m_fileWidget, "Filename", NULL, readHistory);
    }

    /**
    * Clear the old widgets for a new Loader type
    */
    void LoadDialog::setupDynamicLayout()
    {
      // Remove the old widgets if necessary
      if( m_loaderLayout )
      {
        m_dialogLayout->takeAt(1);
        QLayoutItem *child;
        while( (child = m_loaderLayout->takeAt(0)) != NULL ) 
        {
          // Mark the widget for deletion later as deleting it now can cause contention issues while
          // things are changing
          child->widget()->deleteLater();
          delete child;
        }
        m_loaderLayout->deleteLater();
        m_loaderLayout = NULL;
      }

      m_loaderLayout = new QGridLayout;
      m_dialogLayout->insertLayout(1, m_loaderLayout);
      // Remove the old workspace validator label as it's simpler to just create another one
      QLayoutItem *child = m_wkspaceLayout->takeAt(2);
      if( child )
      {
        delete child->widget();
        delete child;
      } 
    }

    /**
    * Create the dynamic widgets for the concrete loader
    */
    void LoadDialog::createDynamicLayout()
    {
      using namespace Mantid::API;
      using namespace Mantid::Kernel;

      if( !m_fileWidget->isValid() ) return;
      // First step is the get the specific loader that is reponsible
      IAlgorithm *loadAlg = getAlgorithm();
      const QString filename = m_fileWidget->getFirstFilename();
      if( filename == m_currentFile ) return;
      m_currentFile = filename;
      if( m_currentFile.isEmpty() ) return;
      try
      {
        loadAlg->setPropertyValue("Filename", filename.toStdString());
      }
      catch(std::exception & exc)
      {
        m_fileWidget->setFileProblem(QString::fromStdString(exc.what()));
        return;
      }
      // Reset the algorithm pointer so that the base class re-reads the properties and drops links from
      // old widgets meaning they are safe to remove
      this->setAlgorithm(loadAlg);

      setupDynamicLayout();
      tieStaticWidgets(false);
      const std::vector<Property*> & inputProps = loadAlg->getProperties();
      for( size_t i = 0; i < inputProps.size(); ++i )
      {
        const Property* prop = inputProps[i];
        const QString propName = QString::fromStdString(prop->name());
        if( propName == "OutputWorkspace" || propName == "Filename" ) continue;
        if( requiresUserInput(propName) )
        {
          createWidgetsForProperty(prop, m_loaderLayout);
        }
      }

      // Attempt to set any values that may have been retrieved
      setPropertyValues(QStringList("Filename"));
    }

    /**
    * Return a layout containing suitable widgets for the given property
    * @param prop A pointer to the algorithm property
    * @param loaderGrid A layout where the widgets are to be placed
    */
    QBoxLayout* LoadDialog::createWidgetsForProperty(const Mantid::Kernel::Property* prop,
      QGridLayout *loaderGrid)
    {
      using namespace Mantid::API;
      using namespace Mantid::Kernel;
      using MantidQt::MantidWidgets::MWRunFiles;

      QString propName = QString::fromStdString(prop->name());
      QWidget *inputWidget(NULL);
      bool addValidator(true);
      const int row = loaderGrid->rowCount();

      // Boolean properties use the name labels differently
      if( const FileProperty* fileType = dynamic_cast<const FileProperty*>(prop) )
      {
        MWRunFiles *fileFinder = new MWRunFiles();
        inputWidget = fileFinder;
        fileFinder->setLabelText(propName);
        fileFinder->isForRunFiles(false);
        fileFinder->isOptional(fileType->isOptional());
        fileFinder->doMultiEntry(false);
        // Ensure this spans the whole width of the grid
        loaderGrid->addWidget(inputWidget, row, 0, 1, 3);
        addValidator = false;
      }
      else
      {
        QLabel *nameLbl = new QLabel(propName);
        nameLbl->setToolTip(QString::fromStdString(prop->documentation()));
        if( dynamic_cast<const PropertyWithValue<bool>* >(prop) )
        {
          QCheckBox *checkBox = new QCheckBox();
          inputWidget = checkBox;
          addValidator = false;
        } 
        // Options box
        else if( !prop->allowedValues().empty() )
        {
          QComboBox *optionsBox = new QComboBox();
          inputWidget = optionsBox;
          std::set<std::string> items = prop->allowedValues();
          std::set<std::string>::const_iterator vend = items.end();
          for(std::set<std::string>::const_iterator vitr = items.begin(); vitr != vend; 
            ++vitr)
          {
            optionsBox->addItem(QString::fromStdString(*vitr));
          }
          addValidator = false;
        }
        // else render a text box
        else
        {
          QLineEdit *textBox = new QLineEdit();
          inputWidget = textBox;
          if( dynamic_cast<const MaskedProperty<std::string> *>(prop) )
          {
            textBox->setEchoMode(QLineEdit::Password);
          }
        }
        nameLbl->setBuddy(inputWidget);	
        loaderGrid->addWidget(nameLbl, row, 0);
        loaderGrid->addWidget(inputWidget, row, 1);
      }

      if( addValidator ) tie(inputWidget, propName, loaderGrid);
      else tie(inputWidget, propName, NULL);

      return NULL;
    }


  }
}
