#ifndef MANTID_ALGORITHMS_CONVERTAXISBYFORMULATEST_H_
#define MANTID_ALGORITHMS_CONVERTAXISBYFORMULATEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ConvertAxisByFormula.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::ConvertAxisByFormula;

class ConvertAxisByFormulaTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertAxisByFormulaTest *createSuite() { return new ConvertAxisByFormulaTest(); }
  static void destroySuite( ConvertAxisByFormulaTest *suite ) { delete suite; }

   void testSquareX()
  {
    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    Mantid::Algorithms::ConvertAxisByFormula alg;
    alg.initialize();

    std::string inputWs= alg.name() + "_testSquareX_Input";
    std::string resultWs= alg.name() + "_testSquareX_Result";

    AnalysisDataService::Instance().add(inputWs,WorkspaceCreationHelper::Create2DWorkspace123(10,10));
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace",inputWs) )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace",resultWs) )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Formula","x*x") )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Axis","X") )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("AxisTitle","XTitle") )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("AxisUnits","XUnit") )

    TS_ASSERT_THROWS_NOTHING( alg.execute() )
    TS_ASSERT( alg.isExecuted() )

    MatrixWorkspace_const_sptr in,result;
    TS_ASSERT_THROWS_NOTHING( in = boost::dynamic_pointer_cast<MatrixWorkspace>
                                (AnalysisDataService::Instance().retrieve(inputWs)) )
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<MatrixWorkspace>
                                (AnalysisDataService::Instance().retrieve(resultWs)) )

    MatrixWorkspace::const_iterator inIt(*in);
    for (MatrixWorkspace::const_iterator it(*result); it != it.end(); ++it,++inIt)
    {
      TS_ASSERT_EQUALS( it->X(), inIt->X()*inIt->X() )
      TS_ASSERT_EQUALS( it->Y(), inIt->Y() )
      TS_ASSERT_EQUALS( it->E(), inIt->E() )
    }

    AnalysisDataService::Instance().remove(inputWs);
    AnalysisDataService::Instance().remove(resultWs);
  }

};


#endif /* MANTID_ALGORITHMS_CONVERTAXISBYFORMULATEST_H_ */