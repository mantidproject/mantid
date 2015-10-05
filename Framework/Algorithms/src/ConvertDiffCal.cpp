#include "MantidAlgorithms/ConvertDiffCal.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/Instrument.h"

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
/** Constructor
 */
ConvertDiffCal::ConvertDiffCal() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ConvertDiffCal::~ConvertDiffCal() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string ConvertDiffCal::name() const { return "ConvertDiffCal"; }

/// Algorithm's version for identification. @see Algorithm::version
int ConvertDiffCal::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ConvertDiffCal::category() const { return "Diffraction"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ConvertDiffCal::summary() const {
  return "Convert diffraction calibration from old to new style";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ConvertDiffCal::init() {
  declareProperty(new WorkspaceProperty<OffsetsWorkspace>("OffsetsWorkspace",
                                                          "", Direction::Input),
                  "OffsetsWorkspace containing the calibration offsets.");
  declareProperty(new WorkspaceProperty<ITableWorkspace>("OutputWorkspace", "",
                                                         Direction::Output),
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
  auto detIDs = offsetsWS->getSpectrum(index)->getDetectorIDs();
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
  instrument->getInstrumentParameters(l1, beamline, beamline_norm, samplePos);

  Geometry::IDetector_const_sptr detector = instrument->getDetector(detid);

  // the factor returned is what is needed to convert TOF->d-spacing
  // the table is supposed to be filled with DIFC which goes the other way
  const double factor = Instrument::calcConversion(l1, beamline, beamline_norm,
                                                   samplePos, detector, offset);
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
