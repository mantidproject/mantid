#ifndef SETSCALINGPSDTEST_H_
#define SETSCALINGPSDTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/SetScalingPSD.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/ConfigService.h"
#include <Poco/File.h>
#include <Poco/Path.h>
#include <fstream>
#include <cmath>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using Mantid::DataHandling::SetScalingPSD;
using Mantid::DataHandling::LoadEmptyInstrument;
using Mantid::DataObjects::Workspace2D;
using Mantid::DataObjects::Workspace2D_sptr;
using Mantid::Kernel::ConfigService;

class SetScalingPSDTest : public CxxTest::TestSuite
{
public:

    static SetScalingPSDTest *createSuite() { return new SetScalingPSDTest(); }
  static void destroySuite(SetScalingPSDTest *suite) { delete suite; }

  SetScalingPSDTest() : m_y_offset(0.0005)
  {
  }

  void test_Input_ASCII_File_Scales_Correctly()
  {
    const int ndets = 5;
    // Test workspace with 5 detectors, 3 detectors + 2 monitors at the end. 1:1 mapping of index:ID
    Workspace2D_sptr testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(ndets, 1, true);
    const std::string scalingFile = createTestScalingFile(testWS);
    // Needs to be in the ADS for this algorithm
    const std::string wsName("PSDTest");
    AnalysisDataServiceImpl & dataStore = AnalysisDataService::Instance();
    dataStore.add(wsName, testWS);

    // Store the detector positions before the shifts so that we can compare
    std::map<int, V3D> originalPositions;
    for( int i = 0; i < ndets; ++i )
    {
      IDetector_sptr det = testWS->getDetector(i);
      originalPositions.insert(std::pair<int, V3D>(i, det->getPos()));
    }

    IAlgorithm_sptr scaler = createAlgorithm();
    scaler->setPropertyValue("ScalingFilename", scalingFile);
    scaler->setPropertyValue("Workspace", wsName);
    TS_ASSERT_THROWS_NOTHING(scaler->execute());

    // Test values.
    double expectedYPos[5] = {-0.0005, 0.0990, 0.1985, -0.002042, -0.0025133 };
    double expectedYScale[3] = {0.995002, 0.995001, 0.995000 };

    const ParameterMap & pmap = testWS->instrumentParameters();
    for( int i = 0; i < ndets; ++i )
    {
      IDetector_sptr det = testWS->getDetector(i);
      V3D newPos = det->getPos();
      V3D oldPos = originalPositions[i];
      TS_ASSERT_DELTA(newPos.Y(), expectedYPos[i], 1e-6);
      TS_ASSERT_DELTA(abs(oldPos.X() - newPos.X()), 0.0, 1e-05);
      TS_ASSERT_DELTA(abs(oldPos.Z() - newPos.Z()), 0.0, 1e-05);
      
      if( det->isMonitor() )
      {
	TS_ASSERT_EQUALS(pmap.contains(det.get(), "sca"), false);
      }
      else
      {
	TS_ASSERT_EQUALS(pmap.contains(det.get(), "sca"), true);
	Parameter_sptr scaleParam = pmap.get(det.get(), "sca");
	const V3D scaleFactor = scaleParam->value<V3D>();
	TS_ASSERT_EQUALS(scaleFactor.X(), 1.0);
	TS_ASSERT_EQUALS(scaleFactor.Z(), 1.0);
	TS_ASSERT_DELTA(scaleFactor.Y(), expectedYScale[i], 1e-06);
	
      }
    }

    Poco::File(scalingFile).remove();
    dataStore.remove(wsName);
  }

  void test_Input_RAW_File_Scales_Correctly()
  {
    // No way around using a raw files here unfortunately as that's what we need to test
    Workspace2D_sptr testWS = loadEmptyMARI();
    TS_ASSERT(testWS);
    if( !testWS )
    {
      TS_FAIL("Error loading test instrument.");
    }

    IAlgorithm_sptr scaler = createAlgorithm();
    // The most commonly used MARI file in the repository
    scaler->setPropertyValue("ScalingFilename", "MAR11060.raw");
    scaler->setPropertyValue("Workspace", testWS->getName());
    scaler->execute();    

    // Test a few detectors
    int testIndices[3] = {6,7,8};
    V3D expectedValues[3] = { V3D(-0.08982175,-1.03708771,3.88495351), \
			      V3D(-0.09233499,-1.06610575,3.87703178), \
			      V3D(-0.09484302,-1.09506369,3.86889169) };
    for( int i = 0; i < 3; ++i )
    {
      IDetector_sptr det = testWS->getDetector(testIndices[i]);
      V3D pos = det->getPos();
      V3D expectedPos = expectedValues[i];
      TS_ASSERT_DELTA(pos.X(), expectedPos.X(), 1e-8);
      TS_ASSERT_DELTA(pos.Y(), expectedPos.Y(), 1e-8);
      TS_ASSERT_DELTA(pos.Z(), expectedPos.Z(), 1e-8);
    }

    AnalysisDataService::Instance().remove(testWS->getName());
  }

private:


  std::string createTestScalingFile(Workspace2D_sptr testWS)
  {
    // NOTE: The only values read by the algorithm are the number of detectors and then from each detector line
    // det no., (l2,theta,phi). All other values will be set to -1 here
    const int ndets = testWS->getNumberHistograms();    
    const std::string filename = "test-setscalingpsd.sca";
    std::ofstream writer(filename.c_str());
    writer << filename << " created by unit test\n";
    writer << ndets << "\t" << -1 << "\n";
    writer << "det no.  offset    l2     code     theta        phi"
	   << "         w_x         w_y         w_z         f_x         f_y\n";
    for( int i = 0; i < ndets; ++i )
    {
      IDetector_sptr det = testWS->getDetector(i);
      V3D oldPos(det->getPos());
      // Move in Y only
      oldPos.setY(oldPos.Y() - m_y_offset*(i+1));
      double l2, theta, phi;
      oldPos.getSpherical(l2, theta, phi);
      writer << i << "\t" << -1 << "\t" << l2 << "\t" << -1 << "\t" << theta << "\t" << phi << "\n";
    }

    writer.close();
    return filename;
  }

  Workspace2D_sptr loadEmptyMARI()
  {
    Poco::Path mariIDF(ConfigService::Instance().getInstrumentDirectory());
    mariIDF.resolve("MARI_Definition.xml");
    LoadEmptyInstrument loader;
    loader.initialize();
    loader.setPropertyValue("Filename", mariIDF.toString());
    const std::string outputName("test-emptyMARI");
    loader.setPropertyValue("OutputWorkspace", outputName);
    loader.execute();
    Workspace_sptr result = AnalysisDataService::Instance().retrieve(outputName);
    return boost::dynamic_pointer_cast<Workspace2D>(result);
  }

  IAlgorithm_sptr createAlgorithm()
  {
    IAlgorithm_sptr scaler(new SetScalingPSD);
    scaler->initialize();
    scaler->setRethrows(true);
    return scaler;
  }
  
  /// Constant shift
  const double m_y_offset;
  
};

#endif /*SETSCALINGPSDTEST_H_*/
