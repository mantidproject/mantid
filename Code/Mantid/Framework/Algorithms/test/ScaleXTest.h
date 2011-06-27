#ifndef SCALEXTEST_H_
#define SCALEXTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/ScaleX.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::MantidVec;

class ScaleXTest : public CxxTest::TestSuite
{
public:
  void testName()
  {
    TS_ASSERT_EQUALS( scale.name(), "ScaleX" )
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( scale.version(), 1 )
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( scale.category(), "Arithmetic" )
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( scale.initialize() )
    TS_ASSERT( scale.isInitialized() )
  }

  void testMultiply()
  {
    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    if (!scale.isInitialized()) scale.initialize();

    AnalysisDataService::Instance().add("tomultiply",WorkspaceCreationHelper::Create2DWorkspace123(10,10));
    TS_ASSERT_THROWS_NOTHING( scale.setPropertyValue("InputWorkspace","tomultiply") )
    TS_ASSERT_THROWS_NOTHING( scale.setPropertyValue("OutputWorkspace","multiplied") )
    TS_ASSERT_THROWS_NOTHING( scale.setPropertyValue("Factor","2.5") )

    TS_ASSERT_THROWS_NOTHING( scale.execute() )
    TS_ASSERT( scale.isExecuted() )

    MatrixWorkspace_const_sptr in,result;
    TS_ASSERT_THROWS_NOTHING( in = boost::dynamic_pointer_cast<MatrixWorkspace>
                                (AnalysisDataService::Instance().retrieve("tomultiply")) )
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<MatrixWorkspace>
                                (AnalysisDataService::Instance().retrieve("multiplied")) )

    MatrixWorkspace::const_iterator inIt(*in);
    for (MatrixWorkspace::const_iterator it(*result); it != it.end(); ++it,++inIt)
    {
      TS_ASSERT_EQUALS( it->X(), 2.5*inIt->X() )
      TS_ASSERT_EQUALS( it->Y(), inIt->Y() )
      TS_ASSERT_EQUALS( it->E(), inIt->E() )
    }

    AnalysisDataService::Instance().remove("tomultiply");
    AnalysisDataService::Instance().remove("multiplied");
  }

private:
  Mantid::Algorithms::ScaleX scale;
};

#endif /*SCALEXTEST_H_*/
