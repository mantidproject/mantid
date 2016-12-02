#include "MantidQtMantidWidgets/MuonFunctionBrowser.h"
#include "MantidQtMantidWidgets/SelectFunctionDialog.h"

namespace MantidQt {
namespace MantidWidgets {

/**
 * Constructor
 * @param parent :: The parent widget.
 * @param multi  :: Option to use the browser for multi-dataset fitting.
 */
MuonFunctionBrowser::MuonFunctionBrowser(QWidget *parent, bool multi)
    : FunctionBrowser(parent, multi) {}

/**
 * Destructor
 */
MuonFunctionBrowser::~MuonFunctionBrowser() {}

/**
 * Ask user to select a function and return it
 * @returns :: function string
 */
QString MuonFunctionBrowser::getUserFunctionFromDialog() {
  SelectFunctionDialog dlg(this, {"Muon", "General", "Background"});
  if (dlg.exec() == QDialog::Accepted) {
    return dlg.getFunction();
  } else {
    return QString();
  }
}

} // namespace MantidWidgets
} // namespace MantidQt
