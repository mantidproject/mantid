// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/MuonFunctionBrowser.h"
#include "MantidQtWidgets/Common/SelectFunctionDialog.h"

namespace MantidQt {
namespace MantidWidgets {

/**
 * Constructor
 * @param parent :: The parent widget.
 * @param multi  :: Option to use the browser for multi-dataset fitting.
 */
MuonFunctionBrowser::MuonFunctionBrowser(QWidget *parent, bool multi)
    : FunctionBrowser(parent, multi, {"Muon", "General", "Background"}) {}

/**
 * Destructor
 */
MuonFunctionBrowser::~MuonFunctionBrowser() {}

///**
// * Ask user to select a function and return it
// * @returns :: function string
// */
// QString MuonFunctionBrowser::getUserFunctionFromDialog() {
//  SelectFunctionDialog dlg(this, {"Muon", "General", "Background"});
//  if (dlg.exec() == QDialog::Accepted) {
//    return dlg.getFunction();
//  } else {
//    return QString();
//  }
//}

} // namespace MantidWidgets
} // namespace MantidQt
