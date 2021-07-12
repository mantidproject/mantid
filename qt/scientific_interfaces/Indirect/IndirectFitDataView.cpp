// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitDataView.h"

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectFitDataView::IndirectFitDataView(QWidget *parent)
    : IIndirectFitDataView(parent), m_dataForm(new Ui::IndirectFitDataView) {
  m_dataForm->setupUi(this);

  connect(m_dataForm->pbAdd, SIGNAL(clicked()), this, SIGNAL(addClicked()));
  connect(m_dataForm->pbRemove, SIGNAL(clicked()), this, SIGNAL(removeClicked()));
}

QTableWidget *IndirectFitDataView::getDataTable() const { return m_dataForm->tbFitData; }

UserInputValidator &IndirectFitDataView::validate(UserInputValidator &validator) {
  if (m_dataForm->tbFitData->rowCount() == 0)
    validator.addErrorMessage("No input data has been provided.");
  return validator;
}

void IndirectFitDataView::displayWarning(const std::string &warning) {
  QMessageBox::warning(parentWidget(), "MantidPlot - Warning", QString::fromStdString(warning));
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
