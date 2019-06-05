// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/IntegrateByComponent.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidTypes/SpectrumDefinition.h"

#include <gsl/gsl_statistics.h>
#include <unordered_map>

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(IntegrateByComponent)

using namespace Mantid::API;
using namespace Mantid::Kernel;

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string IntegrateByComponent::name() const {
  return "IntegrateByComponent";
}

/// Algorithm's version for identification. @see Algorithm::version
int IntegrateByComponent::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string IntegrateByComponent::category() const {
  return "Utility\\Workspaces";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void IntegrateByComponent::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>(
                      "InputWorkspace", "", Direction::Input,
                      boost::make_shared<HistogramValidator>()),
                  "The input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                        Direction::Output),
                  "The output workspace.");
  auto mustBePosInt = boost::make_shared<BoundedValidator<int>>();
  mustBePosInt->setLower(0);
  declareProperty(
      "LevelsUp", 0, mustBePosInt,
      "Levels above pixel that will be used to compute the average.\n"
      "If no level is specified, the median is over the whole instrument.\n If "
      "0, it will just return the integrated values in each pixel");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void IntegrateByComponent::exec() {
  MatrixWorkspace_sptr inputWS = this->getProperty("InputWorkspace");
  int parents = getProperty("LevelsUp");
  // Make sure it's integrated
  IAlgorithm_sptr childAlg = createChildAlgorithm("Integration", 0, 0.2);
  childAlg->setProperty("InputWorkspace", inputWS);
  childAlg->setProperty("StartWorkspaceIndex", 0);
  childAlg->setProperty("EndWorkspaceIndex", EMPTY_INT());
  childAlg->setProperty("RangeLower", inputWS->getXMin());
  childAlg->setProperty("RangeUpper", inputWS->getXMax());
  childAlg->setPropertyValue("IncludePartialBins", "1");
  childAlg->executeAsChildAlg();
  MatrixWorkspace_sptr integratedWS = childAlg->getProperty("OutputWorkspace");

  if (parents > 0) {
    std::vector<std::vector<size_t>> specmap = makeMap(integratedWS, parents);
    API::Progress prog(this, 0.3, 1.0, specmap.size());
    // calculate averages
    const auto &spectrumInfo = integratedWS->spectrumInfo();
    for (auto hists : specmap) {
      prog.report();
      std::vector<double> averageYInput, averageEInput;

      PARALLEL_FOR_IF(Kernel::threadSafe(*integratedWS))
      for (int i = 0; i < static_cast<int>(hists.size()); ++i) { // NOLINT
        PARALLEL_START_INTERUPT_REGION

        if (spectrumInfo.isMonitor(hists[i]))
          continue;
        if (spectrumInfo.isMasked(hists[i]))
          continue;

        const double yValue = integratedWS->y(hists[i])[0];
        const double eValue = integratedWS->e(hists[i])[0];

        if (!std::isfinite(yValue) || !std::isfinite(eValue)) // NaNs/Infs
          continue;

        // Now we have a good value
        PARALLEL_CRITICAL(IntegrateByComponent_good) {
          averageYInput.push_back(yValue);
          averageEInput.push_back(eValue * eValue);
        }

        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION

      double averageY, averageE;
      if (averageYInput.empty()) {
        g_log.information(
            "some group has no valid histograms. Will use 0 for average.");
        averageY = 0.;
        averageE = 0.;
      } else {
        averageY = gsl_stats_mean(&averageYInput[0], 1, averageEInput.size());
        averageE = std::sqrt(
            gsl_stats_mean(&averageEInput[0], 1, averageYInput.size()));
      }

      PARALLEL_FOR_IF(Kernel::threadSafe(*integratedWS))
      for (int i = 0; i < static_cast<int>(hists.size()); ++i) { // NOLINT
        PARALLEL_START_INTERUPT_REGION
        if (spectrumInfo.isMonitor(hists[i]))
          continue;
        if (spectrumInfo.isMasked(hists[i]))
          continue;

        const double yValue = integratedWS->y(hists[i])[0];
        const double eValue = integratedWS->e(hists[i])[0];
        if (!std::isfinite(yValue) || !std::isfinite(eValue)) // NaNs/Infs
          continue;

        // Now we have a good value
        PARALLEL_CRITICAL(IntegrateByComponent_setaverage) {
          integratedWS->dataY(hists[i])[0] = averageY;
          integratedWS->dataE(hists[i])[0] = averageE;
        }

        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION
    }
  }
  // Assign it to the output workspace property
  setProperty("OutputWorkspace", integratedWS);
}

/**
 * @brief Creates a map of subcomponents where every spectrum belongs to 1 group
 * only
 * @param countsWS the workspace to check for components/parents
 * @return  vector of vectors, containing each spectrum that belongs to each
 * group
 */
std::vector<std::vector<size_t>>
IntegrateByComponent::makeInstrumentMap(API::MatrixWorkspace_sptr countsWS) {
  std::vector<std::vector<size_t>> mymap;
  std::vector<size_t> single;

  for (size_t i = 0; i < countsWS->getNumberHistograms(); i++) {
    single.push_back(i);
  }
  mymap.push_back(single);
  return mymap;
}

/**
 * @brief This function will check how to group spectra when calculating median
 * @param countsWS the workspace to check for componets/parents
 * @param parents  how many levels above detector to create the grouping
 * @return vector of vectors, containing each spectrum that belongs to each
 * group
 */
std::vector<std::vector<size_t>>
IntegrateByComponent::makeMap(API::MatrixWorkspace_sptr countsWS, int parents) {
  std::unordered_multimap<Mantid::Geometry::ComponentID, size_t> mymap;

  if (parents == 0) // this should not happen in this file, but if one reuses
                    // the function and parents==0, the program has a sudden end
                    // without this check.
  {
    return makeInstrumentMap(countsWS);
  }

  const auto spectrumInfo = countsWS->spectrumInfo();
  const auto &detectorInfo = countsWS->detectorInfo();
  for (size_t i = 0; i < countsWS->getNumberHistograms(); i++) {
    if (!spectrumInfo.hasDetectors(i)) {
      g_log.debug("Spectrum has no detector, skipping");
      continue;
    }

    const auto detIdx = spectrumInfo.spectrumDefinition(i)[0].first;
    std::vector<boost::shared_ptr<const Mantid::Geometry::IComponent>> anc =
        detectorInfo.detector(detIdx).getAncestors();

    if (anc.size() < static_cast<size_t>(parents)) {
      g_log.warning("Too many levels up. Will ignore LevelsUp");
      parents = 0;
      return makeInstrumentMap(countsWS);
    }
    mymap.emplace(anc[parents - 1]->getComponentID(), i);
  }

  std::vector<std::vector<size_t>> speclist;
  std::vector<size_t> speclistsingle;

  auto s_it = mymap.begin();
  for (auto m_it = mymap.begin(); m_it != mymap.end(); m_it = s_it) {
    Mantid::Geometry::ComponentID theKey = (*m_it).first;
    auto keyRange = mymap.equal_range(theKey);

    // Iterate over all map elements with key == theKey
    speclistsingle.clear();
    for (s_it = keyRange.first; s_it != keyRange.second; ++s_it) {
      speclistsingle.push_back((*s_it).second);
    }
    speclist.push_back(speclistsingle);
  }

  return speclist;
}

} // namespace Algorithms
} // namespace Mantid
