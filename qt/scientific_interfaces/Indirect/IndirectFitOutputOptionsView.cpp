// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitOutputOptionsView.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectFitOutputOptionsView::IndirectFitOutputOptionsView(QWidget *parent)
    : API::MantidWidget(parent),
      m_outputOptions(new Ui::IndirectFitOutputOptions) {
  m_outputOptions->setupUi(this);

  connect(m_outputOptions->pbPlot, SIGNAL(clicked()), this,
          SLOT(emitPlotClicked()));
  connect(m_outputOptions->pbSave, SIGNAL(clicked()), this,
          SLOT(emitSaveClicked()));
}

IndirectFitOutputOptionsView::~IndirectFitOutputOptionsView() {}

void IndirectFitOutputOptionsView::emitPlotClicked() { emit plotClicked(); }

void IndirectFitOutputOptionsView::emitSaveClicked() { emit saveClicked(); }

void IndirectFitOutputOptionsView::clearPlotTypes() {
  m_outputOptions->cbPlotType->clear();
}

void IndirectFitOutputOptionsView::setAvailablePlotTypes(
    std::vector<std::string> const &parameterNames) {
  m_outputOptions->cbPlotType->addItem("All");
  for (auto const &name : parameterNames)
    m_outputOptions->cbPlotType->addItem(QString::fromStdString(name));
}

void IndirectFitOutputOptionsView::setPlotTypeIndex(int index) {
  m_outputOptions->cbPlotType->setCurrentIndex(index);
}

std::string IndirectFitOutputOptionsView::getSelectedPlotType() const {
  return m_outputOptions->cbPlotType->currentText().toStdString();
}

void IndirectFitOutputOptionsView::setPlotText(QString const &text) {
  m_outputOptions->pbPlot->setText(text);
}

void IndirectFitOutputOptionsView::setSaveText(QString const &text) {
  m_outputOptions->pbSave->setText(text);
}

void IndirectFitOutputOptionsView::setPlotEnabled(bool enable) {
  m_outputOptions->pbPlot->setEnabled(enable);
  m_outputOptions->cbPlotType->setEnabled(enable);
}

void IndirectFitOutputOptionsView::setSaveEnabled(bool enable) {
  m_outputOptions->pbSave->setEnabled(enable);
}

void IndirectFitOutputOptionsView::displayWarning(std::string const &message) {
  QMessageBox::warning(parentWidget(), "MantidPlot - Warning",
                       QString::fromStdString(message));
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
