#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidCrystal/CentroidPeaks.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

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
/** Constructor
 */
CentroidPeaks::CentroidPeaks() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
CentroidPeaks::~CentroidPeaks() {}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CentroidPeaks::init() {
  declareProperty(new WorkspaceProperty<PeaksWorkspace>("InPeaksWorkspace", "",
                                                        Direction::Input),
                  "A PeaksWorkspace containing the peaks to centroid.");

  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "An input 2D Workspace.");

  declareProperty(
      new PropertyWithValue<int>("PeakRadius", 10, Direction::Input),
      "Fixed radius around each peak position in which to calculate the "
      "centroid.");

  declareProperty(new PropertyWithValue<int>("EdgePixels", 0, Direction::Input),
                  "The number of pixels where peaks are removed at edges. Only "
                  "for instruments with RectangularDetectors. ");

  declareProperty(
      new WorkspaceProperty<PeaksWorkspace>("OutPeaksWorkspace", "",
                                            Direction::Output),
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
  for (int i = 0; i < NumberPeaks; i++) {
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

  int Edge = getProperty("EdgePixels");
  Progress prog(this, MinPeaks, 1.0, MaxPeaks);
  PARALLEL_FOR2(inWS, peakWS)
  for (int i = MinPeaks; i <= MaxPeaks; i++) {
    PARALLEL_START_INTERUPT_REGION
    // Get a direct ref to that peak.
    IPeak &peak = peakWS->getPeak(i);
    int col = peak.getCol();
    int row = peak.getRow();
    int pixelID = peak.getDetectorID();
    detid2index_map::const_iterator it = wi_to_detid_map.find(pixelID);
    if (it == wi_to_detid_map.end()) {
      continue;
    }
    size_t workspaceIndex = it->second;
    double TOFPeakd = peak.getTOF();
    const MantidVec &X = inWS->readX(workspaceIndex);
    int chan = Kernel::VectorHelper::getBinIndex(X, TOFPeakd);
    std::string bankName = peak.getBankName();

    double intensity = 0.0;
    double chancentroid = 0.0;

    int chanstart = std::max(0, chan - PeakRadius);
    int chanend = std::min(static_cast<int>(X.size()), chan + PeakRadius);
    double rowcentroid = 0.0;
    int rowstart = std::max(0, row - PeakRadius);
    int rowend = row + PeakRadius;
    double colcentroid = 0.0;
    int colstart = col - PeakRadius;
    int colend = col + PeakRadius;
    for (int ichan = chanstart; ichan <= chanend; ++ichan) {
      for (int irow = rowstart; irow <= rowend; ++irow) {
        for (int icol = colstart; icol <= colend; ++icol) {
          if (edgePixel(bankName, icol, irow, Edge))
            continue;
          detid2index_map::const_iterator it =
              wi_to_detid_map.find(findPixelID(bankName, icol, irow));
          if (it == wi_to_detid_map.end())
            continue;
          size_t workspaceIndex = (it->second);

          const MantidVec &histogram = inWS->readY(workspaceIndex);

          intensity += histogram[ichan];
          rowcentroid += irow * histogram[ichan];
          colcentroid += icol * histogram[ichan];
          chancentroid += ichan * histogram[ichan];
        }
      }
    }
    // Set pixelID to change row and col
    row = int(rowcentroid / intensity);
    row = std::max(0, row);
    col = int(colcentroid / intensity);
    col = std::max(0, col);
    chan = int(chancentroid / intensity);
    chan = std::max(0, chan);
    chan = std::min(static_cast<int>(inWS->blocksize()), chan);

    peak.setDetectorID(findPixelID(bankName, col, row));
    // Set wavelength to change tof for peak object
    if (!edgePixel(bankName, col, row, Edge)) {
      it = wi_to_detid_map.find(findPixelID(bankName, col, row));
      workspaceIndex = (it->second);
      Mantid::Kernel::Units::Wavelength wl;
      std::vector<double> timeflight;
      timeflight.push_back(inWS->readX(workspaceIndex)[chan]);
      double scattering = peak.getScattering();
      double L1 = peak.getL1();
      double L2 = peak.getL2();
      wl.fromTOF(timeflight, timeflight, L1, L2, scattering, 0, 0, 0);
      const double lambda = timeflight[0];
      timeflight.clear();

      peak.setWavelength(lambda);
      peak.setBinCount(inWS->readY(workspaceIndex)[chan]);
    }
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  for (int i = int(peakWS->getNumberPeaks()) - 1; i >= 0; --i) {
    // Get a direct ref to that peak.
    IPeak &peak = peakWS->getPeak(i);
    int col = peak.getCol();
    int row = peak.getRow();
    std::string bankName = peak.getBankName();

    if (edgePixel(bankName, col, row, Edge)) {
      peakWS->removePeak(i);
    }
  }

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
  for (int i = 0; i < NumberPeaks; i++) {
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

  int Edge = getProperty("EdgePixels");
  Progress prog(this, MinPeaks, 1.0, MaxPeaks);
  PARALLEL_FOR2(inWS, peakWS)
  for (int i = MinPeaks; i <= MaxPeaks; i++) {
    PARALLEL_START_INTERUPT_REGION
    // Get a direct ref to that peak.
    IPeak &peak = peakWS->getPeak(i);
    int col = peak.getCol();
    int row = peak.getRow();
    double TOFPeakd = peak.getTOF();
    std::string bankName = peak.getBankName();

    double intensity = 0.0;
    double tofcentroid = 0.0;
    if (edgePixel(bankName, col, row, Edge))
      continue;
    Mantid::detid2index_map::iterator it;
    it = wi_to_detid_map.find(findPixelID(bankName, col, row));

    double tofstart = TOFPeakd * std::pow(1.004, -PeakRadius);
    double tofend = TOFPeakd * std::pow(1.004, PeakRadius);
    double rowcentroid = 0.0;
    int rowstart = std::max(0, row - PeakRadius);
    int rowend = row + PeakRadius;
    double colcentroid = 0.0;
    int colstart = std::max(0, col - PeakRadius);
    int colend = col + PeakRadius;
    for (int irow = rowstart; irow <= rowend; ++irow) {
      for (int icol = colstart; icol <= colend; ++icol) {
        Mantid::detid2index_map::iterator it;
        if (edgePixel(bankName, icol, irow, Edge))
          continue;
        it = wi_to_detid_map.find(findPixelID(bankName, icol, irow));
        size_t workspaceIndex = (it->second);
        EventList el = eventW->getEventList(workspaceIndex);
        el.switchTo(WEIGHTED_NOTIME);
        std::vector<WeightedEventNoTime> events = el.getWeightedEventsNoTime();

        std::vector<WeightedEventNoTime>::iterator itev;
        std::vector<WeightedEventNoTime>::iterator itev_end = events.end();

        // Check for events in tof range
        for (itev = events.begin(); itev != itev_end; ++itev) {
          double tof = itev->tof();
          if (tof > tofstart && tof < tofend) {
            double weight = itev->weight();
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
    row = std::max(0, row);
    col = int(colcentroid / intensity);
    col = std::max(0, col);
    if (!edgePixel(bankName, col, row, Edge)) {
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

  for (int i = int(peakWS->getNumberPeaks()) - 1; i >= 0; --i) {
    // Get a direct ref to that peak.
    IPeak &peak = peakWS->getPeak(i);
    int col = peak.getCol();
    int row = peak.getRow();
    std::string bankName = peak.getBankName();

    if (edgePixel(bankName, col, row, Edge)) {
      peakWS->removePeak(i);
    }
  }
  // Save the output
  setProperty("OutPeaksWorkspace", peakWS);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CentroidPeaks::exec() {
  inWS = getProperty("InputWorkspace");

  // For quickly looking up workspace index from det id
  wi_to_detid_map = inWS->getDetectorIDToWorkspaceIndexMap();

  eventW = boost::dynamic_pointer_cast<const EventWorkspace>(inWS);
  if (eventW) {
    eventW->sortAll(TOF_SORT, NULL);
    this->integrateEvent();
  } else {
    this->integrate();
  }
}
int CentroidPeaks::findPixelID(std::string bankName, int col, int row) {
  Geometry::Instrument_const_sptr Iptr = inWS->getInstrument();
  boost::shared_ptr<const IComponent> parent =
      Iptr->getComponentByName(bankName);
  if (parent->type().compare("RectangularDetector") == 0) {
    boost::shared_ptr<const RectangularDetector> RDet =
        boost::dynamic_pointer_cast<const RectangularDetector>(parent);

    boost::shared_ptr<Detector> pixel = RDet->getAtXY(col, row);
    return pixel->getID();
  } else {
    std::string bankName0 = bankName;
    // Only works for WISH
    bankName0.erase(0, 4);
    std::ostringstream pixelString;
    pixelString << Iptr->getName() << "/" << bankName0 << "/" << bankName
                << "/tube" << std::setw(3) << std::setfill('0') << col
                << "/pixel" << std::setw(4) << std::setfill('0') << row;
    boost::shared_ptr<const Geometry::IComponent> component =
        Iptr->getComponentByName(pixelString.str());
    boost::shared_ptr<const Detector> pixel =
        boost::dynamic_pointer_cast<const Detector>(component);
    return pixel->getID();
  }
}
bool CentroidPeaks::edgePixel(std::string bankName, int col, int row,
                              int Edge) {
  if (bankName.compare("None") == 0)
    return false;
  Geometry::Instrument_const_sptr Iptr = inWS->getInstrument();
  boost::shared_ptr<const IComponent> parent =
      Iptr->getComponentByName(bankName);
  if (parent->type().compare("RectangularDetector") == 0) {
    boost::shared_ptr<const RectangularDetector> RDet =
        boost::dynamic_pointer_cast<const RectangularDetector>(parent);

    if (col < Edge || col >= (RDet->xpixels() - Edge) || row < Edge ||
        row >= (RDet->ypixels() - Edge))
      return true;
    else
      return false;
  } else {
    std::vector<Geometry::IComponent_const_sptr> children;
    boost::shared_ptr<const Geometry::ICompAssembly> asmb =
        boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(parent);
    asmb->getChildren(children, false);
    boost::shared_ptr<const Geometry::ICompAssembly> asmb2 =
        boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(children[0]);
    std::vector<Geometry::IComponent_const_sptr> grandchildren;
    asmb2->getChildren(grandchildren, false);
    int NROWS = static_cast<int>(grandchildren.size());
    int NCOLS = static_cast<int>(children.size());
    // Wish pixels and tubes start at 1 not 0
    if (col - 1 < Edge || col - 1 >= (NCOLS - Edge) || row - 1 < Edge ||
        row - 1 >= (NROWS - Edge))
      return true;
    else
      return false;
  }
  return false;
}

} // namespace Mantid
} // namespace Crystal
