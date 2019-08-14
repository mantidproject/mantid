// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/CentroidPeaks.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/EdgePixel.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/VectorHelper.h"
#include <boost/algorithm/clamp.hpp>

using Mantid::DataObjects::PeaksWorkspace;

namespace Mantid {
namespace Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CentroidPeaks)

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::Crystal;

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CentroidPeaks::init() {
  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>(
                      "InPeaksWorkspace", "", Direction::Input),
                  "A PeaksWorkspace containing the peaks to centroid.");

  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "",
                                                        Direction::Input),
                  "An input 2D Workspace.");

  declareProperty(
      std::make_unique<PropertyWithValue<int>>("PeakRadius", 10,
                                               Direction::Input),
      "Fixed radius around each peak position in which to calculate the "
      "centroid.");

  declareProperty(std::make_unique<PropertyWithValue<int>>("EdgePixels", 0,
                                                           Direction::Input),
                  "The number of pixels where peaks are removed at edges. Only "
                  "for instruments with RectangularDetectors. ");

  declareProperty(
      std::make_unique<WorkspaceProperty<PeaksWorkspace>>(
          "OutPeaksWorkspace", "", Direction::Output),
      "The output PeaksWorkspace will be a copy of the input PeaksWorkspace "
      "with the peaks' positions modified by the new found centroids.");
}

//----------------------------------------------------------------------------------------------
/** Integrate the peaks of the workspace using parameters saved in the algorithm
 * class
 */
