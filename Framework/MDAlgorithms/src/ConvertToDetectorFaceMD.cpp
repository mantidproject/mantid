// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/ConvertToDetectorFaceMD.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Unit.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using Mantid::Types::Event::TofEvent;

namespace Mantid::MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertToDetectorFaceMD)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string ConvertToDetectorFaceMD::name() const { return "ConvertToDetectorFaceMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int ConvertToDetectorFaceMD::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ConvertToDetectorFaceMD::category() const { return "MDAlgorithms\\Creation"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ConvertToDetectorFaceMD::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
                  "An input MatrixWorkspace.");
  declareProperty(std::make_unique<ArrayProperty<int>>("BankNumbers", Direction::Input),
                  "A list of the bank numbers to convert. If empty, will use "
                  "all banksMust have at least one entry.");

  // Now the box controller settings
  this->initBoxControllerProps("2", 200, 20);

  declareProperty(std::make_unique<WorkspaceProperty<IMDEventWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "Name of the output MDEventWorkspace.");
}

//----------------------------------------------------------------------------------------------
/** Convert an event list to 3/4D detector face space add it to the
 *MDEventWorkspace
 *
 * @param outWS
 * @param workspaceIndex : index into the workspace
 * @param x : x-coordinate for all output events
 * @param y : y-coordinate for all output events
 * @param bankNum : coordinate for the 4th dimension (optional)
 * @param expInfoIndex : index of the run, starting at 0
 * @param goniometerIndex : 0-based index determines the goniometer
 * settings when this event occurred
 * @param detectorID : detectorID for this event list
 */
template <class T, class MDE, size_t nd>
void ConvertToDetectorFaceMD::convertEventList(std::shared_ptr<Mantid::DataObjects::MDEventWorkspace<MDE, nd>> outWS,
                                               size_t workspaceIndex, coord_t x, coord_t y, coord_t bankNum,
                                               uint16_t expInfoIndex, uint16_t goniometerIndex, int32_t detectorID) {

  EventList &el = in_ws->getSpectrum(workspaceIndex);

  // The 3/4D DataObjects that will be added into the MDEventWorkspce
  std::vector<MDE> out_events;
  out_events.reserve(el.getNumberEvents());

  // This little dance makes the getting vector of events more general (since
  // you can't overload by return type).
  typename std::vector<T> *events_ptr;
  getEventsFrom(el, events_ptr);
  typename std::vector<T> &events = *events_ptr;

  // Iterators to start/end
  auto it = events.begin();
  auto it_end = events.end();

  for (; it != it_end; it++) {
    auto tof = static_cast<coord_t>(it->tof());
    if (nd == 3) {
      coord_t center[3] = {x, y, tof};
      out_events.emplace_back(float(it->weight()), float(it->errorSquared()), expInfoIndex, goniometerIndex, detectorID,
                              center);
    } else if (nd == 4) {
      coord_t center[4] = {x, y, tof, bankNum};
      out_events.emplace_back(static_cast<float>(it->weight()), static_cast<float>(it->errorSquared()), expInfoIndex,
                              detectorID, goniometerIndex, center);
    }
  }

  // Add them to the MDEW
  outWS->addEvents(out_events);
}

//----------------------------------------------------------------------------------------------
/** Get the list of banks, given the settings
 *
 * @return map with key = bank number; value = pointer to the rectangular
 *detector
 */
