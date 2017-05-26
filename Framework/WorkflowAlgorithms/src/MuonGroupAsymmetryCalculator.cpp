#include "MantidWorkflowAlgorithms/MuonGroupAsymmetryCalculator.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/VectorHelper.h"

using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::Workspace_sptr;
using Mantid::API::IAlgorithm_sptr;
using Mantid::API::ITableWorkspace_sptr;
using Mantid::API::AlgorithmManager;
using Mantid::API::WorkspaceFactory;

namespace Mantid {
namespace WorkflowAlgorithms {

//----------------------------------------------------------------------------------------------
/** Constructor
* @param inputWS :: [input] Input workspace group
* @param summedPeriods :: [input] Vector of period indexes to be summed
* @param subtractedPeriods :: [input] Vector of period indexes to be subtracted
* from summed periods
* @param start is the start time
* @param end is the end time
* @param groupIndex :: [input] Workspace index of the group to analyse
 */
MuonGroupAsymmetryCalculator::MuonGroupAsymmetryCalculator(
    const Mantid::API::WorkspaceGroup_sptr inputWS,
    const std::vector<int> summedPeriods,
    const std::vector<int> subtractedPeriods, const int groupIndex,
    const double start, const double end)
    : MuonGroupCalculator(inputWS, summedPeriods, subtractedPeriods,
                          groupIndex) {
  MuonGroupCalculator::setStartEnd(start, end);
}

/**
* Calculates asymmetry between given group (specified via group index) and Muon
* exponential decay
* @returns Workspace containing result of calculation
*/
MatrixWorkspace_sptr MuonGroupAsymmetryCalculator::calculate() const {
  // The output workspace

  MatrixWorkspace_sptr tempWS;
  int numPeriods = m_inputWS->getNumberOfEntries();
  if (numPeriods > 1) {
    // Several period workspaces were supplied

    auto summedWS = sumPeriods(m_summedPeriods);
    auto subtractedWS = sumPeriods(m_subtractedPeriods);

    // Remove decay (summed periods ws)
    MatrixWorkspace_sptr asymSummedPeriods =
        estimateAsymmetry(summedWS, m_groupIndex);

    if (!m_subtractedPeriods.empty()) {
      // Remove decay (subtracted periods ws)
      MatrixWorkspace_sptr asymSubtractedPeriods =
          estimateAsymmetry(subtractedWS, m_groupIndex);

      // Now subtract
      tempWS = subtractWorkspaces(asymSummedPeriods, asymSubtractedPeriods);
    } else {
      tempWS = asymSummedPeriods;
    }
  } else {
    // Only one period was supplied
    tempWS = estimateAsymmetry(m_inputWS->getItem(0),
                               m_groupIndex); // change -1 to m_groupIndex and
                                              // follow through to store as a
                                              // table for later.
  }

  // Extract the requested spectrum
  MatrixWorkspace_sptr outWS = extractSpectrum(tempWS, m_groupIndex);
  return outWS;
}

/**
* Removes exponential decay from the given workspace.
* @param inputWS :: [input] Workspace to remove decay from
* @param index :: [input] GroupIndex (fit only the requested spectrum): use -1
* for "unset"
* @returns Result of the removal
*/
MatrixWorkspace_sptr
MuonGroupAsymmetryCalculator::removeExpDecay(const Workspace_sptr &inputWS,
                                             const int index) const {
  MatrixWorkspace_sptr outWS;
  // Remove decay
  if (inputWS) {
    IAlgorithm_sptr asym =
        AlgorithmManager::Instance().create("RemoveExpDecay");
    asym->setChild(true);
    asym->setProperty("InputWorkspace", inputWS);
    if (index > 0) {
      // GroupIndex as vector
      // Necessary if we want RemoveExpDecay to fit only the requested
      // spectrum
      std::vector<int> spec(1, index);
      asym->setProperty("Spectra", spec);
    }
    asym->setProperty("OutputWorkspace", "__NotUsed__");
    asym->execute();
    outWS = asym->getProperty("OutputWorkspace");
  }
  return outWS;
}
/**
* Estimate the asymmetrey for the given workspace (TF data).
* @param inputWS :: [input] Workspace to calculate asymmetry for
* @param index :: [input] GroupIndex (fit only the requested spectrum): use -1
* for "unset"
* @returns Result of the removal
*/
MatrixWorkspace_sptr
MuonGroupAsymmetryCalculator::estimateAsymmetry(const Workspace_sptr &inputWS,
                                                const int index) const {
  std::vector<double> normEst;
  MatrixWorkspace_sptr outWS;
  // calculate asymmetry
  if (inputWS) {
    IAlgorithm_sptr asym =
        AlgorithmManager::Instance().create("EstimateMuonAsymmetryFromCounts");
    asym->setChild(true);
    asym->setProperty("InputWorkspace", inputWS);
    if (index > -1) {
      std::vector<int> spec(1, index);
      asym->setProperty("Spectra", spec);
    }
    asym->setProperty("OutputWorkspace", "__NotUsed__");
    asym->setProperty("StartX", m_startX);
    asym->setProperty("EndX", m_endX);
    asym->setProperty("NormalizationIn", getStoredNorm());
    asym->execute();
    outWS = asym->getProperty("OutputWorkspace");
    auto tmp = asym->getPropertyValue("NormalizationConstant");
    // move to helper later as a function
    normEst = Kernel::VectorHelper::splitStringIntoVector<double>(tmp);
    ITableWorkspace_sptr table = WorkspaceFactory::Instance().createTable();
    API::AnalysisDataService::Instance().addOrReplace("__norm__", table);
    table->addColumn("double", "norm");
    table->addColumn("int", "spectra");

    for (double norm : normEst) {
      API::TableRow row = table->appendRow();

      row << norm << index;
    }
  }
  return outWS;
}
/*
* Reads in the stored normalization
* if >0 then it is used instead of
* estimating the norm.
*/
double getStoredNorm() {
  if (!API::AnalysisDataService::Instance().doesExist("__keepNorm__")) {
    return 0.0;
  } else {
    API::ITableWorkspace_sptr table =
        boost::dynamic_pointer_cast<API::ITableWorkspace>(
            API::AnalysisDataService::Instance().retrieve("__keepNorm__"));
    auto colNorm = table->getColumn("norm");
    return (*colNorm)[0];
  }
}

} // namespace WorkflowAlgorithms
} // namespace Mantid
