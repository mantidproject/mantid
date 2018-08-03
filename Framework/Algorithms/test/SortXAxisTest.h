#ifndef MANTID_ALGORITHMS_SORTXAXISTEST_H_
#define MANTID_ALGORITHMS_SORTXAXISTEST_H_

#include "MantidAlgorithms/SortXAxis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Workspace.h"
#include <cxxtest/TestSuite.h>

namespace {
MatrixWorkspace_sptr createWorkspace(const HistogramX &xData,
                                     const HistogramY &yData,
                                     const HistogramE &eData,
                                     const int nSpec = 1) {

  Workspace2D_sptr outWS = boost::make_shared<Workspace2D>();
  outWS->initialize(nSpec, xData.size(), yData.size());
  for (int i = 0; i < nSpec; ++i) {
    outWS->mutableY(i) = yData;
    outWS->mutableE(i) = eData;
    outWS->mutableX(i) = xData;
  }

  outWS->getAxis(0)->unit() = UnitFactory::Instance().create("Wavelength");

  return outWS;
}

class SortXAxisTest : public CxxTest::TestSuite {
    // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SortXAxisTest *createSuite() { return new SortXAxisTest(); }
  static void destroySuite(SortXAxisTest *suite) { delete suite; }

  void testXAscending(){
    std::vector<double> xData = {1,2,3};
    std::vector<double> yData = {1,2,3};
    std::vector<double> eData = {1,2,3};

    MatrixWorkspace_sptr unsortedws = createWorkspace(xData, yData, eData);

    SortXAxis alg;
    alg.setProperty("InputWorkspacd", "unsortedws");
    alg.setProperty("OutputWorkspace", "sortedws");
    alg.execute();
    MatrixWorkspace_sptr sortedws = 

  }

  void testXDescending(){}

  void testOnMultipleSpectrum(){}

  void testSortsXHistogramAscending(){}

  void testSortsXHistogramDescending(){}

  void testSortXWorksChild(){}

  void testDxMultipleSpectrum(){}

  void testDxHistogramAscending(){}

  void testSortDescending(){}
};


#endif /*MANTID_ALGORITHMS_SORTXAXISTEST_H_*/
