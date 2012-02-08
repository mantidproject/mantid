#ifndef IMD_MATRIX_WORKSPACETEST_H_
#define IMD_MATRIX_WORKSPACETEST_H_

//Tests the MatrixWorkspace as an IMDWorkspace.

#include <cxxtest/TestSuite.h>
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidAPI/MatrixWSIndexCalculator.h"
#include "MantidTestHelpers/FakeObjects.h"

using std::size_t;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

//Test the MatrixWorkspace as an IMDWorkspace.
class IMDWorkspaceTest : public CxxTest::TestSuite
{
private:

  WorkspaceTester workspace;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IMDWorkspaceTest *createSuite() { return new IMDWorkspaceTest(); }
  static void destroySuite( IMDWorkspaceTest *suite ) { delete suite; }

  IMDWorkspaceTest()
  {
    workspace.setTitle("workspace");
    workspace.initialize(2,4,3);
    workspace.getSpectrum(0)->setSpectrumNo(1);
    workspace.getSpectrum(1)->setSpectrumNo(2);
    for (int i = 0; i < 4; ++i)
    {
      workspace.dataX(0)[i] = i;
      workspace.dataX(1)[i] = i+4;
    }

    for (int i = 0; i < 3; ++i)
    {
      workspace.dataY(0)[i] = i*10;
      workspace.dataE(0)[i] = sqrt(workspace.dataY(0)[i]);
      workspace.dataY(1)[i] = i*100;
      workspace.dataE(1)[i] = sqrt(workspace.dataY(1)[i]);
    }
  }

  void testGetXDimension() 
  { 
    WorkspaceTester matrixWS;
    matrixWS.init(1, 1, 1);
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> dimension = matrixWS.getXDimension();
    std::string id = dimension->getDimensionId();
    TSM_ASSERT_EQUALS("Dimension-X does not have the expected dimension id.", "xDimension", id);
  }


  void testGetYDimension()
  {
    WorkspaceTester matrixWS;
    matrixWS.init(1, 1, 1);
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> dimension = matrixWS.getYDimension();
    std::string id = dimension->getDimensionId();
    TSM_ASSERT_EQUALS("Dimension-Y does not have the expected dimension id.", "yDimension", id);
  }

  void testGetZDimension()
  {
    WorkspaceTester matrixWS;
    TSM_ASSERT_THROWS_ANYTHING("Current implementation should throw runtime error.", matrixWS.getZDimension());
  }

  void testGettDimension()
  {
    WorkspaceTester matrixWS;
    TSM_ASSERT_THROWS_ANYTHING("Current implementation should throw runtime error.", matrixWS.getTDimension());
  }

  void testGetDimensionThrows()
  {
    WorkspaceTester matrixWS;
    matrixWS.init(1,1,1);
    TSM_ASSERT_THROWS("Id doesn't exist. Should throw during find routine.", matrixWS.getDimensionWithId("3"), std::overflow_error);
  }

  void testGetDimension()
  {
    WorkspaceTester matrixWS;
    matrixWS.init(1,1,1);
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> dim = matrixWS.getDimensionWithId("yDimension");
    TSM_ASSERT_EQUALS("The dimension id found is not the same as that searched for.", "yDimension", dim->getDimensionId());
  }

  void testGetDimensionOverflow()
  {
    WorkspaceTester matrixWS;
    matrixWS.init(1,1,1);
    TSM_ASSERT_THROWS("The dimension does not exist. Attempting to get it should throw", matrixWS.getDimensionWithId("1"), std::overflow_error);
  }

  void testGetNPoints()
  {
    WorkspaceTester matrixWS;
    matrixWS.init(5, 5, 5);
    TSM_ASSERT_EQUALS("The expected number of points have not been returned.", 25, matrixWS.getNPoints());
  }

  void testGetHistogramIndex()
  {
    MatrixWSIndexCalculator indexCalculator(5);
    HistogramIndex histogramIndexA = indexCalculator.getHistogramIndex(4);
    HistogramIndex histogramIndexB = indexCalculator.getHistogramIndex(5);
    HistogramIndex histogramIndexC = indexCalculator.getHistogramIndex(10);
    TSM_ASSERT_EQUALS("histogram index has not been calculated correctly.", 0, histogramIndexA);
    TSM_ASSERT_EQUALS("histogram index has not been calculated correctly.", 1, histogramIndexB);
    TSM_ASSERT_EQUALS("histogram index has not been calculated correctly.", 2, histogramIndexC);
  }

  void testGetBinIndex()
  {
    MatrixWSIndexCalculator indexCalculator(5);
    BinIndex binIndexA = indexCalculator.getBinIndex(4, 0);
    BinIndex binIndexB = indexCalculator.getBinIndex(12, 2);
    TSM_ASSERT_EQUALS("bin index has not been calculated correctly.", 4, binIndexA);
    TSM_ASSERT_EQUALS("bin index has not been calculated correctly.", 2, binIndexB);
  }


};

#endif /*IMD_MATRIX_WORKSPACETEST_H_*/
