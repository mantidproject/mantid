// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CalculateEfficiency.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include <vector>

namespace Mantid::Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(CalculateEfficiency)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

/** Initialization method.
 *
 */
void CalculateEfficiency::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
                  "The workspace containing the flood data");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "The name of the workspace to be created as the output of the algorithm");

  auto positiveDouble = std::make_shared<BoundedValidator<double>>();
  positiveDouble->setLower(0);
  declareProperty("MinEfficiency", EMPTY_DBL(), positiveDouble,
                  "Minimum efficiency for a pixel to be considered (default: no minimum).");
  declareProperty("MaxEfficiency", EMPTY_DBL(), positiveDouble,
                  "Maximum efficiency for a pixel to be considered (default: no maximum).");
  declareProperty("MaskedFullComponent", "", "Component Name to fully mask according to the IDF file.");
  declareProperty(std::make_unique<ArrayProperty<int>>("MaskedEdges"),
                  "Number of pixels to mask on the edges: X-low, X-high, Y-low, Y-high");
  declareProperty("MaskedComponent", "", "Component Name to mask the edges according to the IDF file.");
}

/** Executes the algorithm
 *
 */
void CalculateEfficiency::exec() {

  // Minimum efficiency. Pixels with lower efficiency will be masked
  double min_eff = getProperty("MinEfficiency");
  // Maximum efficiency. Pixels with higher efficiency will be masked
  double max_eff = getProperty("MaxEfficiency");

  // Get the input workspace
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr rebinnedWS; // = inputWS;

  //  BioSANS has 2 detectors and reduces one at the time: one is masked!
  //  We must use that masked detector
  const std::string maskedFullComponent = getPropertyValue("MaskedFullComponent");
  if (!maskedFullComponent.empty()) {
    g_log.debug() << "CalculateEfficiency: Masking Full Component: " << maskedFullComponent << "\n";
    maskComponent(*inputWS, maskedFullComponent);
  }

  // BioSANS has 2 detectors and the front masks the back!!!!
  // We must mask the shaded part to calculate efficency
  std::vector<int> maskedEdges = getProperty("MaskedEdges");
  if (!maskedEdges.empty() && (maskedEdges[0] > 0 || maskedEdges[1] > 0 || maskedEdges[2] > 0 || maskedEdges[3] > 0)) {
    g_log.debug() << "CalculateEfficiency: Masking edges length = " << maskedEdges.size() << ")"
                  << " of the component " << maskedFullComponent << "\n";
    const std::string maskedComponent = getPropertyValue("MaskedComponent");
    maskEdges(inputWS, maskedEdges[0], maskedEdges[1], maskedEdges[2], maskedEdges[3], maskedComponent);
  }
  // Now create the output workspace
  MatrixWorkspace_sptr outputWS; // = getProperty("OutputWorkspace");

  // DataObjects::EventWorkspace_const_sptr inputEventWS =
  // std::dynamic_pointer_cast<const EventWorkspace>(inputWS);

  // Sum up all the wavelength bins
  auto childAlg = createChildAlgorithm("Integration", 0.0, 0.2);
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWS);
  childAlg->executeAsChildAlg();
  rebinnedWS = childAlg->getProperty("OutputWorkspace");

  outputWS = WorkspaceFactory::Instance().create(rebinnedWS);
  WorkspaceFactory::Instance().initializeFromParent(*inputWS, *outputWS, false);
  for (int i = 0; i < static_cast<int>(rebinnedWS->getNumberHistograms()); i++) {
    outputWS->setSharedX(i, rebinnedWS->sharedX(i));
  }
  setProperty("OutputWorkspace", outputWS);

  double sum = 0.0;
  double err = 0.0;
  int npixels = 0;

  // Loop over spectra and sum all the counts to get normalization
  // Skip monitors and masked detectors
  sumUnmaskedDetectors(rebinnedWS, sum, err, npixels);

  // Normalize each detector pixel by the sum we just found to get the
  // relative efficiency. If the minimum and maximum efficiencies are
  // provided, the pixels falling outside this range will be marked
  // as 'masked' in both the input and output workspace.
  // We mask detectors in the input workspace so that we can resum the
  // counts to find a new normalization factor that takes into account
  // the newly masked detectors.
  normalizeDetectors(rebinnedWS, outputWS, sum, err, npixels, min_eff, max_eff);

  if (!isEmpty(min_eff) || !isEmpty(max_eff)) {
    // Recompute the normalization, excluding the pixels that were outside
    // the acceptable efficiency range.
    sumUnmaskedDetectors(rebinnedWS, sum, err, npixels);

    // Now that we have a normalization factor that excludes bad pixels,
    // recompute the relative efficiency.
    // We pass EMPTY_DBL() to avoid masking pixels that might end up high or low
    // after the new normalization.
    normalizeDetectors(rebinnedWS, outputWS, sum, err, npixels, EMPTY_DBL(), EMPTY_DBL());
  }
}

