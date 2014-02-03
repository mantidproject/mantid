#include "MantidQtCustomDialogs/CatalogPublishDialog.h"

#include "MantidAPI/CatalogFactory.h"
#include "MantidAPI/ICatalog.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"

#include <QDir>

namespace MantidQt
{
  namespace CustomDialogs
  {
    DECLARE_DIALOG(CatalogPublishDialog);

    /**
     * Default constructor.
     * @param parent :: Parent dialog.
     */
    CatalogPublishDialog::CatalogPublishDialog(QWidget *parent) : MantidQt::API::AlgorithmDialog(parent), m_uiForm() {}

    /// Destructor
    CatalogPublishDialog::~CatalogPublishDialog() {}

    /// Initialise the layout
    void CatalogPublishDialog::initLayout()
    {
      m_uiForm.setupUi(this);
      this->setWindowTitle(m_algName);

      tie(m_uiForm.fileNameTxt,"FileName");
      tie(m_uiForm.inputWorkspaceCb,"InputWorkspace");
      tie(m_uiForm.nameInCatalogTxt,"NameInCatalog");
      tie(m_uiForm.investigationNumberCb,"InvestigationNumber");
      tie(m_uiForm.descriptionInput,"DataFileDescription");

      // Allows the combo box to show workspaces when they are loaded into Mantid.
      m_uiForm.inputWorkspaceCb->setValidatingAlgorithm(m_algName);

      // This allows the user NOT to select a workspace if there are any loaded into Mantid.
      m_uiForm.inputWorkspaceCb->insertItem("", 0);

      // Open a browsing dialog when the browse button is pressed.
      connect(m_uiForm.browseBtn,SIGNAL(clicked()),this,SLOT(onBrowse()));

      // Assign the buttons with the inherited methods.
      connect(m_uiForm.runBtn,SIGNAL(clicked()),this,SLOT(accept()));
      connect(m_uiForm.cancelBtn,SIGNAL(clicked()),this,SLOT(reject()));
      connect(m_uiForm.helpBtn,SIGNAL(clicked()),this,SLOT(helpClicked()));

      // Populate "investigationNumberCb" with the investigation IDs that the user can publish to.
      populateUserInvestigations();

      // Get optional message here as we may set it if user has no investigations to publish to.
      m_uiForm.instructions->setText(getOptionalMessage());
    }

    /**
     * Populate the investigation number combo-box with investigations that the user can publish to.
     */
    void CatalogPublishDialog::populateUserInvestigations()
    {
      auto workspace = Mantid::API::WorkspaceFactory::Instance().createTable();
      std::string catalogName = Mantid::Kernel::ConfigService::Instance().getFacility().catalogInfo().catalogName();
      auto catalog = Mantid::API::CatalogFactory::Instance().create(catalogName);
      catalog->myData(workspace);

      // The user is not an investigator on any investigations and cannot publish
      // or they are not logged into the catalog then update the related message..
      if (workspace->rowCount() == 0)
      {
        setOptionalMessage("You cannot publish datafiles as you are not an investigator on any investigations or are not logged into the catalog.");
        // Disable the input fields and run button to prevent user from running algorithm.
        m_uiForm.scrollArea->setDisabled(true);
        m_uiForm.runBtn->setDisabled(true);
        return;
      }

      // Populate the form with investigations that the user can publish to.
      for (size_t row = 0; row < workspace->rowCount(); row++)
      {
        m_uiForm.investigationNumberCb->addItem(QString::fromStdString(workspace->cell<std::string>(row, 0)));
        // Add better tooltip for ease of use (much easier to recall the investigation if title and instrument are also provided).
        m_uiForm.investigationNumberCb->setItemData(static_cast<int>(row),
            QString::fromStdString("The title of the investigation is: \"" + workspace->cell<std::string>(row, 1) +
                                   "\".\nThe instrument of the investigation is: \"" + workspace->cell<std::string>(row, 2)) + "\".",
                                   Qt::ToolTipRole);
      }
    }

    /**
     * When the "browse" button is clicked open a file browser.
     */
    void CatalogPublishDialog::onBrowse()
    {
      if( !m_uiForm.fileNameTxt->text().isEmpty() )
      {
        QString lastdir = QFileInfo(m_uiForm.fileNameTxt->text()).absoluteDir().path();
        MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(lastdir);
      }

      QString filepath = this->openFileDialog("FileName"); //name of algorithm property.
      if( !filepath.isEmpty() )
      {
        m_uiForm.fileNameTxt->clear();
        m_uiForm.fileNameTxt->setText(filepath.trimmed());
      }
    }
  }
}
