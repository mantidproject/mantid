#ifndef PLOTASYMMETRYBYLOGVALUTEST_H_
#define PLOTASYMMETRYBYLOGVALUTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/PlotAsymmetryByLogValue.h"

using namespace Mantid::Algorithms;

class PlotAsymmetryByLogValueTest : public CxxTest::TestSuite
{
public:

    void testExec()
    {
        PlotAsymmetryByLogValue alg;
        alg.initialize();
        alg.setPropertyValue("FirstRun","../../../../Test/Nexus/MUSR00015189.nxs");
        alg.setPropertyValue("LastRun","../../../../Test/Nexus/MUSR00015199.nxs");
        alg.setPropertyValue("OutputWorkspace","PlotAsymmetryByLogValueTest_WS");
        alg.setPropertyValue("LogValue","Field_Danfysik");
        alg.setPropertyValue("Red","2");
        alg.setPropertyValue("Green","1");
        TS_ASSERT_THROWS_NOTHING(alg.execute());
    }
  
};

#endif /*PLOTASYMMETRYBYLOGVALUTEST_H_*/
