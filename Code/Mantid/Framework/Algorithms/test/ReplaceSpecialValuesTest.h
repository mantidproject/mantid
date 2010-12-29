#ifndef REPLACESPECIALVALUESTEST_H_
#define REPLACESPECIALVALUESTEST_H_

#include <cxxtest/TestSuite.h>

#include "WorkspaceCreationHelper.hh"
#include "MantidAlgorithms/ReplaceSpecialValues.h"
#include "MantidAPI/AnalysisDataService.h"
#include <limits>
#include <cmath>


using namespace Mantid::API;
using namespace Mantid::Kernel;

class ReplaceSpecialValuesTest : public CxxTest::TestSuite
{
public:
  void testName()
  {
    TS_ASSERT_EQUALS( alg.name(), "ReplaceSpecialValues" );
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( alg.version(), 1 );
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( alg.category(), "General" );
  }

  void testInit()
  {
    Mantid::Algorithms::ReplaceSpecialValues alg2;
    TS_ASSERT_THROWS_NOTHING( alg2.initialize() );
    TS_ASSERT( alg2.isInitialized() );

    const std::vector<Property*> props = alg2.getProperties();
    TS_ASSERT_EQUALS( props.size(), 9 );

    TS_ASSERT_EQUALS( props[0]->name(), "InputWorkspace" );
    TS_ASSERT( props[0]->isDefault() );
    TS_ASSERT( dynamic_cast<WorkspaceProperty<MatrixWorkspace>* >(props[0]) );

    TS_ASSERT_EQUALS( props[1]->name(), "OutputWorkspace" );
    TS_ASSERT( props[1]->isDefault() );
    TS_ASSERT( dynamic_cast<WorkspaceProperty<MatrixWorkspace>* >(props[1]) );

    TS_ASSERT_EQUALS( props[2]->name(), "NaNValue" );
    TS_ASSERT( props[2]->isDefault() );
    TS_ASSERT_EQUALS( props[3]->name(), "NaNError" );
    TS_ASSERT( props[3]->isDefault() );
    TS_ASSERT_EQUALS( props[4]->name(), "InfinityValue" );
    TS_ASSERT( props[4]->isDefault() );
    TS_ASSERT_EQUALS( props[5]->name(), "InfinityError" );
    TS_ASSERT( props[5]->isDefault() );
    TS_ASSERT_EQUALS( props[6]->name(), "BigNumberThreshold" );
    TS_ASSERT( props[6]->isDefault() );
    TS_ASSERT_EQUALS( props[7]->name(), "BigNumberValue" );
    TS_ASSERT( props[7]->isDefault() );
    TS_ASSERT_EQUALS( props[8]->name(), "BigNumberError" );
    TS_ASSERT( props[8]->isDefault() );
  }

