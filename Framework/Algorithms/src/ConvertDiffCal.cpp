#include "MantidAlgorithms/ConvertDiffCal.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/IComponent.h"

namespace Mantid {
namespace Algorithms {

using Mantid::Kernel::Direction;
using Mantid::API::IAlgorithm_sptr;
using Mantid::API::ISpectrum;
using Mantid::API::ITableWorkspace;
using Mantid::API::ITableWorkspace_sptr;
using Mantid::API::Progress;
using Mantid::API::WorkspaceProperty;
using Mantid::DataObjects::OffsetsWorkspace;
using Mantid::DataObjects::OffsetsWorkspace_const_sptr;
using Mantid::DataObjects::TableWorkspace;
using Mantid::DataObjects::TableWorkspace_sptr;
using Mantid::Geometry::Instrument;
using Mantid::Geometry::Instrument_const_sptr;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertDiffCal)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string ConvertDiffCal::name() const { return "ConvertDiffCal"; }

/// Algorithm's version for identification. @see Algorithm::version
int ConvertDiffCal::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ConvertDiffCal::category() const {
  return "Diffraction\\Utility";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ConvertDiffCal::summary() const {
  return "Convert diffraction calibration from old to new style";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ConvertDiffCal::init() {
  declareProperty(Kernel::make_unique<WorkspaceProperty<OffsetsWorkspace>>(
                      "OffsetsWorkspace", "", Direction::Input),
                  "OffsetsWorkspace containing the calibration offsets.");
  declareProperty(Kernel::make_unique<WorkspaceProperty<ITableWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

/**
 * @throws std::logic_error if there is more than one detector id
 * for the spectrum.
 * @param offsetsWS
 * @param index
 * @return The proper detector id.
 */
detid_t getDetID(OffsetsWorkspace_const_sptr offsetsWS, const size_t index) {
  auto detIDs = offsetsWS->getSpectrum(index).getDetectorIDs();
  if (detIDs.size() != 1) {
    std::stringstream msg;
    msg << "Encountered spectrum with multiple detector ids (size="
        << detIDs.size() << ")";
    throw std::logic_error(msg.str());
  }
  return (*(detIDs.begin()));
}

/**
 * @throws std::logic_error if the offset found is non-physical.
 * @param offsetsWS
 * @param detid
 * @return The offset value or zero if not specified.
 */
double getOffset(OffsetsWorkspace_const_sptr offsetsWS, const detid_t detid) {
  const double offset = offsetsWS->getValue(detid, 0.0);
  if (offset <= -1.) { // non-physical
    std::stringstream msg;
    msg << "Encountered offset of " << offset
        << " which converts data to negative d-spacing for detectorID " << detid
        << "\n";
    throw std::logic_error(msg.str());
  }
  return offset;
}
namespace {

//------------------------------------------------------------------------------------------------
/** Get several instrument parameters used in tof to D-space conversion
 *
 */
void getInstrumentParameters(const Instrument &instrument, double &l1,
                             Kernel::V3D &beamline, double &beamline_norm,
                             Kernel::V3D &samplePos) {
  using namespace Mantid::Geometry;
  // Get some positions
  const IComponent_const_sptr sourceObj = instrument.getSource();
  if (sourceObj == nullptr) {
    throw Mantid::Kernel::Exception::InstrumentDefinitionError(
        "Failed to get source component from instrument");
  }
  const Kernel::V3D sourcePos = sourceObj->getPos();
  samplePos = instrument.getSample()->getPos();
  beamline = samplePos - sourcePos;
  beamline_norm = 2.0 * beamline.norm();

  // Get the distance between the source and the sample (assume in metres)
  IComponent_const_sptr sample = instrument.getSample();
  try {
    l1 = instrument.getSource()->getDistance(*sample);
  } catch (Mantid::Kernel::Exception::NotFoundError &) {
    throw Mantid::Kernel::Exception::InstrumentDefinitionError(
        "Unable to calculate source-sample distance ", instrument.getName());
  }
}
double calcConversion(const double l1, const Kernel::V3D &beamline,
                      const double beamline_norm, const Kernel::V3D &samplePos,
                      const Kernel::V3D &detPos, const double offset) {
  if (offset <=
      -1.) // not physically possible, means result is negative d-spacing
  {
    std::stringstream msg;
    msg << "Encountered offset of " << offset
        << " which converts data to negative d-spacing\n";
    throw std::logic_error(msg.str());
  }

  // Now detPos will be set with respect to samplePos
  Kernel::V3D relDetPos = detPos - samplePos;
  // 0.5*cos(2theta)
  double l2 = relDetPos.norm();
  double halfcosTwoTheta =
      relDetPos.scalar_prod(beamline) / (l2 * beamline_norm);
  // This is sin(theta)
  double sinTheta = sqrt(0.5 - halfcosTwoTheta);
  const double numerator = (1.0 + offset);
  sinTheta *= (l1 + l2);
  const double CONSTANT = (PhysicalConstants::h * 1e10) /
                          (2.0 * PhysicalConstants::NeutronMass * 1e6);

  return (numerator * CONSTANT) / sinTheta;
}

//-----------------------------------------------------------------------
/** Calculate the conversion factor (tof -> d-spacing)
 * for a LIST of detectors assigned to a single spectrum.
 */
double calcConversion(const double l1, const Kernel::V3D &beamline,
                      const double beamline_norm, const Kernel::V3D &samplePos,
                      const boost::shared_ptr<const Instrument> &instrument,
                      const std::vector<detid_t> &detectors,
                      const std::map<detid_t, double> &offsets) {
  double factor = 0.;
  double offset;
  for (auto detector : detectors) {
    auto off_iter = offsets.find(detector);
    if (off_iter != offsets.cend()) {
      offset = offsets.find(detector)->second;
    } else {
      offset = 0.;
    }
    factor +=
        calcConversion(l1, beamline, beamline_norm, samplePos,
                       instrument->getDetector(detector)->getPos(), offset);
  }
  return factor / static_cast<double>(detectors.size());
}
}

/**
 * @param offsetsWS
 * @param index
 * @return The offset adjusted value of DIFC
 */
double calculateDIFC(OffsetsWorkspace_const_sptr offsetsWS,
                     const size_t index) {
  Instrument_const_sptr instrument = offsetsWS->getInstrument();

  const detid_t detid = getDetID(offsetsWS, index);
  const double offset = getOffset(offsetsWS, detid);

  double l1;
  Kernel::V3D beamline, samplePos;
  double beamline_norm;
  getInstrumentParameters(*instrument, l1, beamline, beamline_norm, samplePos);

  Geometry::IDetector_const_sptr detector = instrument->getDetector(detid);

  // the factor returned is what is needed to convert TOF->d-spacing
  // the table is supposed to be filled with DIFC which goes the other way
  const double factor = calcConversion(l1, beamline, beamline_norm, samplePos,
                                       detector->getPos(), offset);
  return 1. / factor;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ConvertDiffCal::exec() {
  OffsetsWorkspace_const_sptr offsetsWS = getProperty("OffsetsWorkspace");

  // initial setup of new style config
  ITableWorkspace_sptr configWksp = boost::make_shared<TableWorkspace>();
  configWksp->addColumn("int", "detid");
  configWksp->addColumn("double", "difc");
  configWksp->addColumn("double", "difa");
  configWksp->addColumn("double", "tzero");

  // create values in the table
  const size_t numberOfSpectra = offsetsWS->getNumberHistograms();
  Progress progress(this, 0.0, 1.0, numberOfSpectra);

  for (size_t i = 0; i < numberOfSpectra; ++i) {
    API::TableRow newrow = configWksp->appendRow();
    newrow << static_cast<int>(getDetID(offsetsWS, i));
    newrow << calculateDIFC(offsetsWS, i);
    newrow << 0.; // difa
    newrow << 0.; // tzero

    progress.report();
  }

  // sort the results
  IAlgorithm_sptr sortTable = createChildAlgorithm("SortTableWorkspace");
  sortTable->setProperty("InputWorkspace", configWksp);
  sortTable->setProperty("OutputWorkspace", configWksp);
  sortTable->setPropertyValue("Columns", "detid");
  sortTable->executeAsChildAlg();

  // copy over the results
  configWksp = sortTable->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", configWksp);
}

} // namespace Algorithms
} // namespace Mantid