/*
 *  Sum up all the unmasked detector pixels.
 *
 * @param rebinnedWS: workspace where all the wavelength bins have been grouped
 *together
 * @param sum: sum of all the unmasked detector pixels (counts)
 * @param error: error on sum (counts)
 * @param nPixels: number of unmasked detector pixels that contributed to sum
 */
void CalculateEfficiency::sumUnmaskedDetectors(const MatrixWorkspace_sptr &rebinnedWS, double &sum, double &error,
                                               int &nPixels) {
  // Number of spectra
  const auto numberOfSpectra = static_cast<int>(rebinnedWS->getNumberHistograms());
  sum = 0.0;
  error = 0.0;
  nPixels = 0;

  const auto &spectrumInfo = rebinnedWS->spectrumInfo();
  for (int i = 0; i < numberOfSpectra; i++) {
    progress(0.2 + 0.2 * i / numberOfSpectra, "Computing sensitivity");
    // Skip if we have a monitor or if the detector is masked.
    if (spectrumInfo.isMonitor(i) || spectrumInfo.isMasked(i))
      continue;

    // Retrieve the spectrum into a vector
    const auto &YValues = rebinnedWS->y(i);
    const auto &YErrors = rebinnedWS->e(i);

    sum += YValues[0];
    error += YErrors[0] * YErrors[0];
    nPixels++;
  }

  g_log.debug() << "sumUnmaskedDetectors: unmasked pixels = " << nPixels << " from a total of " << numberOfSpectra
                << "\n";

  error = std::sqrt(error);
}

/*
 * Normalize each detector to produce the relative detector efficiency.
 * Pixels that fall outside those efficiency limits will be masked in both
 * the input and output workspaces.
 *
 * @param rebinnedWS: input workspace
 * @param outputWS: output workspace containing the relative efficiency
 * @param sum: sum of all the unmasked detector pixels (counts)
 * @param error: error on sum (counts)
 * @param nPixels: number of unmasked detector pixels that contributed to sum
 */

void CalculateEfficiency::normalizeDetectors(const MatrixWorkspace_sptr &rebinnedWS,
                                             const MatrixWorkspace_sptr &outputWS, double sum, double error,
                                             int nPixels, double min_eff, double max_eff) {
  // Number of spectra
  const size_t numberOfSpectra = rebinnedWS->getNumberHistograms();

  // Empty vector to store the pixels that outside the acceptable efficiency
  // range
  std::vector<size_t> dets_to_mask;

  const auto &spectrumInfo = rebinnedWS->spectrumInfo();
  for (size_t i = 0; i < numberOfSpectra; i++) {
    const double currProgress = 0.4 + 0.2 * (static_cast<double>(i) / static_cast<double>(numberOfSpectra));
    progress(currProgress, "Computing sensitivity");
    // If this spectrum is masked, skip to the next one
    if (spectrumInfo.isMasked(i))
      continue;

    // Retrieve the spectrum into a vector
    auto &YIn = rebinnedWS->y(i);
    auto &EIn = rebinnedWS->e(i);
    auto &YOut = outputWS->mutableY(i);
    auto &EOut = outputWS->mutableE(i);
    // If this detector is a monitor, skip to the next one
    if (spectrumInfo.isMonitor(i)) {
      YOut[0] = 1.0;
      EOut[0] = 0.0;
      continue;
    }

    // Normalize counts to get relative efficiency
    YOut[0] = nPixels / sum * YIn[0];
    const double err_sum = YIn[0] / sum * error;
    EOut[0] = nPixels / std::abs(sum) * std::sqrt(EIn[0] * EIn[0] + err_sum * err_sum);

    // Mask this detector if the signal is outside the acceptable band
    if (!isEmpty(min_eff) && YOut[0] < min_eff)
      dets_to_mask.emplace_back(i);
    if (!isEmpty(max_eff) && YOut[0] > max_eff)
      dets_to_mask.emplace_back(i);
  }

  g_log.debug() << "normalizeDetectors: Masked pixels outside the acceptable "
                   "efficiency range ["
                << min_eff << "," << max_eff << "] = " << dets_to_mask.size()
                << " from a total of non masked = " << nPixels
                << " (from a total number of spectra in the ws = " << numberOfSpectra << ")\n";

  // If we identified pixels to be masked, mask them now
  if (!dets_to_mask.empty()) {
    // Mask detectors that were found to be outside the acceptable efficiency
    // band
    try {
      auto mask = createChildAlgorithm("MaskDetectors", 0.8, 0.9);
      // First we mask detectors in the output workspace
      mask->setProperty<MatrixWorkspace_sptr>("Workspace", outputWS);
      mask->setProperty<std::vector<size_t>>("WorkspaceIndexList", dets_to_mask);
      mask->execute();

      mask = createChildAlgorithm("MaskDetectors", 0.9, 1.0);
      // Then we mask the same detectors in the input workspace
      mask->setProperty<MatrixWorkspace_sptr>("Workspace", rebinnedWS);
      mask->setProperty<std::vector<size_t>>("WorkspaceIndexList", dets_to_mask);
      mask->execute();
    } catch (std::invalid_argument &err) {
      std::stringstream e;
      e << "Invalid argument to MaskDetectors Child Algorithm: " << err.what();
      g_log.error(e.str());
    } catch (std::runtime_error &err) {
      std::stringstream e;
      e << "Unable to successfully run MaskDetectors Child Algorithm: " << err.what();
      g_log.error(e.str());
    }
  }
}

