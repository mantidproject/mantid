//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCrystal/MaskPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Strings.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <fstream>
#include <ostream>
#include <iomanip>
#include <sstream>
#include <set>

namespace Mantid {
namespace Crystal {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(MaskPeaksWorkspace)

using namespace Kernel;
using namespace API;
using namespace DataObjects;
using namespace Geometry;
using std::string;

/// Constructor
MaskPeaksWorkspace::MaskPeaksWorkspace() {}

/// Destructor
MaskPeaksWorkspace::~MaskPeaksWorkspace() {}

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void MaskPeaksWorkspace::init() {

  declareProperty(new WorkspaceProperty<MatrixWorkspace>(
                      "InputWorkspace", "", Direction::Input,
                      boost::make_shared<InstrumentValidator>()),
                  "A workspace containing one or more rectangular area "
                  "detectors. Each spectrum needs to correspond to only one "
                  "pixelID (e.g. no grouping or previous calls to "
                  "SumNeighbours).");
  declareProperty(new WorkspaceProperty<PeaksWorkspace>("InPeaksWorkspace", "",
                                                        Direction::Input),
                  "The name of the workspace that will be created. Can replace "
                  "the input workspace.");
  declareProperty(
      "XMin", -2,
      "Minimum of X (col) Range to mask peak relative to peak's center");
  declareProperty(
      "XMax", 2,
      "Maximum of X (col) Range to mask peak relative to peak's center");
  declareProperty(
      "YMin", -2,
      "Minimum of Y (row) Range to mask peak relative to peak's center");
  declareProperty(
      "YMax", 2,
      "Maximum of Y (row) Range to mask peak relative to peak's center");
  declareProperty("TOFMin", EMPTY_DBL(), "Optional(all TOF if not specified): "
                                         "Minimum TOF relative to peak's "
                                         "center TOF.");
  declareProperty("TOFMax", EMPTY_DBL(), "Optional(all TOF if not specified): "
                                         "Maximum TOF relative to peak's "
                                         "center TOF.");
}

/** Executes the algorithm
 *
 *  @throw Exception::FileError If the grouping file cannot be opened or read
 *successfully
 */
void MaskPeaksWorkspace::exec() {
  retrieveProperties();

  MantidVecPtr XValues;
  PeaksWorkspace_const_sptr peaksW = getProperty("InPeaksWorkspace");

  // To get the workspace index from the detector ID
  const detid2index_map pixel_to_wi =
      inputW->getDetectorIDToWorkspaceIndexMap();
  // Get some stuff from the input workspace
  Geometry::Instrument_const_sptr inst = inputW->getInstrument();

  // Init a table workspace
  DataObjects::TableWorkspace_sptr tablews =
      boost::shared_ptr<DataObjects::TableWorkspace>(
          new DataObjects::TableWorkspace());
  tablews->addColumn("double", "XMin");
  tablews->addColumn("double", "XMax");
  tablews->addColumn("str", "SpectraList");

  // Loop over peaks
  const std::vector<Peak> &peaks = peaksW->getPeaks();
  PARALLEL_FOR3(inputW, peaksW, tablews)
  for (int i = 0; i < static_cast<int>(peaks.size()); i++) {
    PARALLEL_START_INTERUPT_REGION
    Peak peak = peaks[i];
    // get the peak location on the detector
    double col = peak.getCol();
    double row = peak.getRow();
    int xPeak = int(col + 0.5) - 1;
    int yPeak = int(row + 0.5) - 1;
    g_log.debug() << "Generating information for peak at x=" << xPeak
                  << " y=" << yPeak << "\n";

    // the detector component for the peak will have all pixels that we mask
    const string bankName = peak.getBankName();
    if (bankName.compare("None") == 0)
      continue;
    Geometry::IComponent_const_sptr comp = inst->getComponentByName(bankName);
    if (!comp) {
      g_log.debug() << "Component " + bankName +
                           " does not exist in instrument\n";
      continue;
    }

    // determine the range in time-of-flight
    double x0;
    double xf;
    bool tofRangeSet(false);
    size_t wi = this->getWkspIndex(pixel_to_wi, comp, xPeak, yPeak);
    if (wi !=
        static_cast<size_t>(EMPTY_INT())) { // scope limit the workspace index
      this->getTofRange(x0, xf, peak.getTOF(), inputW->readX(wi));
      tofRangeSet = true;
    }

    // determine the spectrum numbers to mask
    std::set<size_t> spectra;
    for (int ix = m_xMin; ix <= m_xMax; ix++) {
      for (int iy = m_yMin; iy <= m_yMax; iy++) {
        // Find the pixel ID at that XY position on the rectangular detector
        size_t wj =
            this->getWkspIndex(pixel_to_wi, comp, xPeak + ix, yPeak + iy);
        if (wj == static_cast<size_t>(EMPTY_INT()))
          continue;
        spectra.insert(wj);
        if (!tofRangeSet) { // scope limit the workspace index
          this->getTofRange(x0, xf, peak.getTOF(), inputW->readX(wj));
          tofRangeSet = true;
        }
      }
    }

    // sanity check the results
    if (!tofRangeSet) {
      g_log.warning() << "Failed to set time-of-flight range for peak (x="
                      << xPeak << ", y=" << yPeak << ", tof=" << peak.getTOF()
                      << ")\n";
    } else if (spectra.empty()) {
      g_log.warning() << "Failed to find spectra for peak (x=" << xPeak
                      << ", y=" << yPeak << ", tof=" << peak.getTOF() << ")\n";
      continue;
    } else
      PARALLEL_CRITICAL(tablews) {
        // append to the table workspace
        API::TableRow newrow = tablews->appendRow();
        newrow << x0 << xf << Kernel::Strings::toString(spectra);
      }
    PARALLEL_END_INTERUPT_REGION
  } // end loop over peaks
  PARALLEL_CHECK_INTERUPT_REGION

  // Mask bins
  API::IAlgorithm_sptr maskbinstb =
      this->createChildAlgorithm("MaskBinsFromTable", 0.5, 1.0, true);
  maskbinstb->setProperty("InputWorkspace", inputW);
  maskbinstb->setPropertyValue("OutputWorkspace", inputW->getName());
  maskbinstb->setProperty("MaskingInformation", tablews);
  maskbinstb->execute();

  return;
}

void MaskPeaksWorkspace::retrieveProperties() {
  inputW = getProperty("InputWorkspace");

  m_xMin = getProperty("XMin");
  m_xMax = getProperty("XMax");
  if (m_xMin >= m_xMax)
    throw std::runtime_error("Must specify Xmin<Xmax");

  m_yMin = getProperty("YMin");
  m_yMax = getProperty("YMax");
  if (m_yMin >= m_yMax)
    throw std::runtime_error("Must specify Ymin<Ymax");

  // Get the value of TOF range to mask
  m_tofMin = getProperty("TOFMin");
  m_tofMax = getProperty("TOFMax");
  if ((!isEmpty(m_tofMin)) && (!isEmpty(m_tofMax))) {
    if (m_tofMin >= m_tofMax)
      throw std::runtime_error("Must specify TOFMin < TOFMax");
  } else if ((!isEmpty(m_tofMin)) ||
             (!isEmpty(m_tofMax))) // check if only one is empty
  {
    throw std::runtime_error("Must specify both TOFMin and TOFMax or neither");
  }
}

size_t MaskPeaksWorkspace::getWkspIndex(const detid2index_map &pixel_to_wi,
                                        Geometry::IComponent_const_sptr comp,
                                        const int x, const int y) {
  Geometry::RectangularDetector_const_sptr det =
      boost::dynamic_pointer_cast<const Geometry::RectangularDetector>(comp);
  if (det) {
    if (x >= det->xpixels() || x < 0 || y >= det->ypixels() || y < 0)
      return EMPTY_INT();
    if ((x >= det->xpixels()) ||
        (x < 0) // this check is unnecessary as callers are doing it too
        || (y >= det->ypixels()) ||
        (y < 0)) // but just to make debugging easier
    {
      std::stringstream msg;
      msg << "Failed to find workspace index for x=" << x << " y=" << y
          << "(max x=" << det->xpixels() << ", max y=" << det->ypixels() << ")";
      throw std::runtime_error(msg.str());
    }

    int pixelID = det->getAtXY(x, y)->getID();

    // Find the corresponding workspace index, if any
    auto wiEntry = pixel_to_wi.find(pixelID);
    if (wiEntry == pixel_to_wi.end()) {
      std::stringstream msg;
      msg << "Failed to find workspace index for x=" << x << " y=" << y;
      throw std::runtime_error(msg.str());
    }
    return wiEntry->second;
  } else {
    std::vector<Geometry::IComponent_const_sptr> children;
    boost::shared_ptr<const Geometry::ICompAssembly> asmb =
        boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(comp);
    asmb->getChildren(children, false);
    boost::shared_ptr<const Geometry::ICompAssembly> asmb2 =
        boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(children[0]);
    std::vector<Geometry::IComponent_const_sptr> grandchildren;
    asmb2->getChildren(grandchildren, false);
    int NROWS = static_cast<int>(grandchildren.size());
    int NCOLS = static_cast<int>(children.size());
    // Wish pixels and tubes start at 1 not 0
    if (x - 1 >= NCOLS || x - 1 < 0 || y - 1 >= NROWS || y - 1 < 0)
      return EMPTY_INT();
    std::string bankName = comp->getName();
    detid2index_map::const_iterator it =
        pixel_to_wi.find(findPixelID(bankName, x, y));
    if (it == pixel_to_wi.end())
      return EMPTY_INT();
    return (it->second);
  }
  return EMPTY_INT();
}

/**
 * @param tofMin Return value for minimum tof to be masked
 * @param tofMax Return value for maximum tof to be masked
 * @param tofPeak time-of-flight of the single crystal peak
 * @param tof tof-of-flight axis for the spectrum where the peak supposedly
 * exists
 */
void MaskPeaksWorkspace::getTofRange(double &tofMin, double &tofMax,
                                     const double tofPeak,
                                     const MantidVec &tof) {
  tofMin = tof.front();
  tofMax = tof.back() - 1;
  if (!isEmpty(m_tofMin)) {
    tofMin = tofPeak + m_tofMin;
  }
  if (!isEmpty(m_tofMax)) {
    tofMax = tofPeak + m_tofMax;
  }
}
int MaskPeaksWorkspace::findPixelID(std::string bankName, int col, int row) {
  Geometry::Instrument_const_sptr Iptr = inputW->getInstrument();
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
} // namespace Crystal
} // namespace Mantid
