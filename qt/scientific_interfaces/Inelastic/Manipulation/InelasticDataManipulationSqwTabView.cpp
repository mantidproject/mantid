// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "InelasticDataManipulationSqwTabView.h"
#include "IndirectDataValidationHelper.h"
#include "InelasticDataManipulationSqwTab.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <QDoubleValidator>
#include <QFileInfo>

using namespace IndirectDataValidationHelper;
using namespace Mantid::API;

namespace {
double roundToPrecision(double value, double precision) { return value - std::remainder(value, precision); }

std::pair<double, double> roundToWidth(std::tuple<double, double> const &axisRange, double width) {
  return std::make_pair(roundToPrecision(std::get<0>(axisRange), width) + width,
                        roundToPrecision(std::get<1>(axisRange), width) - width);
}
} // namespace

namespace MantidQt::CustomInterfaces {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
InelasticDataManipulationSqwTabView::InelasticDataManipulationSqwTabView(QWidget *parent) : m_presenter() {
  m_uiForm.setupUi(parent);

  m_uiForm.rqwPlot2D->setCanvasColour(QColor(240, 240, 240));

  connect(m_uiForm.dsInput, SIGNAL(dataReady(QString const &)), this, SLOT(notifyDataReady(QString const &)));
  connect(m_uiForm.spQLow, SIGNAL(valueChanged(double)), this, SLOT(notifyQLowChanged(double)));
  connect(m_uiForm.spQWidth, SIGNAL(valueChanged(double)), this, SLOT(notifyQWidthChanged(double)));
  connect(m_uiForm.spQHigh, SIGNAL(valueChanged(double)), this, SLOT(notifyQHighChanged(double)));
  connect(m_uiForm.spELow, SIGNAL(valueChanged(double)), this, SLOT(notifyELowChanged(double)));
  connect(m_uiForm.spEWidth, SIGNAL(valueChanged(double)), this, SLOT(notifyEWidthChanged(double)));
  connect(m_uiForm.spEHigh, SIGNAL(valueChanged(double)), this, SLOT(notifyEHighChanged(double)));
  connect(m_uiForm.ckRebinInEnergy, SIGNAL(stateChanged(int)), this, SLOT(notifyRebinEChanged(int)));

  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(notifyRunClicked()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(notifySaveClicked()));
  // Allows empty workspace selector when initially selected
  m_uiForm.dsInput->isOptional(true);

  // Disables searching for run files in the data archive
  m_uiForm.dsInput->isForRunFiles(false);
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
InelasticDataManipulationSqwTabView::~InelasticDataManipulationSqwTabView() {}

void InelasticDataManipulationSqwTabView::subscribePresenter(ISqwPresenter *presenter) { m_presenter = presenter; }

IndirectPlotOptionsView *InelasticDataManipulationSqwTabView::getPlotOptions() const { return m_uiForm.ipoPlotOptions; }

std::string InelasticDataManipulationSqwTabView::getDataName() const {
  return m_uiForm.dsInput->getCurrentDataName().toStdString();
}

void InelasticDataManipulationSqwTabView::setFBSuffixes(QStringList const &suffix) {
  m_uiForm.dsInput->setFBSuffixes(suffix);
}

void InelasticDataManipulationSqwTabView::setWSSuffixes(QStringList const &suffix) {
  m_uiForm.dsInput->setWSSuffixes(suffix);
}

bool InelasticDataManipulationSqwTabView::validate() {
  UserInputValidator uiv;
  validateDataIsOfType(uiv, m_uiForm.dsInput, "Sample", DataType::Red);

  auto const errorMessage = uiv.generateErrorMessage();
  if (!errorMessage.isEmpty())
    showMessageBox(errorMessage.toStdString());
  return errorMessage.isEmpty();
}

void InelasticDataManipulationSqwTabView::notifyDataReady(QString const &dataName) {
  m_presenter->handleDataReady(dataName.toStdString());
}

void InelasticDataManipulationSqwTabView::notifyQLowChanged(double value) { m_presenter->handleQLowChanged(value); }

void InelasticDataManipulationSqwTabView::notifyQWidthChanged(double value) { m_presenter->handleQWidthChanged(value); }

void InelasticDataManipulationSqwTabView::notifyQHighChanged(double value) { m_presenter->handleQHighChanged(value); }

void InelasticDataManipulationSqwTabView::notifyELowChanged(double value) { m_presenter->handleELowChanged(value); }

void InelasticDataManipulationSqwTabView::notifyEWidthChanged(double value) { m_presenter->handleEWidthChanged(value); }

void InelasticDataManipulationSqwTabView::notifyEHighChanged(double value) { m_presenter->handleEHighChanged(value); }

void InelasticDataManipulationSqwTabView::notifyRebinEChanged(int value) { m_presenter->handleRebinEChanged(value); }

void InelasticDataManipulationSqwTabView::notifyRunClicked() { m_presenter->handleRunClicked(); }

void InelasticDataManipulationSqwTabView::notifySaveClicked() { m_presenter->handleSaveClicked(); }

void InelasticDataManipulationSqwTabView::setRunButtonText(std::string const &runText) {
  m_uiForm.pbRun->setText(QString::fromStdString(runText));
  m_uiForm.pbRun->setEnabled(runText == "Run");
}

void InelasticDataManipulationSqwTabView::setEnableOutputOptions(bool const enable) {
  m_uiForm.ipoPlotOptions->setEnabled(enable);
  m_uiForm.pbSave->setEnabled(enable);
}

void InelasticDataManipulationSqwTabView::plotRqwContour(MatrixWorkspace_sptr rqwWorkspace) {
  m_uiForm.rqwPlot2D->setWorkspace(rqwWorkspace);
}

void InelasticDataManipulationSqwTabView::setDefaultQAndEnergy() {
  setQRange(m_uiForm.rqwPlot2D->getAxisRange(MantidWidgets::AxisID::YLeft));
  setEnergyRange(m_uiForm.rqwPlot2D->getAxisRange(MantidWidgets::AxisID::XBottom));
}

void InelasticDataManipulationSqwTabView::setQRange(std::tuple<double, double> const &axisRange) {
  auto const qRange = roundToWidth(axisRange, m_uiForm.spQWidth->value());
  m_uiForm.spQLow->setValue(qRange.first);
  m_uiForm.spQHigh->setValue(qRange.second);
}

void InelasticDataManipulationSqwTabView::setEnergyRange(std::tuple<double, double> const &axisRange) {
  auto const energyRange = roundToWidth(axisRange, m_uiForm.spEWidth->value());
  m_uiForm.spELow->setValue(energyRange.first);
  m_uiForm.spEHigh->setValue(energyRange.second);
}

std::tuple<double, double> InelasticDataManipulationSqwTabView::getQRangeFromPlot() const {
  return m_uiForm.rqwPlot2D->getAxisRange(MantidWidgets::AxisID::YLeft);
}

std::tuple<double, double> InelasticDataManipulationSqwTabView::getERangeFromPlot() const {
  return m_uiForm.rqwPlot2D->getAxisRange(MantidWidgets::AxisID::XBottom);
}

void InelasticDataManipulationSqwTabView::showMessageBox(std::string const &message) const {
  QMessageBox::information(parentWidget(), this->windowTitle(), QString::fromStdString(message));
}

} // namespace MantidQt::CustomInterfaces