/**
 * Fully masks one component named componentName
 * @param ws :: workspace with the respective instrument assigned
 * @param componentName :: must be a known CompAssembly.
 */
void CalculateEfficiency::maskComponent(MatrixWorkspace &ws, const std::string &componentName) {
  auto instrument = ws.getInstrument();
  try {
    std::shared_ptr<const Geometry::ICompAssembly> component =
        std::dynamic_pointer_cast<const Geometry::ICompAssembly>(instrument->getComponentByName(componentName));
    if (!component) {
      g_log.warning("Component " + componentName + " expected to be a CompAssembly, e.g., a bank. Component " +
                    componentName + " not masked!");
      return;
    }
    std::vector<detid_t> detectorList;
    for (int x = 0; x < component->nelements(); x++) {
      std::shared_ptr<Geometry::ICompAssembly> xColumn =
          std::dynamic_pointer_cast<Geometry::ICompAssembly>((*component)[x]);
      for (int y = 0; y < xColumn->nelements(); y++) {
        std::shared_ptr<Geometry::Detector> detector = std::dynamic_pointer_cast<Geometry::Detector>((*xColumn)[y]);
        if (detector) {
          auto detID = detector->getID();
          detectorList.emplace_back(detID);
        }
      }
    }
    auto indexList = ws.getIndicesFromDetectorIDs(detectorList);
    auto &spectrumInfo = ws.mutableSpectrumInfo();
    for (const auto &idx : indexList) {
      ws.getSpectrum(idx).clearData();
      spectrumInfo.setMasked(idx, true);
    }
  } catch (std::exception &) {
    g_log.warning("Expecting the component " + componentName +
                  " to be a CompAssembly, e.g., a bank. Component not masked!");
  }
}

/**
 * Mask edges of a RectangularDetector
 * @param ws :: Input workspace
 * @param left :: number of columns to mask left
 * @param right :: number of columns to mask right
 * @param high :: number of rows to mask top
 * @param low :: number of rows to mask Bottom
 * @param componentName :: Must be a RectangularDetector
 */
void CalculateEfficiency::maskEdges(const MatrixWorkspace_sptr &ws, int left, int right, int high, int low,
                                    const std::string &componentName) {

  auto instrument = ws->getInstrument();

  std::shared_ptr<Mantid::Geometry::RectangularDetector> component;
  try {
    component = std::const_pointer_cast<Mantid::Geometry::RectangularDetector>(
        std::dynamic_pointer_cast<const Mantid::Geometry::RectangularDetector>(
            instrument->getComponentByName(componentName)));
  } catch (std::exception &) {
    g_log.warning("Expecting the component " + componentName + " to be a RectangularDetector. maskEdges not executed.");
    return;
  }
  if (!component) {
    g_log.warning("Component " + componentName + " is not a RectangularDetector. MaskEdges not executed.");
    return;
  }

  std::vector<int> IDs;
  int i = 0;

  while (i < left * component->idstep()) {
    IDs.emplace_back(component->idstart() + i);
    i += 1;
  }
  // right
  i = component->maxDetectorID() - right * component->idstep();
  while (i < component->maxDetectorID()) {
    IDs.emplace_back(i);
    i += 1;
  }
  // low: 0,256,512,768,..,1,257,513
  for (int row = 0; row < low; row++) {
    i = row + component->idstart();
    while (i < component->nelements() * component->idstep() - component->idstep() + low + component->idstart()) {
      IDs.emplace_back(i);
      i += component->idstep();
    }
  }
  // high # 255, 511, 767..
  for (int row = 0; row < high; row++) {
    i = component->idstep() + component->idstart() - row - 1;
    while (i < component->nelements() * component->idstep() + component->idstart()) {
      IDs.emplace_back(i);
      i += component->idstep();
    }
  }

  g_log.debug() << "CalculateEfficiency::maskEdges Detector Ids to Mask:" << std::endl;
  for (auto id : IDs) {
    g_log.debug() << id << " ";
  }
  g_log.debug() << std::endl;

  auto maskAlg = createChildAlgorithm("MaskDetectors");
  maskAlg->setChild(true);
  maskAlg->setProperty("Workspace", ws);
  maskAlg->setProperty("DetectorList", IDs);
  maskAlg->execute();
}

} // namespace Mantid::Algorithms
