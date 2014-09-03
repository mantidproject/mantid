//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidQtCustomDialogs/LoadDialog.h"
#include "MantidQtMantidWidgets/MWRunFiles.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
// Qt
#include <QCheckBox>
#include <QComboBox>
#include <QUrl>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QFileInfo>

// Mantid
#include "MantidKernel/Property.h"
#include "MantidKernel/MaskedProperty.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidQtAPI/HelpWindow.h"

namespace MantidQt
{
  namespace CustomDialogs
  {
    namespace
    {
      /// Holds a flag at a given value
      /// and flips it back to its starting value on destruction
      struct HoldFlag
      {
        HoldFlag(bool& current, const bool holdValue) : initial(current), heldflag(current)
        {
          heldflag = holdValue;
        }
        ~HoldFlag() { heldflag = initial; }
        bool initial;
        bool & heldflag;
      };
    }

    // Declare the dialog. Name must match the class name
    DECLARE_DIALOG(LoadDialog);

    //--------------------------------------------------------------------------
    // Public methods
    //---------------------------------------------------------------------------

    /// Default constructor
    LoadDialog:: LoadDialog(QWidget *parent)
      : API::AlgorithmDialog(parent), m_form(), m_currentFiles(), m_initialHeight(0),
        m_populating(false)
    {
      // We will handle parsing the input ourselves on startup
      m_autoParseOnInit = false;
    }

    //--------------------------------------------------------------------------
    // Private methods (slot)
    //---------------------------------------------------------------------------

    /**
    * Activated when the file has been changed
    */
    void LoadDialog::createDynamicWidgets()
    {
      HoldFlag hold(m_populating, true);

      m_form.fileWidget->blockSignals(true);
      createDynamicLayout();
      m_form.fileWidget->blockSignals(false);
    }

    /// Override the help button clicked method
    void LoadDialog::helpClicked()
    {
      const std::string & loaderName = getAlgorithm()->getPropertyValue("LoaderName");
      QString helpPage = (loaderName.empty()) ? QString("Load") : QString::fromStdString(loaderName);
      MantidQt::API::HelpWindow::showAlgorithm(this->nativeParentWidget(), helpPage);
    }

    /**
    * Suggest a workspace name from the file
    */
    void LoadDialog::suggestWSName()
    {
      if( !m_form.workspaceEdit->isEnabled() ) return;
      QString suggestion;
      if( m_form.fileWidget->isValid() )
      {
        if( m_form.fileWidget->getFilenames().size() == 1 )
          suggestion = QFileInfo(m_form.fileWidget->getFirstFilename()).baseName();
        else
          suggestion = "MultiFiles";
      }
      m_form.workspaceEdit->setText(suggestion);
    }

    /**
    * Connect/Disconnect the signal that updates the workspace name with a suggested value
    * @param on :: If true then a workspace name will be suggested
    */
    void LoadDialog::enableNameSuggestion(const bool on)
    {
      if( on )
      {
        connect(m_form.fileWidget, SIGNAL(filesFound()), this, SLOT(suggestWSName()));
      }
      else
      {
        disconnect(m_form.fileWidget, SIGNAL(filesFound()), this, SLOT(suggestWSName()));
      }
    }

    /**
     * Called when the run button is clicked
     */
    void LoadDialog::accept()
    {
      // The file widget may have been edited but not lost focus so that the search wasn't
      // attempted for the new contents. Force one here.
      // The widget does nothing if the contents have not changed so it will be quick for this case
      m_form.fileWidget->findFiles();
      while(m_form.fileWidget->isSearching() || m_populating)
      {
        QApplication::instance()->processEvents();
      }

      // Check that the file still exists just incase it somehow got removed
      std::string errMess = getAlgorithm()->getPointerToProperty("Filename")->isValid();
      if ( !errMess.empty() )
      {
        m_currentFiles = "";
        createDynamicWidgets();
        return;
      }
      AlgorithmDialog::accept();
    }


    //--------------------------------------------------------------------------
    // Private methods (non-slot)
    //---------------------------------------------------------------------------

