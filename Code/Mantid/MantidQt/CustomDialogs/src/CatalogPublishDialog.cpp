#include "MantidQtCustomDialogs/CatalogPublishDialog.h"

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

      // Open a browsing dialog when the browse button is pressed.
      connect(m_uiForm.browseBtn,SIGNAL(clicked()),this,SLOT(onBrowse()));

      // Allows the combo box to show workspaces when they are loaded into Mantid.
      m_uiForm.inputWorkspaceCb->setValidatingAlgorithm(m_algName);

      // Add the automatically generated buttons (help, run, and cancel) to the ui.
      m_uiForm.buttonGrid->addLayout(createDefaultButtonLayout());

      // This allows the user NOT to select a workspace if there are any loaded into Mantid.
      m_uiForm.inputWorkspaceCb->insertItem("", 0);

  }
}
