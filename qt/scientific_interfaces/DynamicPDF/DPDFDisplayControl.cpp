// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Main Module Header
#include "DPDFDisplayControl.h"
// Mantid Headers from the same project
#include "DPDFInputDataControl.h"
// Mantid headers from other projects
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Exception.h"
#include "MantidQtWidgets/Plotting/Qwt/DisplayCurveFit.h"

namespace {
Mantid::Kernel::Logger g_log("DynamicPDF");
}

namespace MantidQt {
namespace CustomInterfaces {
namespace DynamicPDF {

using curveType = MantidQt::MantidWidgets::DisplayCurveFit::curveType;
using dcRange = MantidQt::MantidWidgets::DisplayCurveFit::dcRange;

/*              **********************
 *              **  Public Members  **
 *              **********************/

/**
 * @brief Constructor
 * @param inputDataControl to handle the input data
 * @param displayModelFit to handle displaying the curves
 */
DisplayControl::DisplayControl(
    InputDataControl *inputDataControl,
    MantidQt::MantidWidgets::DisplayCurveFit *displayModelFit)
    : m_inputDataControl{inputDataControl}, m_displayModelFit{displayModelFit},
      m_fitRangeSelector{nullptr},
      m_dataShown(), m_dataShownName{"__DPDFDataShown"} {
  // nothing in the body
}

/**
 * @brief Initialize the fitting range and the baseline in the display
 */
void DisplayControl::init() {
  m_displayModelFit->addRangeSelector(dcRange::fit);
  m_fitRangeSelector = m_displayModelFit->m_rangeSelector.at(dcRange::fit);
  m_displayModelFit->addResidualsZeroline();

  // SIGNAL/SLOT connections
  // user manipulated the fit-range selector
  connect(m_fitRangeSelector, SIGNAL(minValueChanged(double)), this,
          SLOT(rangeSelectorFitUpdated(double)));
  connect(m_fitRangeSelector, SIGNAL(maxValueChanged(double)), this,
          SLOT(rangeSelectorFitUpdated(double)));
}

/**
 * @brief default destructor
 */
DisplayControl::~DisplayControl() = default;

/**
 * @brief get the current boundaries of the rangeSelectorFit
 */
std::pair<double, double> DisplayControl::getFitMinMax() {
  return std::pair<double, double>(m_fitRangeSelector->getMinimum(),
                                   m_fitRangeSelector->getMaximum());
}

/**
 * @brief set the low boundary of the rangeSelectorFit
 */
void DisplayControl::setFitMin(const double &boundary) {
  m_fitRangeSelector->setMinimum(boundary);
}

/**
 * @brief set the upper boundary of the rangeSelectorFit
 */
void DisplayControl::setFitMax(const double &boundary) {
  m_fitRangeSelector->setMaximum(boundary);
}

/*              ********************
 *              **  Public Slots  **
 *              ********************/

/*
 * @brief Reset the data to be displayed. Remove model evaluation curves
 */
void DisplayControl::updateSliceForFitting() {
  try {
    // check the workspace is registered in the Analysis Data Service
    auto workspace =
        Mantid::API::AnalysisDataService::Instance()
            .retrieveWS<Mantid::API::MatrixWorkspace>(m_dataShownName);
    // delete the workspace being shown
    auto deleteWsAlg =
        Mantid::API::AlgorithmManager::Instance().create("DeleteWorkspace");
    deleteWsAlg->initialize();
    deleteWsAlg->setChild(true);
    deleteWsAlg->setLogging(false);
    deleteWsAlg->setProperty("Workspace", m_dataShownName);
    deleteWsAlg->execute();
  } catch (Mantid::Kernel::Exception::NotFoundError &) {
  }
  if (m_displayModelFit->hasCurve(curveType::fit)) {
    m_displayModelFit->removeSpectrum(curveType::fit);
    m_displayModelFit->removeSpectrum(curveType::residuals);
  }
  // create the data workspace to be shown
  auto dataX = m_inputDataControl->selectedDataX();
  auto dataY = m_inputDataControl->selectedDataY();
  auto dataE = m_inputDataControl->selectedDataE();
  auto energy = m_inputDataControl->getSelectedEnergy();
  std::ostringstream energyLabelStream;
  energyLabelStream << energy;
  // create internal workspace containing the non-zero signal
  auto createWsAlg =
      Mantid::API::AlgorithmManager::Instance().create("CreateWorkspace");
  createWsAlg->initialize();
  createWsAlg->setChild(true);
  createWsAlg->setLogging(false);
  createWsAlg->setProperty("OutputWorkspace", m_dataShownName);
  createWsAlg->setProperty("NSpec", 1);
  createWsAlg->setProperty("DataX", dataX);
  createWsAlg->setProperty("DataY", dataY);
  createWsAlg->setProperty("DataE", dataE);
  createWsAlg->setProperty("UnitX", "MomentumTransfer");
  createWsAlg->setProperty("VerticalAxisUnit", "DeltaE");
  createWsAlg->setProperty("VerticalAxisValues", energyLabelStream.str());
  createWsAlg->execute();
  Mantid::API::MatrixWorkspace_sptr m_dataShown =
      createWsAlg->getProperty("OutputWorkspace");
  Mantid::API::AnalysisDataService::Instance().add(m_dataShownName,
                                                   m_dataShown);
  // show the workspace with appropriate range selector
  m_displayModelFit->addSpectrum(curveType::data, m_dataShown);
  auto curveRange = m_displayModelFit->getCurveRange(curveType::data);
  auto rangeSelectorFit = m_displayModelFit->m_rangeSelector.at(dcRange::fit);
  rangeSelectorFit->setRange(curveRange.first, curveRange.second);
  rangeSelectorFit->setMinimum(curveRange.first);
  rangeSelectorFit->setMaximum(curveRange.second);
  // emit signalRangeSelectorFitUpdated();
}

/**
 * @brief pass the signal received from the fit-range selector
 */
void DisplayControl::rangeSelectorFitUpdated(const double &boundary) {
  UNUSED_ARG(boundary);
  emit signalRangeSelectorFitUpdated();
}

/*
 * @brief Display the new model evaluation and residuals
 * @param workspaceName workspace name containing the evaluation of the model
 */
void DisplayControl::updateModelEvaluationDisplay(
    const QString &workspaceName) {
  auto modelWorkspace = Mantid::API::AnalysisDataService::Instance()
                            .retrieveWS<Mantid::API::MatrixWorkspace>(
                                workspaceName.toStdString());
  if (!modelWorkspace) {
    throw std::runtime_error("Unfound workspace containing model evaluation");
  }
  if (m_displayModelFit->hasCurve(curveType::fit)) {
    m_displayModelFit->removeSpectrum(curveType::fit);
    m_displayModelFit->removeSpectrum(curveType::residuals);
  }
  // index = 1 is the model evaluation and index=2 contain the residuals
  m_displayModelFit->addSpectrum(curveType::fit, modelWorkspace, 1);
  m_displayModelFit->addSpectrum(curveType::residuals, modelWorkspace, 2);
}
} // namespace DynamicPDF
} // namespace CustomInterfaces
} // namespace MantidQt
