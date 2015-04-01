#include "MantidQtCustomDialogs/CatalogPublishDialog.h"

#include "MantidAPI/CatalogFactory.h"
#include "MantidAPI/CatalogManager.h"
#include "MantidAPI/ICatalog.h"
#include "MantidAPI/ICatalogInfoService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidQtMantidWidgets/DataSelector.h"

namespace MantidQt
{
  namespace CustomDialogs
  {
    DECLARE_DIALOG(CatalogPublishDialog)

    /**
     * Default constructor.
     * @param parent :: Parent dialog.
     */
    CatalogPublishDialog::CatalogPublishDialog(QWidget *parent) : API::AlgorithmDialog(parent), m_uiForm() {}

    /// Initialise the layout
    void CatalogPublishDialog::initLayout()
    {
      m_uiForm.setupUi(this);
      this->setWindowTitle(m_algName);

      tie(m_uiForm.nameInCatalogTxt,"NameInCatalog");
      tie(m_uiForm.investigationNumberCb,"InvestigationNumber");
      tie(m_uiForm.descriptionInput,"DataFileDescription");

      // Assign the buttons with the inherited methods.
      connect(m_uiForm.runBtn,SIGNAL(clicked()),this,SLOT(accept()));
      connect(m_uiForm.cancelBtn,SIGNAL(clicked()),this,SLOT(reject()));
      connect(m_uiForm.helpBtn,SIGNAL(clicked()),this,SLOT(helpClicked()));
      connect(m_uiForm.investigationNumberCb,SIGNAL(currentIndexChanged(int)),this,SLOT(setSessionProperty(int)));
      connect(m_uiForm.dataSelector,SIGNAL(dataReady(const QString&)),this,SLOT(workspaceSelected(const QString&)));
      // When a file is chosen to be published, set the related "FileName" property of the algorithm.
      connect(m_uiForm.dataSelector,SIGNAL(filesFound()),this,SLOT(fileSelected()));

      // Populate "investigationNumberCb" with the investigation IDs that the user can publish to.
      populateUserInvestigations();

      // Get optional message here as we may set it if user has no investigations to publish to.
      m_uiForm.instructions->setText(getOptionalMessage());
      // This is required as we use the currentIndexChanged SLOT.
      storePropertyValue("Session",m_uiForm.investigationNumberCb->itemData(0,Qt::UserRole).toString());
    }

    /**
     * Populate the investigation number combo-box with investigations that the user can publish to.
     */
    void CatalogPublishDialog::populateUserInvestigations()
    {
      auto workspace = Mantid::API::WorkspaceFactory::Instance().createTable();
      auto session = Mantid::API::CatalogManager::Instance().getActiveSessions();

      // We need to catch the exception to prevent a fatal error.
      try
      {
        if (!session.empty())
        {
          // Cast a catalog to a catalogInfoService to access downloading functionality.
          auto catalogInfoService = boost::dynamic_pointer_cast<Mantid::API::ICatalogInfoService>(
              Mantid::API::CatalogManager::Instance().getCatalog(session.front()->getSessionId()));
          // Check if the catalog created supports publishing functionality.
          if (!catalogInfoService) throw std::runtime_error("The catalog that you are using does not support publishing.");
          // Populate the workspace with investigations that the user has CREATE access to.
          workspace = catalogInfoService->getPublishInvestigations();
        }
      }
      catch(std::runtime_error& e)
      {
        setOptionalMessage(e.what());
      }

      if (workspace->rowCount() > 0)
      {
        // Populate the form with investigations that the user can publish to.
        for (size_t row = 0; row < workspace->rowCount(); row++)
        {
          m_uiForm.investigationNumberCb->addItem(QString::fromStdString(workspace->getRef<std::string>("InvestigationID",row)));
          // Added tooltips to improve usability.
          m_uiForm.investigationNumberCb->setItemData(static_cast<int>(row),
            QString::fromStdString("The title of the investigation is: \"" + workspace->getRef<std::string>("Title",row) +
              "\".\nThe instrument of the investigation is: \"" + workspace->getRef<std::string>("Instrument",row)) + "\".",
              Qt::ToolTipRole);
          // Set the user role to the sessionID.
          m_uiForm.investigationNumberCb->setItemData(static_cast<int>(row),
            QString::fromStdString(workspace->getRef<std::string>("SessionID",row)),Qt::UserRole);
        }
      }
      else
      {
        disableDialog();
      }
    }

    /**
     * Obtain the name of the workspace selected, and set it to the algorithm's property.
     * @param wsName :: The name of the workspace to publish.
     */
    void CatalogPublishDialog::workspaceSelected(const QString& wsName)
    {
      // Prevents both a file and workspace being published at same time.
      storePropertyValue("FileName", "");
      setPropertyValue("FileName", true);
      // Set the workspace property to the one the user has selected to publish.
      storePropertyValue("InputWorkspace", wsName);
      setPropertyValue("InputWorkspace", true);
    }

    /**
     * Set the "FileName" property when a file is selected from the file browser.
     */
    void CatalogPublishDialog::fileSelected()
    {
      // Reset workspace property as the input is a file. This prevents both being selected.
      storePropertyValue("InputWorkspace", "");
      setPropertyValue("InputWorkspace", true);
      // Set the FileName property to the path that appears in the input field on the dialog.
      storePropertyValue("FileName", m_uiForm.dataSelector->getFullFilePath());
      setPropertyValue("FileName", true);
    }

    /**
     * Diables fields on dialog to improve usability
     */
    void CatalogPublishDialog::disableDialog()
    {
      m_uiForm.scrollArea->setDisabled(true);
      m_uiForm.runBtn->setDisabled(true);
    }

    /**
     * Set/Update the sessionID of the `Session` property when
     * the user selects an investigation from the combo-box.
     */
    void CatalogPublishDialog::setSessionProperty(int index)
    {
      storePropertyValue("Session",
          m_uiForm.investigationNumberCb->itemData(index,Qt::UserRole).toString());
    }

    /**
     * Overridden to enable dataselector validators.
     */
    void CatalogPublishDialog::accept()
    {
      if (!m_uiForm.dataSelector->isValid())
      {
        if (m_uiForm.dataSelector->getFullFilePath().isEmpty())
        {
          QMessageBox::critical(this,"Error in catalog publishing.","No file specified.");
        }
        else
        {
          QMessageBox::critical(this,"Error in catalog publishing.",m_uiForm.dataSelector->getProblem());
        }
      }
      else
      {
        AlgorithmDialog::accept();
      }
    }

  }
}
