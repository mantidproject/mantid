#ifndef STRIPVANADIUMPEAKSTEST_H_
#define STRIPVANADIUMPEAKSTEST_H_

#include <cxxtest/TestSuite.h>
#include "WorkspaceCreationHelper.hh"
#include "MantidAlgorithms/StripVanadiumPeaks.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VectorHelper.h"

using namespace Mantid::API;
using namespace Mantid::Kernel::VectorHelper;
using Mantid::Algorithms::StripVanadiumPeaks;

class StripVanadiumPeaksTest : public CxxTest::TestSuite
{
public:

  void testTheBasics()
  {
    StripVanadiumPeaks strip;
    TS_ASSERT_EQUALS( strip.name(), "StripVanadiumPeaks" );
    TS_ASSERT_EQUALS( strip.version(), 1 );
    TS_ASSERT_EQUALS( strip.category(), "General" );
  }

  void testInit()
  {
    StripVanadiumPeaks strip;
    TS_ASSERT_THROWS_NOTHING( strip.initialize() );
    TS_ASSERT( strip.isInitialized() );
  }

  void testExec()
  {
    std::string inputWSName("PG3_733");
    std::string outputWSName("PG3_733_stripped");

    // Start by loading our NXS file
    IAlgorithm* loader = Mantid::API::FrameworkManager::Instance().createAlgorithm("LoadNexus");
    loader->setPropertyValue("Filename","../../../../Test/Data/PG3_733.nxs");
    loader->setPropertyValue("OutputWorkspace", inputWSName);
    loader->execute();
    TS_ASSERT( loader->isExecuted());

    StripVanadiumPeaks strip;
    if ( !strip.isInitialized() ) strip.initialize();

    strip.setPropertyValue("InputWorkspace",inputWSName);
    strip.setPropertyValue("OutputWorkspace",outputWSName);
    strip.setPropertyValue("PeakWidthPercent", "3.0");
    strip.setPropertyValue("AlternativePeakPositions", "");
    strip.execute();
    TS_ASSERT( strip.isExecuted() );

    MatrixWorkspace_const_sptr output;
    output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputWSName));

    //Get a spectrum
    MantidVec X = output->dataX(2);
    MantidVec Y = output->dataX(2);

    //Check the height at a couple of peak position
    int bin;
    bin = getBinIndex(X, 0.8113);
    TS_ASSERT_LESS_THAN( Y[bin], 11407);
    bin = getBinIndex(X, 0.8758);
    TS_ASSERT_LESS_THAN( Y[bin], 10850);

    AnalysisDataService::Instance().remove(outputWSName);
    AnalysisDataService::Instance().remove(inputWSName);
  }

private:
};

#endif /*STRIPVANADIUMPEAKSTEST_H_*/

