#include "MantidQtCustomDialogs/CatalogPublishDialog.h"

namespace MantidQt
{
  namespace CustomDialogs
  {
    // Declare the dialog. Name must match the class name.
    DECLARE_DIALOG(CatalogPublishDialog);

    /**
     * Default constructor.
     * @param parent :: Parent dialog.
     */
    CatalogPublishDialog::CatalogPublishDialog(QWidget *parent) : MantidQt::API::AlgorithmDialog(parent) {}

    /// Destructor
    CatalogPublishDialog::~CatalogPublishDialog() {}

    /// Initialise the layout
    void CatalogPublishDialog::initLayout() {}
  }
}
