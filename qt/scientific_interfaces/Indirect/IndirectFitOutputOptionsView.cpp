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

void IndirectFitOutputOptionsView::setAsPlotting(bool plotting) {
  setButtonText(m_outputOptions->pbPlot, plotting ? "Plotting..." : "Plot");
  setButtonsEnabled(!plotting);
}

void IndirectFitOutputOptionsView::setAvailablePlotParameters(
    std::vector<std::string> const &parameterNames) {
  m_outputOptions->cbPlotType->addItem("All");
  for (auto const &name : parameterNames)
    m_outputOptions->cbPlotType->addItem(QString::fromStdString(name));
}

void IndirectFitOutputOptionsView::setSelectedParameterIndex(int index) {
  m_outputOptions->cbPlotType->setCurrentIndex(index);
}

void IndirectFitOutputOptionsView::clearPlotParameters() {
  m_outputOptions->cbPlotType->clear();
}

void IndirectFitOutputOptionsView::setButtonText(QPushButton *button,
                                                 QString const &text) {
  button->setText(text);
}

void IndirectFitOutputOptionsView::setButtonsEnabled(bool enable) {
  setPlotEnabled(enable);
  setSaveEnabled(enable);
}

void IndirectFitOutputOptionsView::setPlotEnabled(bool enable) {
  m_outputOptions->pbPlot->setEnabled(enable);
  m_outputOptions->cbPlotType->setEnabled(enable);
}

void IndirectFitOutputOptionsView::setSaveEnabled(bool enable) {
  m_outputOptions->pbSave->setEnabled(enable);
}

std::string IndirectFitOutputOptionsView::getPlotType() const {
  return m_outputOptions->cbPlotType->currentText().toStdString();
}

void IndirectFitOutputOptionsView::displayWarning(std::string const &message) {
  QMessageBox::warning(parentWidget(), "MantidPlot - Warning",
                       QString::fromStdString(message));
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