void CentroidPeaks::integrate() {

  /// Peak workspace to centroid
  Mantid::DataObjects::PeaksWorkspace_sptr inPeakWS =
      getProperty("InPeaksWorkspace");

  /// Output peaks workspace, create if needed
  Mantid::DataObjects::PeaksWorkspace_sptr peakWS =
      getProperty("OutPeaksWorkspace");
  if (peakWS != inPeakWS)
    peakWS = inPeakWS->clone();

  /// Radius to use around peaks
  int PeakRadius = getProperty("PeakRadius");

  int MinPeaks = -1;
  int MaxPeaks = -1;
  size_t Numberwi = inWS->getNumberHistograms();
  int NumberPeaks = peakWS->getNumberPeaks();
  for (int i = 0; i < NumberPeaks; ++i) {
    Peak &peak = peakWS->getPeaks()[i];
    int pixelID = peak.getDetectorID();

    // Find the workspace index for this detector ID
    if (wi_to_detid_map.find(pixelID) != wi_to_detid_map.end()) {
      size_t wi = wi_to_detid_map[pixelID];
      if (MinPeaks == -1 && peak.getRunNumber() == inWS->getRunNumber() &&
          wi < Numberwi)
        MinPeaks = i;
      if (peak.getRunNumber() == inWS->getRunNumber() && wi < Numberwi)
        MaxPeaks = i;
    }
  }

  const auto inBlocksize = static_cast<int>(inWS->blocksize());
  int Edge = getProperty("EdgePixels");
  Progress prog(this, MinPeaks, 1.0, MaxPeaks);
  PARALLEL_FOR_IF(Kernel::threadSafe(*inWS, *peakWS))
  for (int i = MinPeaks; i <= MaxPeaks; ++i) {
    PARALLEL_START_INTERUPT_REGION
    // Get a direct ref to that peak.
    auto &peak = peakWS->getPeak(i);
    int col = peak.getCol();
    int row = peak.getRow();
    int pixelID = peak.getDetectorID();
    auto detidIterator = wi_to_detid_map.find(pixelID);
    if (detidIterator == wi_to_detid_map.end()) {
      continue;
    }
    size_t workspaceIndex = detidIterator->second;
    double TOFPeakd = peak.getTOF();
    const auto &X = inWS->x(workspaceIndex);
    int chan = Kernel::VectorHelper::getBinIndex(X.rawData(), TOFPeakd);
    std::string bankName = peak.getBankName();
    int nCols = 0, nRows = 0;
    sizeBanks(bankName, nCols, nRows);

    double intensity = 0.0;
    double chancentroid = 0.0;

    int chanstart = std::max(0, chan - PeakRadius);
    int chanend = std::min(static_cast<int>(X.size()), chan + PeakRadius);
    double rowcentroid = 0.0;
    int rowstart = std::max(0, row - PeakRadius);
    int rowend = std::min(nRows - 1, row + PeakRadius);
    double colcentroid = 0.0;
    int colstart = std::max(0, col - PeakRadius);
    int colend = std::min(nCols - 1, col + PeakRadius);
    for (int ichan = chanstart; ichan <= chanend; ++ichan) {
      for (int irow = rowstart; irow <= rowend; ++irow) {
        for (int icol = colstart; icol <= colend; ++icol) {
          if (edgePixel(inst, bankName, icol, irow, Edge))
            continue;
          const auto it =
              wi_to_detid_map.find(findPixelID(bankName, icol, irow));
          if (it == wi_to_detid_map.end())
            continue;

          const auto &histogram = inWS->y(it->second);

          intensity += histogram[ichan];
          rowcentroid += irow * histogram[ichan];
          colcentroid += icol * histogram[ichan];
          chancentroid += ichan * histogram[ichan];
        }
      }
    }
    // Set pixelID to change row and col
    row = int(rowcentroid / intensity);
    boost::algorithm::clamp(row, 0, nRows - 1);
    col = int(colcentroid / intensity);
    boost::algorithm::clamp(col, 0, nCols - 1);
    chan = int(chancentroid / intensity);
    boost::algorithm::clamp(chan, 0, inBlocksize);

    // Set wavelength to change tof for peak object
    if (!edgePixel(inst, bankName, col, row, Edge)) {
      peak.setDetectorID(findPixelID(bankName, col, row));
      detidIterator = wi_to_detid_map.find(findPixelID(bankName, col, row));
      workspaceIndex = (detidIterator->second);
      Mantid::Kernel::Units::Wavelength wl;
      std::vector<double> timeflight;
      timeflight.push_back(inWS->x(workspaceIndex)[chan]);
      double scattering = peak.getScattering();
      double L1 = peak.getL1();
      double L2 = peak.getL2();
      wl.fromTOF(timeflight, timeflight, L1, L2, scattering, 0, 0, 0);
      const double lambda = timeflight[0];
      timeflight.clear();

      peak.setWavelength(lambda);
      peak.setBinCount(inWS->y(workspaceIndex)[chan]);
    }
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  removeEdgePeaks(*peakWS);

  // Save the output
  setProperty("OutPeaksWorkspace", peakWS);
}

//----------------------------------------------------------------------------------------------
/** Integrate the peaks of the workspace using parameters saved in the algorithm
 * class
 */
void CentroidPeaks::integrateEvent() {

  /// Peak workspace to centroid
  Mantid::DataObjects::PeaksWorkspace_sptr inPeakWS =
      getProperty("InPeaksWorkspace");

  /// Output peaks workspace, create if needed
  Mantid::DataObjects::PeaksWorkspace_sptr peakWS =
      getProperty("OutPeaksWorkspace");
  if (peakWS != inPeakWS)
    peakWS = inPeakWS->clone();

  /// Radius to use around peaks
  int PeakRadius = getProperty("PeakRadius");

  int MinPeaks = -1;
  int MaxPeaks = -1;
  size_t Numberwi = inWS->getNumberHistograms();
  int NumberPeaks = peakWS->getNumberPeaks();

  for (int i = 0; i < NumberPeaks; ++i) {
    auto &peak = peakWS->getPeak(i);
    int pixelID = peak.getDetectorID();

    // Find the workspace index for this detector ID
    if (wi_to_detid_map.find(pixelID) != wi_to_detid_map.end()) {
      size_t wi = wi_to_detid_map[pixelID];
      if (MinPeaks == -1 && peak.getRunNumber() == inWS->getRunNumber() &&
          wi < Numberwi)
        MinPeaks = i;
      if (peak.getRunNumber() == inWS->getRunNumber() && wi < Numberwi)
        MaxPeaks = i;
    }
  }

  int Edge = getProperty("EdgePixels");
  Progress prog(this, MinPeaks, 1.0, MaxPeaks);
  PARALLEL_FOR_IF(Kernel::threadSafe(*inWS, *peakWS))
  for (int i = MinPeaks; i <= MaxPeaks; ++i) {
    PARALLEL_START_INTERUPT_REGION
    // Get a direct ref to that peak.
    auto &peak = peakWS->getPeak(i);
    int col = peak.getCol();
    int row = peak.getRow();
    double TOFPeakd = peak.getTOF();
    std::string bankName = peak.getBankName();
    int nCols = 0, nRows = 0;
    sizeBanks(bankName, nCols, nRows);

    double intensity = 0.0;
    double tofcentroid = 0.0;
    if (edgePixel(inst, bankName, col, row, Edge))
      continue;

    double tofstart = TOFPeakd * std::pow(1.004, -PeakRadius);
    double tofend = TOFPeakd * std::pow(1.004, PeakRadius);
    double rowcentroid = 0.0;
    int rowstart = std::max(0, row - PeakRadius);
    int rowend = std::min(nRows - 1, row + PeakRadius);
    double colcentroid = 0.0;
    int colstart = std::max(0, col - PeakRadius);
    int colend = std::min(nCols - 1, col + PeakRadius);
    for (int irow = rowstart; irow <= rowend; ++irow) {
      for (int icol = colstart; icol <= colend; ++icol) {
        if (edgePixel(inst, bankName, icol, irow, Edge))
          continue;
        auto it1 = wi_to_detid_map.find(findPixelID(bankName, icol, irow));
        size_t workspaceIndex = (it1->second);
        EventList el = eventW->getSpectrum(workspaceIndex);
        el.switchTo(WEIGHTED_NOTIME);
        std::vector<WeightedEventNoTime> events = el.getWeightedEventsNoTime();

        // Check for events in tof range
        for (const auto &event : events) {
          double tof = event.tof();
          if (tof > tofstart && tof < tofend) {
            double weight = event.weight();
            intensity += weight;
            rowcentroid += irow * weight;
            colcentroid += icol * weight;
            tofcentroid += tof * weight;
          }
        }
      }
    }
    // Set pixelID to change row and col
    row = int(rowcentroid / intensity);
    boost::algorithm::clamp(row, 0, nRows - 1);
    col = int(colcentroid / intensity);
    boost::algorithm::clamp(col, 0, nCols - 1);
    if (!edgePixel(inst, bankName, col, row, Edge)) {
      peak.setDetectorID(findPixelID(bankName, col, row));

      // Set wavelength to change tof for peak object
      double tof = tofcentroid / intensity;
      Mantid::Kernel::Units::Wavelength wl;
      std::vector<double> timeflight;
      timeflight.push_back(tof);
      double scattering = peak.getScattering();
      double L1 = peak.getL1();
      double L2 = peak.getL2();
      wl.fromTOF(timeflight, timeflight, L1, L2, scattering, 0, 0, 0);
      const double lambda = timeflight[0];
      timeflight.clear();

      peak.setWavelength(lambda);
      peak.setBinCount(intensity);
    }
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  removeEdgePeaks(*peakWS);

  // Save the output
  setProperty("OutPeaksWorkspace", peakWS);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CentroidPeaks::exec() {
  inWS = getProperty("InputWorkspace");
  inst = inWS->getInstrument();
  // For quickly looking up workspace index from det id
  wi_to_detid_map = inWS->getDetectorIDToWorkspaceIndexMap();

  eventW = boost::dynamic_pointer_cast<const EventWorkspace>(inWS);
  if (eventW) {
    eventW->sortAll(TOF_SORT, nullptr);
    this->integrateEvent();
  } else {
    this->integrate();
  }
}

int CentroidPeaks::findPixelID(std::string bankName, int col, int row) {
  boost::shared_ptr<const IComponent> parent =
      inst->getComponentByName(bankName);
  if (parent->type() == "RectangularDetector") {
    boost::shared_ptr<const RectangularDetector> RDet =
        boost::dynamic_pointer_cast<const RectangularDetector>(parent);

    boost::shared_ptr<Detector> pixel = RDet->getAtXY(col, row);
    return pixel->getID();
  } else {
    std::string bankName0 = bankName;
    // Only works for WISH
    bankName0.erase(0, 4);
    std::ostringstream pixelString;
    pixelString << inst->getName() << "/" << bankName0 << "/" << bankName
                << "/tube" << std::setw(3) << std::setfill('0') << col
                << "/pixel" << std::setw(4) << std::setfill('0') << row;
    boost::shared_ptr<const Geometry::IComponent> component =
        inst->getComponentByName(pixelString.str());
    boost::shared_ptr<const Detector> pixel =
        boost::dynamic_pointer_cast<const Detector>(component);
    return pixel->getID();
  }
}

void CentroidPeaks::removeEdgePeaks(
    Mantid::DataObjects::PeaksWorkspace &peakWS) {
  int Edge = getProperty("EdgePixels");
  std::vector<int> badPeaks;
  size_t numPeaks = peakWS.getNumberPeaks();
  for (int i = 0; i < static_cast<int>(numPeaks); i++) {
    // Get a direct ref to that peak.
    const auto &peak = peakWS.getPeak(i);
    int col = peak.getCol();
    int row = peak.getRow();
    const std::string &bankName = peak.getBankName();

    if (edgePixel(inst, bankName, col, row, Edge)) {
      badPeaks.push_back(i);
    }
  }
  peakWS.removePeaks(std::move(badPeaks));
}

void CentroidPeaks::sizeBanks(const std::string &bankName, int &nCols,
                              int &nRows) {
  if (bankName == "None")
    return;
  ExperimentInfo expInfo;
  expInfo.setInstrument(inst);
  const auto &compInfo = expInfo.componentInfo();

  // Get a single bank
  auto bank = inst->getComponentByName(bankName);
  auto bankID = bank->getComponentID();
  auto allBankDetectorIndexes =
      compInfo.detectorsInSubtree(compInfo.indexOf(bankID));

  nRows = static_cast<int>(
      compInfo.componentsInSubtree(compInfo.indexOf(bankID)).size() -
      allBankDetectorIndexes.size() - 1);
  nCols = static_cast<int>(allBankDetectorIndexes.size()) / nRows;

  if (nCols * nRows != static_cast<int>(allBankDetectorIndexes.size())) {
    // Need grandchild instead of child
    nRows = static_cast<int>(
        compInfo.componentsInSubtree(compInfo.indexOf(bankID)).size() -
        allBankDetectorIndexes.size() - 2);
    nCols = static_cast<int>(allBankDetectorIndexes.size()) / nRows;
  }
}

} // namespace Crystal
} // namespace Mantid
