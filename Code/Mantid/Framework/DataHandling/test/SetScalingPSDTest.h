#ifndef SETSCALINGPSDTEST_H_
#define SETSCALINGPSDTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/SetScalingPSD.h"
#include "../../Algorithms/test/WorkspaceCreationHelper.hh"
#include <Poco/File.h>
#include <fstream>
#include <cmath>

using Mantid::Algorithms::SetScalingPSD;

class SetScalingPSDTest : public CxxTest::TestSuite
{
public:

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
    std::map<int, Geometry::V3D> originalPositions;
    for( int i = 0; i < ndets; ++i )
    {
      IDetector_sptr det = testWS->getDetector(i);
      originalPositions.insert(std::pair<int, Geometry::V3D>(i, det->getPos()));
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