    /// Initialize the layout
    void LoadDialog::initLayout()
    {
      m_form.setupUi(this);

      // Add the helpful summary message
      if(isMessageAvailable())
        m_form.instructions->setText(getOptionalMessage());

      m_form.dialogLayout->addLayout(this->createDefaultButtonLayout());
      m_form.fileWidget->readSettings("Mantid/Algorithms/Load");
      m_initialHeight = this->height();

      // Guess at an output workspace name but only if the user hasn't changed anything
      enableNameSuggestion(true);
      connect(m_form.workspaceEdit, SIGNAL(textEdited(const QString&)), this, SLOT(enableNameSuggestion()));
      // Connect the file finder's file found signal to the dynamic property create method.
      // When the file text is set the Load algorithm finds the concrete loader and then we
      // know what extra properties to create
      connect(m_form.fileWidget, SIGNAL(filesFound()), this, SLOT(createDynamicWidgets()));
      tieStaticWidgets(true);
    }

    /**
    * Save the input after OK is clicked
    */
    void LoadDialog::saveInput()
    {
      m_form.fileWidget->saveSettings("Mantid/Algorithms/Load");
      AlgorithmDialog::saveInput();
      //Ensure the filename is store as the full file
      API::AlgorithmInputHistory::Instance().storeNewValue("Load",
							   QPair<QString, QString>("Filename", m_currentFiles));
    }

    /**
    * Tie static widgets to their properties
    * @param readHistory :: If true then the history will be re read.
    */
    void LoadDialog::tieStaticWidgets(const bool readHistory)
    {
      // If a workspace validator asterisk exists, remove it since the underlying AlgorithmDialog gets confused
      if( m_form.workspaceLayout->count() == 3 )
      {
        QLayoutItem *validLbl = m_form.workspaceLayout->takeAt(2);
        delete validLbl->widget();
        delete validLbl;
      }
      tie(m_form.workspaceEdit, "OutputWorkspace", m_form.workspaceLayout, readHistory);
      tie(m_form.fileWidget, "Filename", NULL, readHistory);
    }

    /**
    * Clear the old widgets for a new Loader type
    * @param layout :: The layout containing the child layouts/widgets
    */
    void LoadDialog::removeOldInputWidgets(QVBoxLayout *layout)
    {
      // Remove the old widgets if necessary
      if( layout->count() > 2 )
      {
        int count = layout->count();
        while( count > 2 )
        {
          QLayoutItem *child = layout->takeAt(count - 1);
          if( QWidget *w = child->widget() )
          {
            w->deleteLater();
          }
          else if( QLayout *l = child->layout() )
          {
            QLayoutItem *subChild(NULL);
            while( (subChild = l->takeAt(0)) != NULL )
            {
              subChild->widget()->deleteLater();
            }
          }
          count = layout->count();
        }
      }
    }

    /**
    * Create the dynamic widgets for the concrete loader
    */
    void LoadDialog::createDynamicLayout()
    {
      // Disable the layout so that a widget cannot be interacted with while it may be being deleted
      m_form.propertyLayout->setEnabled(false);

      using namespace Mantid::API;
      using namespace Mantid::Kernel;

      if( !m_form.fileWidget->isValid() ) return;
      // First step is the get the specific loader that is responsible
      auto loadAlg = getAlgorithm();
      const QString filenames = m_form.fileWidget->getUserInput().asString();
      if( filenames == m_currentFiles ) return;
      m_currentFiles = filenames;
      removeOldInputWidgets(m_form.propertyLayout); // The new file might be invalid
      try
      {
        loadAlg->setPropertyValue("Filename", filenames.toStdString());
      }
      catch(std::exception & exc)
      {
        m_form.fileWidget->setFileProblem(QString::fromStdString(exc.what()));
        m_form.propertyLayout->setEnabled(true);
        m_form.propertyLayout->activate();
        this->resize(this->width(), m_initialHeight + 15);

        // Reset the algorithm pointer so that the base class re-reads the properties and drops links from
        // old widgets meaning they are safe to remove
        setAlgorithm(loadAlg);
        tieStaticWidgets(false); //The ties are cleared when resetting the algorithm

        return;
      }
      // Reset the algorithm pointer so that the base class re-reads the properties and drops links from
      // old widgets meaning they are safe to remove
      setAlgorithm(loadAlg);
      tieStaticWidgets(false); //The ties are cleared when resetting the algorithm
      // Add the new ones
      const std::vector<Property*> & inputProps = loadAlg->getProperties();
      int dialogHeight = m_initialHeight;
      for( size_t i = 0; i < inputProps.size(); ++i )
      {
        const Property* prop = inputProps[i];
        const QString propName = QString::fromStdString(prop->name());
        if( propName == "OutputWorkspace" || propName == "Filename" ) continue;
        if( requiresUserInput(propName) )
        {
          dialogHeight += createWidgetsForProperty(prop, m_form.propertyLayout, m_form.scrollAreaWidgetContents);
        }
      }
      // Re-enable and recompute the size of the layout
      m_form.propertyLayout->setEnabled(true);
      m_form.propertyLayout->activate();

      const int screenHeight = QApplication::desktop()->height();
      // If the thing won't end up too big compared to the screen height,
      // resize the scroll area so we don't get a scroll bar
      if ( dialogHeight < 0.8*screenHeight ) this->resize(this->width(),dialogHeight + 20);

      // Make sure the OutputWorkspace value has been stored so that the validator is cleared appropriately
      QString wsName(m_form.workspaceEdit->text());
      if(!wsName.isEmpty()) storePropertyValue("OutputWorkspace",wsName);
      setPropertyValues(QStringList("Filename"));
    }

