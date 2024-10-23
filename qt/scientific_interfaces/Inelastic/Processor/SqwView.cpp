// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "SqwView.h"
#include "MantidQtWidgets/Spectroscopy/DataValidationHelper.h"
#include "SqwPresenter.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <QDoubleValidator>
#include <QFileInfo>

using namespace DataValidationHelper;
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
SqwView::SqwView(QWidget *parent) : QWidget(parent), m_presenter() {
  m_uiForm.setupUi(parent);

  m_uiForm.rqwPlot2D->setCanvasColour(QColor(240, 240, 240));

  connect(m_uiForm.dsInput, &DataSelector::dataReady, this, &SqwView::notifyDataReady);
  connect(m_uiForm.spQLow, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this,
          &SqwView::notifyQLowChanged);
  connect(m_uiForm.spQWidth, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this,
          &SqwView::notifyQWidthChanged);
  connect(m_uiForm.spQHigh, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this,
          &SqwView::notifyQHighChanged);
  connect(m_uiForm.spELow, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this,
          &SqwView::notifyELowChanged);
  connect(m_uiForm.spEWidth, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this,
          &SqwView::notifyEWidthChanged);
  connect(m_uiForm.spEHigh, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this,
          &SqwView::notifyEHighChanged);
  connect(m_uiForm.ckRebinInEnergy, &QCheckBox::stateChanged, this, &SqwView::notifyRebinEChanged);
  connect(m_uiForm.pbSave, &QPushButton::clicked, this, &SqwView::notifySaveClicked);
  // Allows empty workspace selector when initially selected
  m_uiForm.dsInput->isOptional(true);

  // Disables searching for run files in the data archive
  m_uiForm.dsInput->isForRunFiles(false);
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
SqwView::~SqwView() {}

void SqwView::subscribePresenter(ISqwPresenter *presenter) { m_presenter = presenter; }

IRunView *SqwView::getRunView() const { return m_uiForm.runWidget; }

IOutputPlotOptionsView *SqwView::getPlotOptions() const { return m_uiForm.ipoPlotOptions; }

std::string SqwView::getDataName() const { return m_uiForm.dsInput->getCurrentDataName().toStdString(); }

void SqwView::setFBSuffixes(QStringList const &suffix) { m_uiForm.dsInput->setFBSuffixes(suffix); }

void SqwView::setWSSuffixes(QStringList const &suffix) { m_uiForm.dsInput->setWSSuffixes(suffix); }

void SqwView::setLoadHistory(bool doLoadHistory) { m_uiForm.dsInput->setLoadProperty("LoadHistory", doLoadHistory); }

bool SqwView::validate() {
  auto uiv = std::make_unique<UserInputValidator>();
  validateDataIsOfType(uiv.get(), m_uiForm.dsInput, "Sample", DataType::Red);

  auto const errorMessage = uiv->generateErrorMessage();
  if (!errorMessage.empty())
    showMessageBox(errorMessage);
  return errorMessage.empty();
}

void SqwView::notifyDataReady(QString const &dataName) { m_presenter->handleDataReady(dataName.toStdString()); }

void SqwView::notifyQLowChanged(double value) { m_presenter->handleQLowChanged(value); }

void SqwView::notifyQWidthChanged(double value) { m_presenter->handleQWidthChanged(value); }

void SqwView::notifyQHighChanged(double value) { m_presenter->handleQHighChanged(value); }

void SqwView::notifyELowChanged(double value) { m_presenter->handleELowChanged(value); }

void SqwView::notifyEWidthChanged(double value) { m_presenter->handleEWidthChanged(value); }

void SqwView::notifyEHighChanged(double value) { m_presenter->handleEHighChanged(value); }

void SqwView::notifyRebinEChanged(int value) { m_presenter->handleRebinEChanged(value); }

void SqwView::notifySaveClicked() { m_presenter->handleSaveClicked(); }

void SqwView::setEnableOutputOptions(bool const enable) {
  m_uiForm.ipoPlotOptions->setEnabled(enable);
  m_uiForm.pbSave->setEnabled(enable);
}

void SqwView::plotRqwContour(MatrixWorkspace_sptr rqwWorkspace) {
  m_uiForm.rqwPlot2D->clearPlot();
  m_uiForm.rqwPlot2D->setWorkspace(rqwWorkspace);
}

void SqwView::setDefaultQAndEnergy() {
  setQRange(m_uiForm.rqwPlot2D->getAxisRange(MantidWidgets::AxisID::YLeft));
  setEnergyRange(m_uiForm.rqwPlot2D->getAxisRange(MantidWidgets::AxisID::XBottom));
}

void SqwView::setQRange(std::tuple<double, double> const &axisRange) {
  auto const qRange = roundToWidth(axisRange, m_uiForm.spQWidth->value());
  m_uiForm.spQLow->setValue(qRange.first);
  m_uiForm.spQHigh->setValue(qRange.second);
}

void SqwView::setEnergyRange(std::tuple<double, double> const &axisRange) {
  auto const energyRange = roundToWidth(axisRange, m_uiForm.spEWidth->value());
  m_uiForm.spELow->setValue(energyRange.first);
  m_uiForm.spEHigh->setValue(energyRange.second);
}

std::tuple<double, double> SqwView::getQRangeFromPlot() const {
  return m_uiForm.rqwPlot2D->getAxisRange(MantidWidgets::AxisID::YLeft);
}

std::tuple<double, double> SqwView::getERangeFromPlot() const {
  return m_uiForm.rqwPlot2D->getAxisRange(MantidWidgets::AxisID::XBottom);
}

void SqwView::showMessageBox(std::string const &message) const {
  QMessageBox::information(parentWidget(), this->windowTitle(), QString::fromStdString(message));
}

} // namespace MantidQt::CustomInterfaces