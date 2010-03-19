#ifndef SCALETEST_H_
#define SCALETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/Scale.h"
#include "WorkspaceCreationHelper.hh"

class ScaleTest : public CxxTest::TestSuite
{
public:
  void testName()
  {
    TS_ASSERT_EQUALS( scale.name(), "Scale" )
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
      TS_ASSERT_EQUALS( it->X(), inIt->X() )
      TS_ASSERT_EQUALS( it->Y(), 2.5*inIt->Y() )
      TS_ASSERT_EQUALS( it->E(), 2.5*inIt->E() )
    }

    AnalysisDataService::Instance().remove("tomultiply");
    AnalysisDataService::Instance().remove("multiplied");
  }

  void testAdd()
  {
    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    Mantid::Algorithms::Scale scale2;
    scale2.initialize();

    AnalysisDataService::Instance().add("toadd",WorkspaceCreationHelper::Create2DWorkspace123(10,10));
    TS_ASSERT_THROWS_NOTHING( scale2.setPropertyValue("InputWorkspace","toadd") )
    TS_ASSERT_THROWS_NOTHING( scale2.setPropertyValue("OutputWorkspace","added") )
    TS_ASSERT_THROWS_NOTHING( scale2.setPropertyValue("Factor","-100.0") )
    TS_ASSERT_THROWS_NOTHING( scale2.setPropertyValue("Operation","Add") )

    TS_ASSERT_THROWS_NOTHING( scale2.execute() )
    TS_ASSERT( scale2.isExecuted() )

    MatrixWorkspace_const_sptr in,result;
    TS_ASSERT_THROWS_NOTHING( in = boost::dynamic_pointer_cast<MatrixWorkspace>
                                (AnalysisDataService::Instance().retrieve("toadd")) )
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<MatrixWorkspace>
                                (AnalysisDataService::Instance().retrieve("added")) )

    MatrixWorkspace::const_iterator inIt(*in);
    for (MatrixWorkspace::const_iterator it(*result); it != it.end(); ++it,++inIt)
    {
      TS_ASSERT_EQUALS( it->X(), inIt->X() )
      TS_ASSERT_EQUALS( it->Y(), inIt->Y()-100.0 )
      TS_ASSERT_EQUALS( it->E(), inIt->E() )
    }

    AnalysisDataService::Instance().remove("toadd");
    AnalysisDataService::Instance().remove("added");
  }

private:
  Mantid::Algorithms::Scale scale;
};

#endif /*SCALETEST_H_*/
