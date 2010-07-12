#ifndef FINDCENTEROFMASSPOSITIONTEST_H_
#define FINDCENTEROFMASSPOSITIONTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/FindCenterOfMassPosition.h"
#include "MantidDataHandling/LoadSpice2D.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/UnitFactory.h"
#include "../../Algorithms/test/WorkspaceCreationHelper.hh"
#include "MantidKernel/ConfigService.h"
#include "Poco/Path.h"
#include <boost/shared_array.hpp>
#include "MantidAPI/TableRow.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid;

class FindCenterOfMassPositionTest : public CxxTest::TestSuite
{
public:

  /*
   * Generate fake data for which we know what the result should be
   */
  void setUp()
  {
    // Test peak position
    center_x = 25.5;
    center_y = 10.5;
    // Gaussian width for the peak
    double width(4.0);
    // Number of detector pixels in each dimension
    int nbins(30);
    // The test instrument has 2 monitors
    const int nMonitors(2);

    // Create a test workspace with test data with a well defined peak
    // The test instrument has two monitor channels
    ws = WorkspaceCreationHelper::Create2DWorkspace123(1,nbins*nbins+nMonitors,1);
    inputWS = "sampledata";
    Mantid::API::AnalysisDataService::Instance().addOrReplace(inputWS, ws);
    ws->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("Wavelength");
    ws->setYUnit("");

    // Generate sample data as a 2D Gaussian around the defined center
    for ( int ix=0; ix<30; ix++ )
    {
      for ( int iy=0; iy<nbins; iy++)
      {
        int i = ix*nbins+iy+nMonitors;
        MantidVec& X = ws->dataX(i);
        MantidVec& Y = ws->dataY(i);
        MantidVec& E = ws->dataE(i);
        X[0] = 1;
        X[1] = 2;
        double dx = (center_x-(double)ix);
        double dy = (center_y-(double)iy);
        Y[0] = exp(-(dx*dx+dy*dy));
        E[0] = 1;
        ws->getAxis(1)->spectraNo(i) = i;
      }
    }

    // Load instrument geometry
    runLoadInstrument("SANSTEST", ws);
    runLoadMappingTable(ws, nbins, nbins);
  }

  void testName()
  {
    setUp();
    TS_ASSERT_EQUALS( center.name(), "FindCenterOfMassPosition" )
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( center.version(), 1 )
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( center.category(), "SANS" )
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( center.initialize() )
    TS_ASSERT( center.isInitialized() )
  }

  void testExec()
  {
    if (!center.isInitialized()) center.initialize();

    const std::string outputWS("center_of_mass");
    center.setPropertyValue("InputWorkspace",inputWS);
    center.setPropertyValue("OutputWorkspace",outputWS);
    center.setPropertyValue("NPixelX","30");
    center.setPropertyValue("NPixelY","30");

    TS_ASSERT_THROWS_NOTHING( center.execute() )
    TS_ASSERT( center.isExecuted() )

    // Get the resulting table workspace
    Mantid::DataObjects::TableWorkspace_sptr table =
        boost::dynamic_pointer_cast<Mantid::DataObjects::TableWorkspace>(AnalysisDataService::Instance().retrieve(outputWS));

    TS_ASSERT_EQUALS(table->rowCount(),2);
    TS_ASSERT_EQUALS(table->columnCount(),2);

    TableRow row = table->getFirstRow();
    TS_ASSERT_EQUALS(row.String(0),"X (m)");
    TS_ASSERT_DELTA(row.Double(1),25.5,0.0001);

    row = table->getRow(1);
    TS_ASSERT_EQUALS(row.String(0),"Y (m)");
    TS_ASSERT_DELTA(row.Double(1),10.5,0.0001);

    Mantid::API::AnalysisDataService::Instance().remove(inputWS);
  }

