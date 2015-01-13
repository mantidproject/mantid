#include "MantidDataObjects/SpecialWorkspace2D.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/SpectraAxis.h"

#include <fstream>
#include <sstream>

using Mantid::API::SpectraAxis;
using std::set;
using std::size_t;

namespace Mantid {
namespace DataObjects {

namespace {
Kernel::Logger g_log("SpecialWorkspace2D");
}

// Register the workspace
DECLARE_WORKSPACE(SpecialWorkspace2D)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SpecialWorkspace2D::SpecialWorkspace2D() {}

//----------------------------------------------------------------------------------------------
/** Constructor, building from an instrument
 *
 * @param inst :: input instrument that is the base for this workspace
 * @param includeMonitors :: If false the monitors are not included
 * @return created SpecialWorkspace2D
 */
SpecialWorkspace2D::SpecialWorkspace2D(Geometry::Instrument_const_sptr inst,
                                       const bool includeMonitors) {
  // Init the Workspace2D with one spectrum per detector, in the same order.
  this->init(inst->getNumberDetectors(!includeMonitors), 1, 1);

  // Copy the instrument
  this->setInstrument(inst);

  // Initialize the spectra-det-map, 1:1 between spectrum number and det ID
  this->MatrixWorkspace::rebuildSpectraMapping(includeMonitors);

  // Make the mapping, which will be used for speed later.
  detID_to_WI.clear();
  for (size_t wi = 0; wi < m_noVectors; wi++) {
    set<detid_t> dets = getSpectrum(wi)->getDetectorIDs();
    for (auto det = dets.begin(); det != dets.end(); ++det) {
      detID_to_WI[*det] = wi;
    }
  }
}

//----------------------------------------------------------------------------------------------
/** Constructor, building from a MatrixWorkspace
 *
 * @param parent :: input workspace that is the base for this workspace
 * @return created SpecialWorkspace2D
 */
SpecialWorkspace2D::SpecialWorkspace2D(API::MatrixWorkspace_const_sptr parent) {
  this->init(parent->getNumberHistograms(), 1, 1);
  API::WorkspaceFactory::Instance().initializeFromParent(
      parent, API::MatrixWorkspace_sptr(this, Mantid::NoDeleting()), false);
  // Make the mapping, which will be used for speed later.
  detID_to_WI.clear();
  for (size_t wi = 0; wi < m_noVectors; wi++) {
    set<detid_t> dets = getSpectrum(wi)->getDetectorIDs();
    for (auto det = dets.begin(); det != dets.end(); ++det) {
      detID_to_WI[*det] = wi;
    }
  }
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
SpecialWorkspace2D::~SpecialWorkspace2D() {}

//----------------------------------------------------------------------------------------------
/** Sets the size of the workspace and initializes arrays to zero
*  @param NVectors :: The number of vectors/histograms/detectors in the
* workspace
*  @param XLength :: Must be 1
*  @param YLength :: Must be 1
*/
void SpecialWorkspace2D::init(const size_t &NVectors, const size_t &XLength,
                              const size_t &YLength) {
  if ((XLength != 1) || (YLength != 1))
    throw std::invalid_argument(
        "SpecialWorkspace2D must have 'spectra' of length 1 only.");
  // Continue with standard initialization
  Workspace2D::init(NVectors, XLength, YLength);
}

/**
 * @return A string containing the workspace description
 */
const std::string SpecialWorkspace2D::toString() const {
  std::ostringstream os;
  os << "Title: " << getTitle() << "\n";
  os << "Histograms: " << getNumberHistograms() << "\n";
  return os.str();
}

//----------------------------------------------------------------------------------------------
/** Return the special value (Y) in the workspace at the given detector ID
 *
 * @param detectorID :: detector ID to look up
 * @return the Y value for that detector ID.
 * @throw std::invalid_argument if the detector ID was not found
 */
double SpecialWorkspace2D::getValue(const detid_t detectorID) const {
  std::map<detid_t, size_t>::const_iterator it = detID_to_WI.find(detectorID);

  if (it == detID_to_WI.end()) {
    std::ostringstream os;
    os << "SpecialWorkspace2D: " << this->getName()
       << "  Detector ID = " << detectorID
       << "  Size(Map) = " << this->detID_to_WI.size() << std::endl;
    throw std::invalid_argument(os.str());
  } else {
    return this->dataY(it->second)[0];
  }
}

//----------------------------------------------------------------------------------------------
/** Return the special value (Y) in the workspace at the given detector ID,
 * but returns a default value instead of throwing if detector is not found.
 *
 * @param detectorID :: detector ID to look up
 * @param defaultValue :: value returned if the ID is not found.
 * @return the Y value for that detector ID.
 */
double SpecialWorkspace2D::getValue(const detid_t detectorID,
                                    const double defaultValue) const {
  std::map<detid_t, size_t>::const_iterator it = detID_to_WI.find(detectorID);
  if (it == detID_to_WI.end())
    return defaultValue;
  else {
    if (it->second < m_noVectors) // don't let it generate an exception
    {
      return this->dataY(it->second)[0];
    } else {
      g_log.debug() << "getValue(" << detectorID << "->" << (it->second) << ", "
                    << defaultValue << ") index out of range\n";
      return defaultValue;
    }
  }
}

//----------------------------------------------------------------------------------------------
/** Set the special value (Y) in the workspace at the given detector ID
 *
 * @param detectorID :: detector ID to look up
 * @param value :: holder for the Y value
 * @param error :: the Y value for that detector ID.
 * @throw std::invalid_argument if the detector ID was not found
 */
void SpecialWorkspace2D::setValue(const detid_t detectorID, const double value,
                                  const double error) {
  std::map<detid_t, size_t>::iterator it = detID_to_WI.find(detectorID);
  if (it == detID_to_WI.end()) {
    std::stringstream msg;
    msg << "SpecialWorkspace2D::setValue(): Input Detector ID = " << detectorID
        << " Is Invalid";
    throw std::invalid_argument(msg.str());
  } else {
    this->dataY(it->second)[0] = value;
    this->dataE(it->second)[0] = error;
  }
}

/**
 * Set the special value (Y) in the workspace at the given detector IDs. This is
 *a
 * convenience function that just calls setValue(const detid_t, double, double).
 *
 * @param detectorIDs :: detector IDs to look up
 * @param value :: holder for the Y value
 * @param error :: the Y value for the detector IDs.
 * @throw std::invalid_argument if any of the detector IDs are not found
 */
void SpecialWorkspace2D::setValue(const set<detid_t> &detectorIDs,
                                  const double value, const double error) {
  for (auto detID = detectorIDs.begin(); detID != detectorIDs.end(); ++detID) {
    this->setValue(*detID, value, error);
  }
}

//----------------------------------------------------------------------------------------------
/** Return the detector ID at the given workspace index (i.e.,
 *spectrum/histogram index)
 *
 * @param workspaceIndex
 * @return
 */
set<detid_t>
SpecialWorkspace2D::getDetectorIDs(const std::size_t workspaceIndex) const {
  if (size_t(workspaceIndex) > this->getNumberHistograms())
    throw std::invalid_argument(
        "SpecialWorkspace2D::getDetectorID(): Invalid workspaceIndex given.");
  return this->getSpectrum(workspaceIndex)->getDetectorIDs();
}

//--------------------------------------------------------------------------------------------
/** Return the result of operator &
 * @ parameter
 * @ return
 */
void SpecialWorkspace2D::binaryOperation(
    boost::shared_ptr<const SpecialWorkspace2D> &ws,
    const unsigned int operatortype) {

  // 1. Check compatibility between this and input workspace
  if (!this->isCompatible(ws)) {
    throw std::invalid_argument(
        "Two SpecialWorkspace2D objects are not compatible!");
  }

  switch (operatortype) {
  case BinaryOperator::AND:
    this->binaryAND(ws);
    break;
  case BinaryOperator::OR:
    this->binaryOR(ws);
    break;
  case BinaryOperator::XOR:
    this->binaryXOR(ws);
    break;
  default:
    throw std::invalid_argument("Invalid Operator");
    break;
  }

  return;
}

//--------------------------------------------------------------------------------------------
/** Return the result of operator &
 * @ parameter
 * @ return
 */
void SpecialWorkspace2D::binaryOperation(const unsigned int operatortype) {

  switch (operatortype) {
  case BinaryOperator::NOT:
    this->binaryNOT();
    break;
  default:
    g_log.error() << "Operator " << operatortype
                  << " Is Not Valid In BinaryOperation(operatortype)"
                  << std::endl;
    throw std::invalid_argument("Invalid Operator");
    break;
  }

  return;
}

/** AND operator
 *
 */
void
SpecialWorkspace2D::binaryAND(boost::shared_ptr<const SpecialWorkspace2D> ws) {

  for (size_t i = 0; i < this->getNumberHistograms(); i++) {
    double y1 = this->dataY(i)[0];
    double y2 = ws->dataY(i)[0];

    if (y1 < 1.0E-10 || y2 < 1.0E-10) {
      this->dataY(i)[0] = 0.0;
    } else {
      this->dataY(i)[0] += y2;
    }
  }

  return;
}

/** OR operator
 *
 */
void
SpecialWorkspace2D::binaryOR(boost::shared_ptr<const SpecialWorkspace2D> ws) {

  for (size_t i = 0; i < this->getNumberHistograms(); i++) {
    double y1 = this->dataY(i)[0];
    double y2 = ws->dataY(i)[0];

    double max = y1;
    if (y2 > y1) {
      max = y2;
    }
    this->dataY(i)[0] = max;

    /*
if (y1 < 1.0E-10 && y2 < 1.0E-10){
  this->dataY(i)[0] = 0.0;
} else {
  this->dataY(i)[0] += y2;
}
*/
  }

  return;
}

/** Excluded Or operator
 *
 */
void
SpecialWorkspace2D::binaryXOR(boost::shared_ptr<const SpecialWorkspace2D> ws) {

  for (size_t i = 0; i < this->getNumberHistograms(); i++) {
    double y1 = this->dataY(i)[0];
    double y2 = ws->dataY(i)[0];
    if (y1 < 1.0E-10 && y2 < 1.0E-10) {
      this->dataY(i)[0] = 0.0;
    } else if (y1 > 1.0E-10 && y2 > 1.0E-10) {
      this->dataY(i)[0] = 0.0;
    } else {
      this->dataY(i)[0] = 1.0;
    }
  }

  return;
}

/**
 *  NOT operator
 */
void SpecialWorkspace2D::binaryNOT() {

  for (size_t i = 0; i < this->getNumberHistograms(); i++) {
    double y1 = this->dataY(i)[0];
    if (y1 < 1.0E-10) {
      this->dataY(i)[0] = 1.0;
    } else {
      this->dataY(i)[0] = 0.0;
    }
  }

  return;
}

//----------------------------------------------------------------------------------------------
/* Check 2 SpecialWorkspace2D are compatible
 * @ parameter
 * @ return
 */
bool SpecialWorkspace2D::isCompatible(
    boost::shared_ptr<const SpecialWorkspace2D> ws) {

  // 1. Check number of histogram
  size_t numhist1 = this->getNumberHistograms();
  size_t numhist2 = ws->getNumberHistograms();
  if (numhist1 != numhist2) {
    g_log.debug() << "2 Workspaces have different number of histograms:  "
                  << numhist1 << "  vs. " << numhist2 << std::endl;
    return false;
  }

  // 2. Check detector ID
  for (size_t ispec = 0; ispec < numhist1; ispec++) {
    set<detid_t> ids1 = this->getSpectrum(ispec)->getDetectorIDs();
    set<detid_t> ids2 = ws->getSpectrum(ispec)->getDetectorIDs();

    if (ids1.size() != ids2.size()) {
      g_log.debug() << "Spectra " << ispec
                    << ": 2 Workspaces have different number of detectors "
                    << ids1.size() << " vs. " << ids2.size() << std::endl;
      return false;
    } else if (ids1.empty()) {
      g_log.debug() << "Spectra " << ispec
                    << ": 2 Workspaces both have 0 detectors. " << std::endl;
      return false;
    } else if (*ids1.begin() != *ids2.begin()) {
      g_log.debug() << "Spectra " << ispec
                    << ": 2 Workspaces have different Detector ID "
                    << *ids1.begin() << " vs. " << *ids2.begin() << std::endl;
      return false;
    }
  } // false

  return true;
}

//----------------------------------------------------------------------------------------------
/** Duplicate SpecialWorkspace2D
  */
void SpecialWorkspace2D::copyFrom(
    boost::shared_ptr<const SpecialWorkspace2D> sourcews) {
  // Check
  if (this->getNumberHistograms() != sourcews->getNumberHistograms()) {
    throw std::invalid_argument("Incompatible number of histograms");
  }

  // Copy data
  for (size_t ispec = 0; ispec < this->getNumberHistograms(); ispec++) {

    // 1.1 Check size
    const MantidVec &inx = sourcews->readX(ispec);
    const MantidVec &iny = sourcews->readY(ispec);
    const MantidVec &ine = sourcews->readE(ispec);

    MantidVec &outx = this->dataX(ispec);
    MantidVec &outy = this->dataY(ispec);
    MantidVec &oute = this->dataE(ispec);

    if (inx.size() != outx.size() || iny.size() != outy.size() ||
        ine.size() != oute.size()) {
      throw std::invalid_argument("X, Y, E size different within spectrum");
    }

    // 1.2 Copy data
    for (size_t i = 0; i < inx.size(); i++) {
      outx[i] = inx[i];
    }
    for (size_t i = 0; i < iny.size(); i++) {
      outy[i] = iny[i];
      oute[i] = ine[i];
    }
  }

  // Copy detector map
  this->detID_to_WI = sourcews->detID_to_WI;

  return;
}

} // namespace Mantid
} // namespace DataObjects

/// @cond TEMPLATE

namespace Mantid {
namespace Kernel {

template <>
DLLExport Mantid::DataObjects::SpecialWorkspace2D_sptr
IPropertyManager::getValue<Mantid::DataObjects::SpecialWorkspace2D_sptr>(
    const std::string &name) const {
  PropertyWithValue<Mantid::DataObjects::SpecialWorkspace2D_sptr> *prop =
      dynamic_cast<
          PropertyWithValue<Mantid::DataObjects::SpecialWorkspace2D_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message = "Attempt to assign property " + name +
                          " to incorrect type. Expected SpecialWorkspace2D.";
    throw std::runtime_error(message);
  }
}

template <>
DLLExport Mantid::DataObjects::SpecialWorkspace2D_const_sptr
IPropertyManager::getValue<Mantid::DataObjects::SpecialWorkspace2D_const_sptr>(
    const std::string &name) const {
  PropertyWithValue<Mantid::DataObjects::SpecialWorkspace2D_sptr> *prop =
      dynamic_cast<
          PropertyWithValue<Mantid::DataObjects::SpecialWorkspace2D_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return prop->operator()();
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected const SpecialWorkspace2D.";
    throw std::runtime_error(message);
  }
}

} // namespace Kernel
} // namespace Mantid

/// @endcond TEMPLATE
