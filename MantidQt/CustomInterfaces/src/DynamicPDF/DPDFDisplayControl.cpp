// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Main Module Header
#include "MantidQtCustomInterfaces/DynamicPDF/DPDFDisplayControl.h"
// Mantid Headers from the same project
#include "MantidQtCustomInterfaces/DynamicPDF/DPDFInputDataControl.h"
// Mantid headers from other projects
#include "MantidQtMantidWidgets/DisplayCurveFit.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Exception.h"
// 3rd party library headers
// System #includes
#include <iostream>

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
DisplayControl::DisplayControl(InputDataControl *inputDataControl,
    MantidQt::MantidWidgets::DisplayCurveFit *displayModelFit) :
  m_inputDataControl{inputDataControl},
  m_displayModelFit{displayModelFit},
  m_dataShown(),
  m_dataShownName{"__DPDFDataShown"} {
  // nothing in the body
}

/**
 * @brief Initialize the fitting range and the baseline in the display
 */
void DisplayControl::init() {
  m_displayModelFit->addRangeSelector(dcRange::fit);
  m_displayModelFit->addResidualsZeroline();
}

/**
 * @brief default destructor
 */
DisplayControl::~DisplayControl() = default;

/*              ********************
 *              **  Public Slots  **
 *              ********************/

/*
 * @brief Reset the data to be displayed
 */
void DisplayControl::updateSliceForFitting() {
  try{
    //check the workspace is registered in the Analysis Data Service
    auto workspace = Mantid::API::AnalysisDataService::Instance().retrieveWS
    <Mantid::API::MatrixWorkspace>(m_dataShownName);
    // delete the workspace being shown
    auto deleteWsAlg = Mantid::API::AlgorithmManager::Instance().create("DeleteWorkspace");
    deleteWsAlg->initialize();
    deleteWsAlg->setChild(true);
    deleteWsAlg->setLogging(false);
    deleteWsAlg->setProperty("Workspace", m_dataShownName);
    deleteWsAlg->execute();
  }catch (Mantid::Kernel::Exception::NotFoundError &) {}
  // create the data workspace to be shown
  auto dataX = m_inputDataControl->selectedDataX();
  auto dataY = m_inputDataControl->selectedDataY();
  auto dataE = m_inputDataControl->selectedDataE();
  auto energy = m_inputDataControl->getSelectedEnergy();
  std::ostringstream energyLabelStream;
  energyLabelStream << energy;
  //create internal workspace containing the non-zero signal
  auto createWsAlg = Mantid::API::AlgorithmManager::Instance().create("CreateWorkspace");
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
  Mantid::API::MatrixWorkspace_sptr m_dataShown = createWsAlg->getProperty("OutputWorkspace");
  Mantid::API::AnalysisDataService::Instance().add(m_dataShownName, m_dataShown);
  // show the workspace with appropriate range selector
  m_displayModelFit->addSpectrum(curveType::data, m_dataShown);
  auto curveRange = m_displayModelFit->getCurveRange(curveType::data);
  auto rangeSelectorFit = m_displayModelFit->m_rangeSelector.at(dcRange::fit);
  rangeSelectorFit->setRange(curveRange.first, curveRange.second);
  rangeSelectorFit->setMinimum(curveRange.first);
  rangeSelectorFit->setMaximum(curveRange.second);
}

}
}
}
