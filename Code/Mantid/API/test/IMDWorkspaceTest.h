#ifndef IMD_MATRIX_WORKSPACETEST_H_
#define IMD_MATRIX_WORKSPACETEST_H_

//Tests the MatrixWorkspace as an IMDWorkspace.

#include <cxxtest/TestSuite.h>
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidGeometry/IInstrument.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace Mantid { namespace DataObjects {
  class MatrixWorkspaceTester : public MatrixWorkspace
  {
  public:
    MatrixWorkspaceTester() : MatrixWorkspace() {}
    virtual ~MatrixWorkspaceTester() {}

    // Empty overrides of virtual methods
    virtual int getNumberHistograms() const { return 1;}
    const std::string id() const {return "MatrixWorkspaceTester";}
    void init(const int& i, const int& j, const int& k)
    {
      vec.resize(j,1.0);
      // Put an 'empty' axis in to test the getAxis method
      m_axes.resize(1);
      m_axes[0] = new NumericAxis(1);
    }
    int size() const {return vec.size();}
    int blocksize() const {return vec.size();}
    MantidVec& dataX(int const ) {return vec;}
    MantidVec& dataY(int const ) {return vec;}
    MantidVec& dataE(int const ) {return vec;}
    const MantidVec& dataX(int const) const {return vec;}
    const MantidVec& dataY(int const) const {return vec;}
    const MantidVec& dataE(int const) const {return vec;}
    Kernel::cow_ptr<MantidVec> refX(const int) const {return Kernel::cow_ptr<MantidVec>();}
    void setX(const int, const Kernel::cow_ptr<MantidVec>&) {}

  private:
    MantidVec vec;
    int spec;
  };
}}// namespace

//Test the MatrixWorkspace as an IMDWorkspace.
class IMDWorkspaceTest : public CxxTest::TestSuite
{

public:

  //--- Owen Arnold 15/12/2010 Characterisation tests. Baseline current implementation ---

  //Characterisation test. 
  void testGetXDimension() 
  {
    using namespace Mantid::DataObjects;
    MatrixWorkspaceTester matrixWS;
    TSM_ASSERT_THROWS("Current implementation should throw runtime error.", matrixWS.getXDimension(), std::runtime_error);
  }

  //Characterisation test. 
  void testGetYDimension()
  {
    using namespace Mantid::DataObjects;
    MatrixWorkspaceTester matrixWS;
    TSM_ASSERT_THROWS("Current implementation should throw runtime error.", matrixWS.getXDimension(), std::runtime_error);
  }

  //Characterisation test. 
  void testGetZDimension()
  {
    using namespace Mantid::DataObjects;
    MatrixWorkspaceTester matrixWS;
    TSM_ASSERT_THROWS("Current implementation should throw runtime error.", matrixWS.getXDimension(), std::runtime_error);
  }

  //Characterisation test. 
  void testGettDimension()
  {
    using namespace Mantid::DataObjects;
    MatrixWorkspaceTester matrixWS;
    TSM_ASSERT_THROWS("Current implementation should throw runtime error.", matrixWS.getXDimension(), std::runtime_error);
  }

  //Characterisation test. 
  void testGetPoint()
  {
    using namespace Mantid::DataObjects;
    MatrixWorkspaceTester matrixWS;
    TSM_ASSERT_THROWS("Current implementation should throw runtime error.", matrixWS.getPoint(1), std::runtime_error);
  }

  //Characterisation test. 
  void testGetDimension()
  {
    using namespace Mantid::DataObjects;
    MatrixWorkspaceTester matrixWS;
    TSM_ASSERT_THROWS("Current implementation should throw runtime error.", matrixWS.getDimension("dim1"), std::runtime_error);
  }

  //Characterisation test. 
  void testGetNPoints()
  {
    using namespace Mantid::DataObjects;
    MatrixWorkspaceTester matrixWS;
    TSM_ASSERT_THROWS("Current implementation should throw runtime error.", matrixWS.getNPoints(), std::runtime_error);
  }

  //Characterisation test. 
  void testGetCellSingleParameterVersion()
  {
    using namespace Mantid::DataObjects;
    MatrixWorkspaceTester matrixWS;
    TSM_ASSERT_THROWS("Current implementation should throw runtime error.", matrixWS.getCell(1), std::runtime_error);
  }

  //Characterisation test. 
  void testGetCellDoubleParameterVersion()
  {
    using namespace Mantid::DataObjects;
    MatrixWorkspaceTester matrixWS;
    TSM_ASSERT_THROWS("Current implementation should throw runtime error.", matrixWS.getCell(1, 1), std::runtime_error);
  }

  //Characterisation test. 
  void testGetCellTripleParameterVersion()
  {
    using namespace Mantid::DataObjects;
    MatrixWorkspaceTester matrixWS;
    TSM_ASSERT_THROWS("Current implementation should throw runtime error.", matrixWS.getCell(1, 1, 1), std::runtime_error);
  }

  //Characterisation test. 
  void testGetCellFourParameterVersion()
  {
    using namespace Mantid::DataObjects;
    MatrixWorkspaceTester matrixWS;
    TSM_ASSERT_THROWS("Current implementation should throw runtime error.", matrixWS.getCell(1, 1, 1, 1), std::runtime_error);
  }

  //Characterisation test. 
  void testGetCellElipsisParameterVersion()
  {
    using namespace Mantid::DataObjects;
    MatrixWorkspaceTester matrixWS;
    TSM_ASSERT_THROWS("Current implementation should throw runtime error.", matrixWS.getCell(1, 1, 1, 1, 1, 1, 1, 1, 1), std::runtime_error);
  }


};

#endif /*IMD_MATRIX_WORKSPACETEST_H_*/