  void testExecCheckBoth()
  {
    MatrixWorkspace_sptr inputWS = createWorkspace();
    AnalysisDataService::Instance().add("InputWS",inputWS);

    Mantid::Algorithms::ReplaceSpecialValues alg3;
    alg3.initialize();
    TS_ASSERT_THROWS_NOTHING( alg3.setPropertyValue("InputWorkspace","InputWS") );
    TS_ASSERT_THROWS_NOTHING( alg3.setPropertyValue("OutputWorkspace","WSCor") );
    TS_ASSERT_THROWS_NOTHING( alg3.setPropertyValue("NaNValue","-99.0") );
    TS_ASSERT_THROWS_NOTHING( alg3.setPropertyValue("NaNError","-50.0") );
    TS_ASSERT_THROWS_NOTHING( alg3.setPropertyValue("InfinityValue","999.0") );
    TS_ASSERT_THROWS_NOTHING( alg3.setPropertyValue("InfinityError","0.00005") );

    TS_ASSERT_THROWS_NOTHING( alg3.execute() );
    TS_ASSERT( alg3.isExecuted() );

    MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("WSCor")) );
    TS_ASSERT( result );

    checkValues(inputWS,result,1,1);

    AnalysisDataService::Instance().remove("InputWS");
    AnalysisDataService::Instance().remove("WSCor");
  }

  void testExecCheckNaN()
  {
    MatrixWorkspace_sptr inputWS = createWorkspace();
    AnalysisDataService::Instance().add("InputWS",inputWS);

    Mantid::Algorithms::ReplaceSpecialValues alg3;
    alg3.initialize();
    TS_ASSERT_THROWS_NOTHING( alg3.setPropertyValue("InputWorkspace","InputWS") );
    TS_ASSERT_THROWS_NOTHING( alg3.setPropertyValue("OutputWorkspace","WSCor") );
    TS_ASSERT_THROWS_NOTHING( alg3.setPropertyValue("NaNValue","-99.0") );
    TS_ASSERT_THROWS_NOTHING( alg3.setPropertyValue("NaNError","-50.0") );

    TS_ASSERT_THROWS_NOTHING( alg3.execute() );
    TS_ASSERT( alg3.isExecuted() );

    MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("WSCor")) );
    TS_ASSERT( result );

    checkValues(inputWS,result,1,0);

    AnalysisDataService::Instance().remove("InputWS");
    AnalysisDataService::Instance().remove("WSCor");
  }

  void testExecCheckInf()
  {
    MatrixWorkspace_sptr inputWS = createWorkspace();
    AnalysisDataService::Instance().add("InputWS",inputWS);

    Mantid::Algorithms::ReplaceSpecialValues alg3;
    alg3.initialize();
    TS_ASSERT_THROWS_NOTHING( alg3.setPropertyValue("InputWorkspace","InputWS") );
    TS_ASSERT_THROWS_NOTHING( alg3.setPropertyValue("OutputWorkspace","WSCor") );
    TS_ASSERT_THROWS_NOTHING( alg3.setPropertyValue("InfinityValue","999.0") );
    TS_ASSERT_THROWS_NOTHING( alg3.setPropertyValue("InfinityError","0.00005") );

    TS_ASSERT_THROWS_NOTHING( alg3.execute() );
    TS_ASSERT( alg3.isExecuted() );

    MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("WSCor")) );
    TS_ASSERT( result );

    checkValues(inputWS,result,0,1);

    AnalysisDataService::Instance().remove("InputWS");
    AnalysisDataService::Instance().remove("WSCor");
  }

  void testExecCheckBig()
  {
    MatrixWorkspace_sptr inputWS = createWorkspace();
    // Put in some 'big' values
    inputWS->dataY(0)[0] = 1.0E12;
    inputWS->dataY(0)[1] = 1.000001E10;    
    AnalysisDataService::Instance().add("InputWS",inputWS);

    Mantid::Algorithms::ReplaceSpecialValues alg3;
    alg3.initialize();
    TS_ASSERT_THROWS_NOTHING( alg3.setPropertyValue("InputWorkspace","InputWS") );
    TS_ASSERT_THROWS_NOTHING( alg3.setPropertyValue("OutputWorkspace","WSCor") );
    TS_ASSERT_THROWS_NOTHING( alg3.setPropertyValue("BigNumberThreshold","1.0E10") );
    TS_ASSERT_THROWS_NOTHING( alg3.setPropertyValue("BigNumberValue","999") );
    TS_ASSERT_THROWS_NOTHING( alg3.setPropertyValue("BigNumberError","0.00005") );

    TS_ASSERT_THROWS_NOTHING( alg3.execute() );
    TS_ASSERT( alg3.isExecuted() );

    MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("WSCor")) );
    TS_ASSERT( result );

    for (int i = 0; i < result->getNumberHistograms(); ++i)
    {
      for (int j = 0; j < 4; ++j)
      {
        if ((i==0 && j!=3) || (i==1 && j==0))
        {
          TS_ASSERT_EQUALS( result->readY(i)[j],999 );
          TS_ASSERT_EQUALS( result->readE(i)[j],0.00005 );
        }
        else if (inputWS->readY(i)[j] != inputWS->readY(i)[j]) //nan
        {
          TS_ASSERT_DIFFERS( result->readY(i)[j], result->readY(i)[j] );
        }
        else
        {
          TS_ASSERT_EQUALS( result->readY(i)[j], inputWS->readY(i)[j] );
        }
      }
    }

    AnalysisDataService::Instance().remove("InputWS");
    AnalysisDataService::Instance().remove("WSCor");
  }
		
  void testExecCheckNone()
  {
    MatrixWorkspace_sptr inputWS = createWorkspace();
    AnalysisDataService::Instance().add("InputWS",inputWS);

    Mantid::Algorithms::ReplaceSpecialValues alg3;
    alg3.initialize();
    TS_ASSERT_THROWS_NOTHING( alg3.setPropertyValue("InputWorkspace","InputWS") );
    TS_ASSERT_THROWS_NOTHING( alg3.setPropertyValue("OutputWorkspace","WSCor") );

    TS_ASSERT_THROWS_NOTHING( alg3.execute() );
    TS_ASSERT( !alg3.isExecuted() );

    MatrixWorkspace_sptr result;

    AnalysisDataService::Instance().remove("InputWS");
  }

  void checkValues(MatrixWorkspace_sptr inputWS, MatrixWorkspace_sptr result, bool naNCheck, bool infCheck)
  {

    for (int i = 0; i < result->getNumberHistograms(); ++i)
    {
      for (int j = 1; j < 5; ++j)
      {
        TS_ASSERT_EQUALS( result->dataX(i)[j-1], inputWS->dataX(i)[j-1] );

        if (infCheck && std::abs(inputWS->dataY(i)[j-1]) == std::numeric_limits<double>::infinity())
        {
          if (std::abs(result->dataY(i)[j-1]) == std::numeric_limits<double>::infinity())
          {
            TS_FAIL("Infinity detected that shold have been replaced");
          }
          else
          {
            TS_ASSERT_DELTA( result->dataY(i)[j-1], 999.0, 1e-8 );
            TS_ASSERT_DELTA( result->dataE(i)[j-1], 0.00005, 1e-8 );
          }
        }
        else if (naNCheck && inputWS->dataY(i)[j-1] != inputWS->dataY(i)[j-1]) //not equal to self == NaN
        {
          TS_ASSERT_DELTA( result->dataY(i)[j-1], -99.0, 1e-8 );
          TS_ASSERT_DELTA( result->dataE(i)[j-1], -50.0, 1e-8 );
        }
        else
        {
          if (!naNCheck && inputWS->dataY(i)[j-1] != inputWS->dataY(i)[j-1])
          {
            TS_ASSERT_DIFFERS( result->dataY(i)[j-1], result->dataY(i)[j-1] );
          } 
          else
          {
            TS_ASSERT_EQUALS( result->dataY(i)[j-1], inputWS->dataY(i)[j-1] );
          }
          TS_ASSERT_EQUALS( result->dataE(i)[j-1], inputWS->dataE(i)[j-1] );
        }
      }
    }
  }

  MatrixWorkspace_sptr createWorkspace()
  {
    MatrixWorkspace_sptr inputWS = WorkspaceCreationHelper::Create2DWorkspaceBinned(4,4,0.5);
    //put some infinitis and NaNs in there
    double inf = std::numeric_limits<double>::infinity();
    inputWS->dataY(0)[2] = inf;
    inputWS->dataY(1)[0] = -inf;
    double nan = std::numeric_limits<double>::quiet_NaN();
    inputWS->dataY(2)[3] = nan;
    inputWS->dataY(3)[1] = nan;

    return inputWS;
  }

private:
  Mantid::Algorithms::ReplaceSpecialValues alg;

};

#endif /*REPLACESPECIALVALUESTEST_H_*/
