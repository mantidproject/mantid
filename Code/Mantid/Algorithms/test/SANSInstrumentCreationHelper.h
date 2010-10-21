#ifndef SANSINSTRUMENTCREATIONHELPER_H_
#define SANSINSTRUMENTCREATIONHELPER_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "../../Algorithms/test/WorkspaceCreationHelper.hh"
#include "MantidKernel/ConfigService.h"
#include "Poco/Path.h"
#include <boost/shared_array.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid;

class SANSInstrumentCreationHelper
{
public:

  // Number of detector pixels in each dimension
  static const int nBins = 30;
  // The test instrument has 2 monitors
  static const int nMonitors = 2;

  /*
   * Generate a SANS test workspace, with instrument geometry.
   * The geometry is the SANSTEST geometry, with a 30x30 pixel 2D detector.
   *
   * @param workspace: name of the workspace to be created.
   */
  static DataObjects::Workspace2D_sptr createSANSInstrumentWorkspace(std::string workspace)
  {
    // Create a test workspace with test data with a well defined peak
    // The test instrument has two monitor channels
    DataObjects::Workspace2D_sptr ws = WorkspaceCreationHelper::Create2DWorkspace123(1,nBins*nBins+nMonitors,1);
    Mantid::API::AnalysisDataService::Instance().addOrReplace(workspace, ws);
    ws->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("Wavelength");
    ws->setYUnit("");
    for (int i = 0; i < ws->getNumberHistograms(); ++i)
    {
      ws->getAxis(1)->spectraNo(i) = i;
    }

    // Load instrument geometry
    runLoadInstrument("SANSTEST", ws);
    runLoadMappingTable(ws, nBins, nBins);

    return ws;
  }

  /** Run the sub-algorithm LoadInstrument (as for LoadRaw)
   * @param inst_name The name written in the Nexus file
   * @param workspace The workspace to insert the instrument into
   */
  static void runLoadInstrument(const std::string & inst_name,
      DataObjects::Workspace2D_sptr workspace)
  {
    // Determine the search directory for XML instrument definition files (IDFs)
    std::string directoryName = Kernel::ConfigService::Instance().getString(
        "instrumentDefinition.directory");
    if (directoryName.empty())
    {
      // This is the assumed deployment directory for IDFs, where we need to be relative to the
      // directory of the executable, not the current working directory.
      directoryName = Poco::Path(Mantid::Kernel::ConfigService::Instance().getBaseDir()).resolve(
          "../Instrument").toString();
    }

    // For Nexus Mantid processed, Instrument XML file name is read from nexus
    std::string instrumentID = inst_name;
    // force ID to upper case
    std::transform(instrumentID.begin(), instrumentID.end(), instrumentID.begin(), toupper);
    std::string fullPathIDF = directoryName + "/" + instrumentID + "_Definition.xml";

    Mantid::DataHandling::LoadInstrument loadInst;
    loadInst.initialize();
    // Now execute the sub-algorithm. Catch and log any error, but don't stop.
    loadInst.setPropertyValue("Filename", fullPathIDF);
    loadInst.setProperty<API::MatrixWorkspace_sptr> ("Workspace", workspace);
    loadInst.execute();

  }

  /**
   * Populate spectra mapping to detector IDs
   *
   * @param workspace: Workspace2D object
   * @param nxbins: number of bins in X
   * @param nybins: number of bins in Y
   */
  static void runLoadMappingTable(DataObjects::Workspace2D_sptr workspace, int nxbins, int nybins)
  {
    // Get the number of monitor channels
    int nMonitors = 0;
    boost::shared_ptr<Geometry::Instrument> instrument = workspace->getBaseInstrument();
    std::vector<int> monitors = instrument->getMonitors();
    nMonitors = monitors.size();

    // Number of monitors should be consistent with data file format
    if( nMonitors != 2 ) {
      std::stringstream error;
      error << "Geometry error for " << instrument->getName() <<
          ": Spice data format defines 2 monitors, " << nMonitors << " were/was found";
      throw std::runtime_error(error.str());
    }

    int ndet = nxbins*nybins + nMonitors;
    boost::shared_array<int> udet(new int[ndet]);
    boost::shared_array<int> spec(new int[ndet]);

    // Generate mapping of detector/channel IDs to spectrum ID

    // Detector/channel counter
    int icount = 0;

    // Monitor: IDs start at 1 and increment by 1
    for(int i=0; i<nMonitors; i++)
    {
      spec[icount] = icount;
      udet[icount] = icount+1;
      icount++;
    }

    // Detector pixels
    for(int ix=0; ix<nxbins; ix++)
    {
      for(int iy=0; iy<nybins; iy++)
      {
        spec[icount] = icount;
        udet[icount] = 1000000 + iy*1000 + ix;
        icount++;
      }
    }

    // Populate the Spectra Map with parameters
    workspace->mutableSpectraMap().populate(spec.get(), udet.get(), ndet);
  }


};

#endif /*SANSINSTRUMENTCREATIONHELPER_H_*/
