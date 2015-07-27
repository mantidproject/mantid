#ifndef LOADINSTRUMENTTESTFROMRAW_H_
#define LOADINSTRUMENTTESTFROMRAW_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadIDFFromNexus.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidTestHelpers/ScopedFileHelper.h"
#include <vector>
#include <Poco/Path.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using ScopedFileHelper::ScopedFile;

class LoadIDFFromNexusTest : public CxxTest::TestSuite
{
public:

  static LoadIDFFromNexusTest *createSuite() { return new LoadIDFFromNexusTest(); }
  static void destroySuite(LoadIDFFromNexusTest *suite) { delete suite; }

  LoadIDFFromNexusTest()
  {
  }

  void testInit()
  {
    TS_ASSERT( !loader.isInitialized() );
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT( loader.isInitialized() );
  }

  void testExec()
  {
    // We load a processed Nexus file with embedded parameters

    if ( !loader.isInitialized() ) loader.initialize();

    //Create a workspace with some sample data
    wsName = "LoadIDFFromNexusTest";
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);

    //Put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    // Set properties
    loader.setPropertyValue("Workspace", wsName);
    loader.setPropertyValue("Filename", "LOQ48127.nxs"); 
    loader.setPropertyValue("InstrumentParentPath","mantid_workspace_1"); 
    inputFile = loader.getPropertyValue("Filename"); // get full pathname
    