    /**
    * Return a layout containing suitable widgets for the given property
    * @param prop A pointer to the algorithm property
    * @param propertyLayout A layout where the widgets are to be placed
    * @param parent The parent widget
    */
    int LoadDialog::createWidgetsForProperty(const Mantid::Kernel::Property* prop,
                                             QVBoxLayout *propertyLayout, QWidget *parent)
    {
      using namespace Mantid::API;
      using namespace Mantid::Kernel;
      using MantidQt::MantidWidgets::MWRunFiles;

      QString propName = QString::fromStdString(prop->name());
      QWidget *inputWidget(NULL);
      QHBoxLayout *widgetLayout(NULL);
      bool addValidator(true);

      // Boolean properties use the name labels differently
      if( const FileProperty* fileType = dynamic_cast<const FileProperty*>(prop) )
      {
        MWRunFiles *fileFinder = new MWRunFiles(parent);
        inputWidget = fileFinder;
        fileFinder->setLabelText(propName);
        fileFinder->isForRunFiles(false);
        fileFinder->isOptional(fileType->isOptional());
        fileFinder->doMultiEntry(false);
        addValidator = false;
        propertyLayout->addWidget(inputWidget);
      }
      else
      {
        QLabel *nameLbl = new QLabel(propName, parent);
        nameLbl->setToolTip(QString::fromStdString(prop->briefDocumentation()));
        if( dynamic_cast<const PropertyWithValue<bool>* >(prop) )
        {
          QCheckBox *checkBox = new QCheckBox(parent);
          inputWidget = checkBox;
          addValidator = false;
        }
        // Options box
        else if( !prop->allowedValues().empty() )
        {
          QComboBox *optionsBox = new QComboBox(parent);
          inputWidget = optionsBox;
          std::vector<std::string> items = prop->allowedValues();
          std::vector<std::string>::const_iterator vend = items.end();
          for(std::vector<std::string>::const_iterator vitr = items.begin(); vitr != vend;
            ++vitr)
          {
            optionsBox->addItem(QString::fromStdString(*vitr));
          }
          // Set current as visible
          int index = optionsBox->findText(QString::fromStdString(prop->value()));
          if( index >= 0 ) optionsBox->setCurrentIndex(index);

          addValidator = false;
        }
        // else render a text box
        else
        {
          QLineEdit *textBox = new QLineEdit(parent);
          inputWidget = textBox;
          if( dynamic_cast<const MaskedProperty<std::string> *>(prop) )
          {
            textBox->setEchoMode(QLineEdit::Password);
          }
        }
        nameLbl->setBuddy(inputWidget);
        widgetLayout = new QHBoxLayout();
        widgetLayout->addWidget(nameLbl);
        widgetLayout->addWidget(inputWidget);
        propertyLayout->addLayout(widgetLayout);
      }

      if( addValidator ) tie(inputWidget, propName, widgetLayout);
      else tie(inputWidget, propName, NULL);

      return inputWidget->geometry().height();
    }


  }
}
