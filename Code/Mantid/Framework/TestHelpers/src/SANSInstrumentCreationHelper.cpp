/*********************************************************************************
 *  PLEASE READ THIS!!!!!!!
 *
 *  This collection of functions MAY NOT be used in any test from a package
 *below
 *  the level of DataHandling (e.g. Kernel, Geometry, API, DataObjects).
 *  I.e. It can only be used by plugin/algorithm-level packages (e.g.
 *DataHandling)
 *********************************************************************************/
#include "MantidTestHelpers/SANSInstrumentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidKernel/UnitFactory.h"

/*****************************************************
 * SANS instrument helper class
 *****************************************************/

using Mantid::detid_t;
using Mantid::specid_t;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using Mantid::DataObjects::Workspace2D_sptr;

// Number of detector pixels in each dimension
const int SANSInstrumentCreationHelper::nBins = 30;
// The test instrument has 2 monitors
const int SANSInstrumentCreationHelper::nMonitors = 2;

/*
 * Generate a SANS test workspace, with instrument geometry.
 * The geometry is the SANSTEST geometry, with a 30x30 pixel 2D detector.
 *
 * @param workspace: name of the workspace to be created.
 */
Workspace2D_sptr SANSInstrumentCreationHelper::createSANSInstrumentWorkspace(
    std::string workspace) {
  // Create a test workspace with test data with a well defined peak
  // The test instrument has two monitor channels
  Workspace2D_sptr ws = WorkspaceCreationHelper::Create2DWorkspace123(
      nBins * nBins + nMonitors, 1, 1);
  AnalysisDataService::Instance().addOrReplace(workspace, ws);
  ws->getAxis(0)->unit() =
      Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
  ws->setYUnit("");

  // Load instrument geometry
  runLoadInstrument("SANSTEST", ws);
  runLoadMappingTable(ws, nBins, nBins);

  return ws;
}

/** Run the Child Algorithm LoadInstrument (as for LoadRaw)
 * @param inst_name :: The name written in the Nexus file
 * @param workspace :: The workspace to insert the instrument into
 */
void SANSInstrumentCreationHelper::runLoadInstrument(
    const std::string &inst_name,
    Mantid::DataObjects::Workspace2D_sptr workspace) {
  // Determine the search directory for XML instrument definition files (IDFs)
  // std::string directoryName =
  // Mantid::Kernel::ConfigService::Instance().getInstrumentDirectory();

  // For Nexus Mantid processed, Instrument XML file name is read from nexus
  std::string instrumentID = inst_name;
  // force ID to upper case
  std::transform(instrumentID.begin(), instrumentID.end(), instrumentID.begin(),
                 toupper);

  Mantid::DataHandling::LoadInstrument loadInst;
  loadInst.initialize();
  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  loadInst.setPropertyValue("Filename", "IDFs_for_UNIT_TESTING/" +
                                            instrumentID + "_Definition.xml");
  loadInst.setProperty<MatrixWorkspace_sptr>("Workspace", workspace);
  loadInst.execute();
}

/**
 * Populate spectra mapping to detector IDs
 *
 * @param workspace: Workspace2D object
 * @param nxbins: number of bins in X
 * @param nybins: number of bins in Y
 */
void SANSInstrumentCreationHelper::runLoadMappingTable(
    Mantid::DataObjects::Workspace2D_sptr workspace, int nxbins, int nybins) {
  // Get the number of monitor channels
  size_t nMonitors(0);
  size_t nXbins, nYbins;
  boost::shared_ptr<const Instrument> instrument = workspace->getInstrument();
  std::vector<detid_t> monitors = instrument->getMonitors();
  nMonitors = monitors.size();

  // Number of monitors should be consistent with data file format
  if (nMonitors != 2) {
    std::stringstream error;
    error << "Geometry error for " << instrument->getName()
          << ": Spice data format defines 2 monitors, " << nMonitors
          << " were/was found";
    throw std::runtime_error(error.str());
  }
  if (nxbins >= 0) {
    nXbins = size_t(nxbins);
  } else {
    throw std::invalid_argument("number of x-bins < 0");
  }
  if (nybins >= 0) {
    nYbins = size_t(nybins);
  } else {
    throw std::invalid_argument("number of y-bins < 0");
  }

  // Generate mapping of detector/channel IDs to spectrum ID

  // Detector/channel counter
  size_t wi = 0;

  // Monitor: IDs start at 1 and increment by 1
  for (size_t i = 0; i < nMonitors; i++) {
    // std::cout << "SANS instrument monitor number " << i << std::endl;
    workspace->getSpectrum(wi)->setSpectrumNo(specid_t(wi));
    workspace->getSpectrum(wi)->setDetectorID(detid_t(wi + 1));
    wi++;
  }

  // Detector pixels
  for (size_t ix = 0; ix < nXbins; ix++) {
    for (size_t iy = 0; iy < nYbins; iy++) {
      workspace->getSpectrum(wi)->setSpectrumNo(specid_t(wi));
      workspace->getSpectrum(wi)
          ->setDetectorID(detid_t(1000000 + iy * 1000 + ix));
      wi++;
    }
  }
}