  void testExecScatteredData()
  {
    if (!center.isInitialized()) center.initialize();

    const std::string outputWS("center_of_mass");
    center.setPropertyValue("InputWorkspace",inputWS);
    center.setPropertyValue("OutputWorkspace",outputWS);
    center.setPropertyValue("NPixelX","30");
    center.setPropertyValue("NPixelY","30");
    center.setPropertyValue("DirectBeam","0");
    center.setPropertyValue("BeamRadius", "1.5");

    TS_ASSERT_THROWS_NOTHING( center.execute() )
    TS_ASSERT( center.isExecuted() )

    // Get the resulting table workspace
    Mantid::DataObjects::TableWorkspace_sptr table =
        boost::dynamic_pointer_cast<Mantid::DataObjects::TableWorkspace>(AnalysisDataService::Instance().retrieve(outputWS));

    TS_ASSERT_EQUALS(table->rowCount(),2);
    TS_ASSERT_EQUALS(table->columnCount(),2);

    TableRow row = table->getFirstRow();
    TS_ASSERT_EQUALS(row.String(0),"X (m)");
    TS_ASSERT_DELTA(row.Double(1),25.5,0.0001);

    row = table->getRow(1);
    TS_ASSERT_EQUALS(row.String(0),"Y (m)");
    TS_ASSERT_DELTA(row.Double(1),10.5,0.0001);

    Mantid::API::AnalysisDataService::Instance().remove(inputWS);
  }

  /*
   * Test that will load an actual data file and perform the center of mass
   * calculation. This test takes a longer time to execute so we won't include
   * it in the set of unit tests.
   */
  void emptyCell()
  {
    Mantid::DataHandling::LoadSpice2D loader;
    loader.initialize();
    loader.setPropertyValue("Filename","../../../../Test/Data/SANS2D/BioSANS_exp61_scan0002_0001_emptycell.xml");
    const std::string inputWS("wav");
    loader.setPropertyValue("OutputWorkspace",inputWS);
    loader.execute();

    if (!center.isInitialized()) center.initialize();

    TS_ASSERT_THROWS_NOTHING( center.setPropertyValue("InputWorkspace",inputWS) )
    const std::string outputWS("result");
    TS_ASSERT_THROWS_NOTHING( center.setPropertyValue("OutputWorkspace",outputWS) )
    center.setPropertyValue("NPixelX","192");
    center.setPropertyValue("NPixelY","192");

    TS_ASSERT_THROWS_NOTHING( center.execute() )
    TS_ASSERT( center.isExecuted() )
    
    // Get the resulting table workspace
    Mantid::DataObjects::TableWorkspace_sptr table =
    boost::dynamic_pointer_cast<Mantid::DataObjects::TableWorkspace>(AnalysisDataService::Instance().retrieve(outputWS));

    TS_ASSERT_EQUALS(table->rowCount(),2);
    TS_ASSERT_EQUALS(table->columnCount(),2);

    // Check that the position is the same as obtained with the HFIR code
    // to within 0.3 pixel
    TableRow row = table->getFirstRow();
    TS_ASSERT_EQUALS(row.String(0),"X (m)");
    TS_ASSERT_DELTA(row.Double(1),16.6038,0.3);

    row = table->getRow(1);
    TS_ASSERT_EQUALS(row.String(0),"Y (m)");
    TS_ASSERT_DELTA(row.Double(1),96.771,0.3);
    
    Mantid::API::AnalysisDataService::Instance().remove(inputWS);
  }

private:
  Mantid::Algorithms::FindCenterOfMassPosition center;
  std::string inputWS;
  double center_x;
  double center_y;
  DataObjects::Workspace2D_sptr ws;

  /** Run the sub-algorithm LoadInstrument (as for LoadRaw)
   * @param inst_name The name written in the Nexus file
   * @param localWorkspace The workspace to insert the instrument into
   */
  void runLoadInstrument(const std::string & inst_name,
      DataObjects::Workspace2D_sptr localWorkspace)
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
    loadInst.setProperty<API::MatrixWorkspace_sptr> ("Workspace", localWorkspace);
    loadInst.execute();

  }

  /**
   * Populate spectra mapping to detector IDs
   *
   * @param localWorkspace: Workspace2D object
   * @param nxbins: number of bins in X
   * @param nybins: number of bins in Y
   */
  void runLoadMappingTable(DataObjects::Workspace2D_sptr localWorkspace, int nxbins, int nybins)
  {
    // Get the number of monitor channels
    int nMonitors = 0;
    boost::shared_ptr<API::Instrument> instrument = localWorkspace->getBaseInstrument();
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
    localWorkspace->mutableSpectraMap().populate(spec.get(), udet.get(), ndet);
  }


};

#endif /*FINDCENTEROFMASSPOSITIONTEST_H_*/