    // Check properties
    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loader.getPropertyValue("Filename") )
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loader.getPropertyValue("Workspace") )
    TS_ASSERT( ! result.compare(wsName));

    TS_ASSERT_THROWS_NOTHING( result = loader.getPropertyValue("InstrumentParentPath") )
    TS_ASSERT( ! result.compare("mantid_workspace_1"));

    // Execute
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT( loader.isExecuted() );

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName));

    // Test instrument name, source and sample
    boost::shared_ptr<const Instrument> i = output->getInstrument();
    TS_ASSERT_EQUALS( i->getName(), "LOQ");

    boost::shared_ptr<const IComponent> source = i->getSource();
    TS_ASSERT_EQUALS( source->getName(), "source");
    TS_ASSERT_DELTA( source->getPos().Z(), 0.0,0.01);

    boost::shared_ptr<const IComponent> samplepos = i->getSample();
    TS_ASSERT_EQUALS( samplepos->getName(),"some-sample-holder");
    TS_ASSERT_DELTA( samplepos->getPos().Z(), 11.0,0.01);

    // Test third pixel in main detector bank, which has indices (2,0)
    boost::shared_ptr<const Detector> ptrDetMain = boost::dynamic_pointer_cast<const Detector>(i->getDetector(5));
    TS_ASSERT_EQUALS( ptrDetMain->getID(), 5);
    TS_ASSERT_EQUALS( ptrDetMain->getName(), "main-detector-bank(2,0)");
    TS_ASSERT_DELTA( ptrDetMain->getPos().X(), -0.3035,0.0001); 
    TS_ASSERT_DELTA( ptrDetMain->getPos().Y(), -0.3124,0.0001);
    double d = ptrDetMain->getPos().distance(samplepos->getPos());
    TS_ASSERT_DELTA(d, 4.1727, 0.0001);
    double cmpDistance = ptrDetMain->getDistance(*samplepos);
    TS_ASSERT_DELTA(cmpDistance, 4.1727, 0.0001);

    TS_ASSERT_EQUALS( ptrDetMain->type(), "RectangularDetectorPixel");

    // Test a HAB pixel detector
    boost::shared_ptr<const Detector> ptrDetHab = boost::dynamic_pointer_cast<const Detector>(i->getDetector(16734));
    TS_ASSERT_EQUALS( ptrDetHab->getID(), 16734);
    TS_ASSERT_EQUALS( ptrDetHab->getName(), "HAB-pixel");
    // Test a non-existant detector
    TS_ASSERT_THROWS(i->getDetector(16735), Exception::NotFoundError);

    // Check the monitors are correctly marked
    TS_ASSERT( i->getDetector(1)->isMonitor() )
    TS_ASSERT( i->getDetector(2)->isMonitor() )
    // ...and that a normal detector isn't
    TS_ASSERT( ! i->getDetector(3)->isMonitor() )
    TS_ASSERT( ! i->getDetector(300)->isMonitor() )
    TS_ASSERT( ! i->getDetector(16500)->isMonitor() )

	  AnalysisDataService::Instance().remove(wsName);
  }

  void test_parameter_source() {

    // We load a processed Nexus file with embedded parameters, one of which has been made different 
    // from the same parameter in the file on disk (LOQ_Parameters.xml)

    if ( !loader.isInitialized() ) loader.initialize();

    //Create a workspace with some sample data
    wsName = "LoadIDFFromNexusTest2";
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);

    //Put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    // Set properties 
    loader.setPropertyValue("Workspace", wsName);
    loader.setPropertyValue("Filename", "LOQ48127p.nxs"); 
    loader.setPropertyValue("InstrumentParentPath","mantid_workspace_1"); 
    inputFile = loader.getPropertyValue("Filename"); // get full pathname

    // Execute
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT( loader.isExecuted() );

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName));

    // We now check the parameter that is different in the embedded parameters
    const ParameterMap& paramMap = output->instrumentParameters();
    boost::shared_ptr<const Instrument> i = output->getInstrument();
    TS_ASSERT_EQUALS(paramMap.getString(i.get(), "low-angle-detector-name"), "LAB");
    // If this gives "main-detector-bank" instead of "LAB", 
    // then the embedded parameters have not, been read and the parameter file has been used instead.

  }

   void test_parameter_file() {

    // We load a processed Nexus file without embedded parameters and 
    // check that parameters has been loaded (from file) despite that

    if ( !loader.isInitialized() ) loader.initialize();

    //Create a workspace with some sample data
    wsName = "LoadIDFFromNexusTest3";
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);

    //Put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    // Set properties 
    loader.setPropertyValue("Workspace", wsName);
    loader.setPropertyValue("Filename", "LOQ48127np.nxs"); 
    loader.setPropertyValue("InstrumentParentPath","mantid_workspace_1"); 
    inputFile = loader.getPropertyValue("Filename"); // get full pathname

    // Execute
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT( loader.isExecuted() );

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName));

    // We now check a parameter
    const ParameterMap& paramMap = output->instrumentParameters();
    boost::shared_ptr<const Instrument> i = output->getInstrument();
    TS_ASSERT_EQUALS(paramMap.getString(i.get(), "low-angle-detector-name"), "main-detector-bank");

  }

  void test_get_parameter_correction_file() {

    // We test the function the looks for a parameter correction file 
    // for a given instrument.

    // TEST1 file exists
    std::string testpath1 = loader.getParameterCorrectionFile("TEST1");
    Poco::Path iPath( true );  // Absolute path
    TS_ASSERT(iPath.tryParse(testpath1)); // Result has correct syntax
    TS_ASSERT(iPath.isFile()); // Result is a file
    TS_ASSERT(iPath.getFileName()=="TEST1_Parameter_Corrections.xml"); // Correct filename
    TS_ASSERT(iPath.directory(iPath.depth()-1)=="embedded_instrument_corrections"); // Correct folder

    // TEST0 file does not exist
    std::string testpath0 = loader.getParameterCorrectionFile("TEST0"); 
    TS_ASSERT(testpath0 == ""); // Nothing should be found
  }

  void test_read_parameter_correction_file() {
    std::string contents = 
      "<EmbeddedParameterCorrections name='XXX'>"
      "   <correction  valid-from='2015-06-26 00:00:00'  valid-to='2015-07-21 23:59:59' file='test1.xml' append='false'/>"
      "   <correction  valid-from='2015-07-22 00:00:00'  valid-to='2015-07-31 11:59:59' file='test2.xml' append='true'/>"
      "</EmbeddedParameterCorrections>";
    std::string correctionFilename = "parameter_correction_test.xml";
    ScopedFile file( contents, correctionFilename, "." );
    std::string parameterFile;
    bool append;

    // Date too early for correction
    TS_ASSERT_THROWS_NOTHING(loader.readParameterCorrectionFile(correctionFilename, "2015-06-25 23:00:00", parameterFile, append ));
    TS_ASSERT(parameterFile == ""); 

    // Date for first correction
    TS_ASSERT_THROWS_NOTHING(loader.readParameterCorrectionFile(correctionFilename, "2015-06-30 13:00:00", parameterFile, append ));
    TS_ASSERT(parameterFile == "test1.xml");
    TS_ASSERT(append == false);

    // Date for second correction
    TS_ASSERT_THROWS_NOTHING(loader.readParameterCorrectionFile(correctionFilename, "2015-07-30 13:00:00", parameterFile, append ));
    TS_ASSERT(parameterFile == "test2.xml");
    TS_ASSERT(append == true);

    // Date too late for correction
    TS_ASSERT_THROWS_NOTHING(loader.readParameterCorrectionFile(correctionFilename, "2015-07-31 12:00:00", parameterFile, append ));
    TS_ASSERT(parameterFile == ""); 
    
  }



private:
  LoadIDFFromNexus loader;
  std::string inputFile;
  std::string wsName;

};

#endif /*LOADINSTRUMENTTESTFROMRAW_H_*/