std::map<int, RectangularDetector_const_sptr> ConvertToDetectorFaceMD::getBanks() {
  Instrument_const_sptr inst = in_ws->getInstrument();

  std::vector<int> bankNums = this->getProperty("BankNumbers");
  std::sort(bankNums.begin(), bankNums.end());

  std::map<int, RectangularDetector_const_sptr> banks;

  if (bankNums.empty()) {
    // --- Find all rectangular detectors ----
    // Get all children
    std::vector<IComponent_const_sptr> comps;
    inst->getChildren(comps, true);

    for (auto &comp : comps) {
      // Retrieve it
      RectangularDetector_const_sptr det = std::dynamic_pointer_cast<const RectangularDetector>(comp);
      if (det) {
        std::string name = det->getName();
        if (name.size() < 5)
          continue;
        std::string bank = name.substr(4, name.size() - 4);
        int bankNum;
        if (Mantid::Kernel::Strings::convert(bank, bankNum))
          banks[bankNum] = det;
        g_log.debug() << "Found bank " << bank << ".\n";
      }
    }
  } else {
    // -- Find detectors using the numbers given ---
    for (auto &bankNum : bankNums) {
      std::string bankName = "bank" + Mantid::Kernel::Strings::toString(bankNum);
      IComponent_const_sptr comp = inst->getComponentByName(bankName);
      RectangularDetector_const_sptr det = std::dynamic_pointer_cast<const RectangularDetector>(comp);
      if (det)
        banks[bankNum] = det;
    }
  }

  for (auto &bank : banks) {
    RectangularDetector_const_sptr det = bank.second;
    // Track the largest detector
    if (det->xpixels() > m_numXPixels)
      m_numXPixels = det->xpixels();
    if (det->ypixels() > m_numYPixels)
      m_numYPixels = det->ypixels();
  }

  if (banks.empty())
    throw std::runtime_error("No RectangularDetectors with a name like "
                             "'bankXX' found in the instrument.");

  return banks;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ConvertToDetectorFaceMD::exec() {
  // TODO convert matrix to event as needed
  MatrixWorkspace_sptr mws = this->getProperty("InputWorkspace");

  in_ws = std::dynamic_pointer_cast<EventWorkspace>(mws);
  if (!in_ws)
    throw std::runtime_error("InputWorkspace is not an EventWorkspace");

  // Fill the map, throw if there are grouped pixels.
  m_detID_to_WI = in_ws->getDetectorIDToWorkspaceIndexVector(m_detID_to_WI_offset, true);

  // Get the map of the banks we'll display
  std::map<int, RectangularDetector_const_sptr> banks = this->getBanks();

  // Find the size in the TOF dimension
  double tof_min, tof_max;
  Axis *ax0 = in_ws->getAxis(0);
  in_ws->getXMinMax(tof_min, tof_max);
  if (ax0->getValue(0) < tof_min)
    tof_min = ax0->getValue(0);
  if (ax0->getValue(ax0->length() - 1) > tof_max)
    tof_max = ax0->getValue(ax0->length() - 1);

  // Get MDFrame of General Frame type
  Mantid::Geometry::GeneralFrame framePixel(Mantid::Geometry::GeneralFrame::GeneralFrameName, "pixel");
  Mantid::Geometry::GeneralFrame frameTOF(Mantid::Geometry::GeneralFrame::GeneralFrameName, ax0->unit()->label());

  // ------------------ Build all the dimensions ----------------------------
  MDHistoDimension_sptr dimX(new MDHistoDimension("x", "x", framePixel, static_cast<coord_t>(0),
                                                  static_cast<coord_t>(m_numXPixels), m_numXPixels));
  MDHistoDimension_sptr dimY(new MDHistoDimension("y", "y", framePixel, static_cast<coord_t>(0),
                                                  static_cast<coord_t>(m_numYPixels), m_numYPixels));
  std::string TOFname = ax0->title();
  if (TOFname.empty())
    TOFname = ax0->unit()->unitID();
  MDHistoDimension_sptr dimTOF(new MDHistoDimension(TOFname, TOFname, frameTOF, static_cast<coord_t>(tof_min),
                                                    static_cast<coord_t>(tof_max), ax0->length()));

  std::vector<IMDDimension_sptr> dims{dimX, dimY, dimTOF};

  if (banks.size() > 1) {
    Mantid::Geometry::GeneralFrame frameNumber(Mantid::Geometry::GeneralFrame::GeneralFrameName, "number");
    int min = banks.begin()->first;
    int max = banks.rbegin()->first + 1;
    MDHistoDimension_sptr dimBanks(new MDHistoDimension("bank", "bank", frameNumber, static_cast<coord_t>(min),
                                                        static_cast<coord_t>(max), max - min));
    dims.emplace_back(dimBanks);
  }

  // --------- Create the workspace with the right number of dimensions
  // ----------
  size_t nd = dims.size();
  IMDEventWorkspace_sptr outWS = MDEventFactory::CreateMDWorkspace(nd, "MDEvent");
  outWS->initGeometry(dims);
  outWS->initialize();
  this->setBoxController(outWS->getBoxController(), mws->getInstrument());
  outWS->splitBox();

  MDEventWorkspace3::sptr outWS3 = std::dynamic_pointer_cast<MDEventWorkspace3>(outWS);
  MDEventWorkspace4::sptr outWS4 = std::dynamic_pointer_cast<MDEventWorkspace4>(outWS);

  // Copy ExperimentInfo (instrument, run, sample) to the output WS
  ExperimentInfo_sptr ei(in_ws->cloneExperimentInfo());
  uint16_t expInfoIndex = outWS->addExperimentInfo(ei);
  uint16_t goniometerIndex(0);
  // ---------------- Convert each bank --------------------------------------
  for (auto &bank : banks) {
    int bankNum = bank.first;
    RectangularDetector_const_sptr det = bank.second;
    for (int x = 0; x < det->xpixels(); x++)
      for (int y = 0; y < det->ypixels(); y++) {
        // Find the workspace index for this pixel coordinate
        detid_t detID = det->getDetectorIDAtXY(x, y);
        size_t wi = m_detID_to_WI[detID + m_detID_to_WI_offset];
        if (wi >= in_ws->getNumberHistograms())
          throw std::runtime_error("Invalid workspace index found in bank " + det->getName() + "!");

        auto xPos = static_cast<coord_t>(x);
        auto yPos = static_cast<coord_t>(y);
        auto bankPos = static_cast<coord_t>(bankNum);

        EventList &el = in_ws->getSpectrum(wi);

        // We want to bind to the right templated function, so we have to know
        // the type of TofEvent contained in the EventList.
        boost::function<void()> func;
        switch (el.getEventType()) {
        case TOF:
          if (nd == 3)
            this->convertEventList<TofEvent, MDEvent<3>, 3>(outWS3, wi, xPos, yPos, bankPos, expInfoIndex,
                                                            goniometerIndex, detID);
          else if (nd == 4)
            this->convertEventList<TofEvent, MDEvent<4>, 4>(outWS4, wi, xPos, yPos, bankPos, expInfoIndex,
                                                            goniometerIndex, detID);
          break;
        case WEIGHTED:
          if (nd == 3)
            this->convertEventList<WeightedEvent, MDEvent<3>, 3>(outWS3, wi, xPos, yPos, bankPos, expInfoIndex,
                                                                 goniometerIndex, detID);
          else if (nd == 4)
            this->convertEventList<WeightedEvent, MDEvent<4>, 4>(outWS4, wi, xPos, yPos, bankPos, expInfoIndex,
                                                                 goniometerIndex, detID);
          break;
        case WEIGHTED_NOTIME:
          if (nd == 3)
            this->convertEventList<WeightedEventNoTime, MDEvent<3>, 3>(outWS3, wi, xPos, yPos, bankPos, expInfoIndex,
                                                                       goniometerIndex, detID);
          else if (nd == 4)
            this->convertEventList<WeightedEventNoTime, MDEvent<4>, 4>(outWS4, wi, xPos, yPos, bankPos, expInfoIndex,
                                                                       goniometerIndex, detID);
          break;
        default:
          throw std::runtime_error("EventList had an unexpected data type!");
        }
      }
  }

  // ---------------------- Perform all box splitting ---------------
  ThreadScheduler *ts = new ThreadSchedulerLargestCost();
  ThreadPool tp(ts);
  outWS->splitAllIfNeeded(ts);
  tp.joinAll();
  outWS->refreshCache();

  // Save the output workspace
  this->setProperty("OutputWorkspace", outWS);
}

} // namespace Mantid::MDAlgorithms
