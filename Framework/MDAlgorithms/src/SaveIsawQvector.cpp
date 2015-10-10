#include "MantidMDAlgorithms/SaveIsawQvector.h"

#include <fstream>

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidMDAlgorithms/MDTransfFactory.h"
#include "MantidMDAlgorithms/UnitsConversionHelper.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveIsawQvector)

/// This only works for diffraction.
const std::string ELASTIC("Elastic");
/// Only convert to Q-vector.
const std::string Q3D("Q3D");
/// Q-vector is always three dimensional.
const std::size_t DIMS(3);
/// Constant for the size of the buffer to write to disk.
const std::size_t BUFF_SIZE(DIMS * sizeof(float));

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SaveIsawQvector::SaveIsawQvector() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
SaveIsawQvector::~SaveIsawQvector() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string SaveIsawQvector::name() const { return "SaveIsawQvector"; }

/// Algorithm's version for identification. @see Algorithm::version
int SaveIsawQvector::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SaveIsawQvector::category() const {
  return "DataHandling\\Isaw";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SaveIsawQvector::init() {
  auto ws_valid = boost::make_shared<CompositeValidator>();
  //
  ws_valid->add<InstrumentValidator>();
  // the validator which checks if the workspace has axis and any units
  ws_valid->add<WorkspaceUnitValidator>("TOF");

  declareProperty(new WorkspaceProperty<EventWorkspace>(
                      "InputWorkspace", "", Direction::Input, ws_valid),
                  "An input EventWorkspace with units along X-axis and defined "
                  "instrument with defined sample");

  std::vector<std::string> exts;
  exts.push_back(".bin");

  declareProperty(
      new FileProperty("Filename", "", FileProperty::OptionalSave, exts),
      "Optional path to an hkl file to save.  Vectors returned if no file "
      "requested.");
  declareProperty("RightHanded", true, "Save the Q-vector as k_f - k_i");
  declareProperty(
      "ISAWcoords", true,
      "Save the Q-vector with y gravitationally up and x pointing downstream");
  std::vector<double> Qx_save, Qy_save, Qz_save;
  declareProperty("Qx_vector", Qx_save,
                  "The name of the vector in which to store the list of Qx",
                  Direction::Output);
  declareProperty("Qy_vector", Qy_save,
                  "The name of the vector in which to store the list of Qy",
                  Direction::Output);
  declareProperty("Qz_vector", Qz_save,
                  "The name of the vector in which to store the list of Qz",
                  Direction::Output);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SaveIsawQvector::exec() {
  // get the input workspace
  EventWorkspace_sptr wksp = getProperty("InputWorkspace");

  // this only works for unweighted events
  if (wksp->getEventType() != API::TOF) {
    throw std::runtime_error("SaveIsawQvector only works for raw events");
  }
  // error out if there are not events
  if (wksp->getNumberEvents() <= 0) {
    throw std::runtime_error(
        "SaveIsawQvector does not work for empty event lists");
  }

  // open the output file
  std::string filename = getPropertyValue("Filename");
  std::ofstream handle;
  if (!filename.empty()) {
    handle.open(filename.c_str(), std::ios::out | std::ios::binary);
    if (!handle.is_open())
      throw std::runtime_error("Failed to open file for writing");
  }
  // set up a descripter of where we are going
  this->initTargetWSDescr(wksp);

  size_t coord_map[DIMS] = {0, 1, 2};      // x->x, y->y, z->z
  double coord_signs[DIMS] = {1., 1., 1.}; // signs are unchanged
  if (this->getProperty("ISAWcoords")) {
    // x -> z
    coord_map[0] = 2;

    // y -> x
    coord_map[1] = 0;

    // z -> y
    coord_map[2] = 1;
  }
  if (this->getProperty("RightHanded")) {
    for (size_t dim = 0; dim < DIMS; ++dim)
      coord_signs[dim] *= -1.; // everything changes sign
  }

  // units conersion helper
  UnitsConversionHelper unitConv;
  unitConv.initialize(m_targWSDescr, "Momentum");

  // initialize the MD coordinates conversion class
  MDTransf_sptr q_converter =
      MDTransfFactory::Instance().create(m_targWSDescr.AlgID);
  q_converter->initialize(m_targWSDescr);

  // set up the progress bar
  const size_t numSpectra = wksp->getNumberHistograms();
  Progress prog(this, 0.5, 1.0, numSpectra);

  // loop through the eventlists
  float buffer[DIMS];
  std::vector<double> Qx_save, Qy_save, Qz_save;
  for (std::size_t i = 0; i < numSpectra; ++i) {
    // get a reference to the event list
    const EventList &events = wksp->getEventList(i);

    // check to see if the event list is empty
    if (events.empty()) {
      prog.report();
      continue; // nothing to do
    }

    // update which pixel is being converted
    std::vector<coord_t> locCoord(DIMS, 0.);
    unitConv.updateConversion(i);
    q_converter->calcYDepCoordinates(locCoord, i);

    // loop over the events
    double signal(1.);  // ignorable garbage
    double errorSq(1.); // ignorable garbage
    const std::vector<TofEvent> &raw_events = events.getEvents();
    for (auto event = raw_events.begin(); event != raw_events.end(); ++event) {
      double val = unitConv.convertUnits(event->tof());
      q_converter->calcMatrixCoord(val, locCoord, signal, errorSq);
      for (size_t dim = 0; dim < DIMS; ++dim) {
        buffer[dim] =
            static_cast<float>(coord_signs[dim] * locCoord[coord_map[dim]]);
      }
      if (filename.empty()) {
        Qx_save.push_back(static_cast<double>(buffer[0]));
        Qy_save.push_back(static_cast<double>(buffer[1]));
        Qz_save.push_back(static_cast<double>(buffer[2]));
      } else
        handle.write(reinterpret_cast<char *>(buffer), BUFF_SIZE);
    } // end of loop over events in list

    prog.report();
  } // end of loop over spectra
  if (filename.empty()) {
    setProperty("Qx_vector", Qx_save);
    setProperty("Qy_vector", Qy_save);
    setProperty("Qz_vector", Qz_save);
  } else
    handle.close(); // cleanup
}

/**
 * @brief SaveIsawQvector::initTargetWSDescr Initialize the output information
 * for the MD conversion framework.
 *
 * @param wksp The workspace to get information from.
 */
void SaveIsawQvector::initTargetWSDescr(EventWorkspace_sptr wksp) {
  m_targWSDescr.setMinMax(std::vector<double>(3, -2000.),
                          std::vector<double>(3, 2000.));
  m_targWSDescr.buildFromMatrixWS(wksp, Q3D, ELASTIC);
  m_targWSDescr.setLorentsCorr(false);

  // generate the detectors table
  Mantid::API::Algorithm_sptr childAlg =
      createChildAlgorithm("PreprocessDetectorsToMD", 0., .5);
  childAlg->setProperty("InputWorkspace", wksp);
  childAlg->executeAsChildAlg();

  DataObjects::TableWorkspace_sptr table =
      childAlg->getProperty("OutputWorkspace");
  if (!table)
    throw(std::runtime_error(
        "Can not retrieve results of \"PreprocessDetectorsToMD\""));
  else
    m_targWSDescr.m_PreprDetTable = table;
}

} // namespace MDAlgorithms
} // namespace Mantid
