//---------------------------------------
// Includes
//---------------------------------------

#include "MantidQtWidgets/Common/MultifitSetupDialog.h"
#include "MantidQtWidgets/Common/FitPropertyBrowser.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/MatrixWorkspace.h"

//---------------------------------------
// Public member functions
//---------------------------------------

namespace MantidQt {
namespace MantidWidgets {

/// Constructor
/// @param fitBrowser
MultifitSetupDialog::MultifitSetupDialog(FitPropertyBrowser *fitBrowser)
    : QDialog(fitBrowser), m_fitBrowser(fitBrowser) {
  ui.setupUi(this);
  auto f = m_fitBrowser->compositeFunction()->getFunction(0);
  if (!f) {
    throw std::runtime_error(
        "IFitFunction expected but func function of another type");
  }
  QAbstractItemModel *model = ui.paramTable->model();
  for (size_t i = 0; i < f->nParams(); ++i) {
    int j = static_cast<int>(i);
    ui.paramTable->insertRow(ui.paramTable->rowCount());
    model->setData(model->index(j, 0),
                   QString::fromStdString(f->parameterName(i)));
    ui.paramTable->item(j, 0)->setFlags(nullptr);
    model->setData(model->index(j, 1), "");
    ui.paramTable->item(j, 1)->setCheckState(Qt::Unchecked);
  }
  ui.paramTable->resizeColumnToContents(0);
  connect(ui.paramTable, SIGNAL(cellChanged(int, int)), this,
          SLOT(cellChanged(int, int)));
}

/// Setup the function and close dialog
void MultifitSetupDialog::accept() {
  m_ties.clear();
  for (int i = 0; i < ui.paramTable->rowCount(); ++i) {
    if (ui.paramTable->item(i, 1)->checkState() == Qt::Checked) {
      m_ties << ui.paramTable->item(i, 1)->text();
    } else {
      m_ties << "";
    }
  }
  close();
}

void MultifitSetupDialog::cellChanged(int row, int col) {
  if (col == 1) {
    bool isChecked = ui.paramTable->item(row, col)->checkState() == Qt::Checked;
    QString text = ui.paramTable->item(row, col)->text();
    // toggle global/local
    if (isChecked && text.isEmpty()) {
      ui.paramTable->item(row, col)->setText(QString::fromStdString(
          m_fitBrowser->compositeFunction()->parameterName(row)));
    } else if (!isChecked) {
      ui.paramTable->item(row, col)->setText("");
    }
  }
}
} // namespace MantidWidgets
} // namespace MantidQt
