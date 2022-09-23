// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectSqwView.h"
#include "IndirectDataValidationHelper.h"

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
IndirectSqwView::IndirectSqwView(QWidget *parent) {
  m_uiForm.setupUi(parent);
  m_dblManager = new QtDoublePropertyManager();
  m_dblEdFac = new DoubleEditorFactory(this);

  m_uiForm.rqwPlot2D->setCanvasColour(QColor(240, 240, 240));

  connect(m_uiForm.dsInput, SIGNAL(dataReady(QString const &)), this, SIGNAL(dataReady(QString const &)));
  connect(m_uiForm.spQLow, SIGNAL(valueChanged(double)), this, SIGNAL(qLowChanged(double)));
  connect(m_uiForm.spQWidth, SIGNAL(valueChanged(double)), this, SIGNAL(qWidthChanged(double)));
  connect(m_uiForm.spQHigh, SIGNAL(valueChanged(double)), this, SIGNAL(qHighChanged(double)));
  connect(m_uiForm.spELow, SIGNAL(valueChanged(double)), this, SIGNAL(eLowChanged(double)));
  connect(m_uiForm.spEWidth, SIGNAL(valueChanged(double)), this, SIGNAL(eWidthChanged(double)));
  connect(m_uiForm.spEHigh, SIGNAL(valueChanged(double)), this, SIGNAL(eHighChanged(double)));
  connect(m_uiForm.ckRebinInEnergy, SIGNAL(stateChanged(int)), this, SIGNAL(rebinEChanged(int)));

  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SIGNAL(runClicked()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SIGNAL(saveClicked()));

  // Allows empty workspace selector when initially selected
  m_uiForm.dsInput->isOptional(true);

  // Disables searching for run files in the data archive
  m_uiForm.dsInput->isForRunFiles(false);
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
IndirectSqwView::~IndirectSqwView() {}

IndirectPlotOptionsView *IndirectSqwView::getPlotOptions() { return m_uiForm.ipoPlotOptions; }

std::string IndirectSqwView::getDataName() { return m_uiForm.dsInput->getCurrentDataName().toStdString(); }

void IndirectSqwView::setFBSuffixes(QStringList suffix) { m_uiForm.dsInput->setFBSuffixes(suffix); }

void IndirectSqwView::setWSSuffixes(QStringList suffix) { m_uiForm.dsInput->setWSSuffixes(suffix); }

bool IndirectSqwView::validate() {
  UserInputValidator uiv;
  validateDataIsOfType(uiv, m_uiForm.dsInput, "Sample", DataType::Red);

  auto const errorMessage = uiv.generateErrorMessage();
  if (!errorMessage.isEmpty())
    showMessageBox(errorMessage);
  return errorMessage.isEmpty();
}

void IndirectSqwView::updateRunButton(bool enabled, std::string const &enableOutputButtons, QString const &message,
                                      QString const &tooltip) {
  setRunEnabled(enabled);
  m_uiForm.pbRun->setText(message);
  m_uiForm.pbRun->setToolTip(tooltip);
  if (enableOutputButtons != "unchanged")
    setSaveEnabled(enableOutputButtons == "enable");
}

void IndirectSqwView::setRunEnabled(bool enabled) { m_uiForm.pbRun->setEnabled(enabled); }

void IndirectSqwView::setSaveEnabled(bool enabled) { m_uiForm.pbSave->setEnabled(enabled); }

void IndirectSqwView::plotRqwContour(MatrixWorkspace_sptr rqwWorkspace) {
  m_uiForm.rqwPlot2D->setWorkspace(rqwWorkspace);
}

void IndirectSqwView::setDefaultQAndEnergy() {
  setQRange(m_uiForm.rqwPlot2D->getAxisRange(MantidWidgets::AxisID::YLeft));
  setEnergyRange(m_uiForm.rqwPlot2D->getAxisRange(MantidWidgets::AxisID::XBottom));
}

void IndirectSqwView::setQRange(std::tuple<double, double> const &axisRange) {
  auto const qRange = roundToWidth(axisRange, m_uiForm.spQWidth->value());
  m_uiForm.spQLow->setValue(qRange.first);
  m_uiForm.spQHigh->setValue(qRange.second);
}

void IndirectSqwView::setEnergyRange(std::tuple<double, double> const &axisRange) {
  auto const energyRange = roundToWidth(axisRange, m_uiForm.spEWidth->value());
  m_uiForm.spELow->setValue(energyRange.first);
  m_uiForm.spEHigh->setValue(energyRange.second);
}

std::tuple<double, double> IndirectSqwView::getQRangeFromPlot() {
  return m_uiForm.rqwPlot2D->getAxisRange(MantidWidgets::AxisID::YLeft);
}

std::tuple<double, double> IndirectSqwView::getERangeFromPlot() {
  return m_uiForm.rqwPlot2D->getAxisRange(MantidWidgets::AxisID::XBottom);
}

} // namespace MantidQt::CustomInterfaces