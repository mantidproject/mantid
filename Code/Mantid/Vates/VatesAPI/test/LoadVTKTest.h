#ifndef LOADVTK_TEST_H_
#define LOADVTK_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/LoadVTK.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"


using namespace Mantid::API;
using namespace Mantid::VATES;
using namespace Mantid::Geometry;

class LoadVTKTest: public CxxTest::TestSuite
{
public:

  void test_catagory()
  {
    LoadVTK loadVTK;
    Algorithm& alg = loadVTK;
    TS_ASSERT_EQUALS("MDAlgorithms", alg.category());
  }

  void test_version()
  {
    LoadVTK loadVTK;
    TS_ASSERT_EQUALS(1, loadVTK.version());
  }

  void test_Properties()
  {
    LoadVTK loadVTK;
    loadVTK.initialize();
    TS_ASSERT_THROWS_NOTHING(loadVTK.setPropertyValue("Filename", "iron_protein.vtk"));
    TS_ASSERT_THROWS_NOTHING(loadVTK.setPropertyValue("OutputWorkspace", "OutWS"));
    TS_ASSERT_THROWS_NOTHING(loadVTK.setPropertyValue("SignalArrayName", "scalars"));
    TS_ASSERT( loadVTK.isInitialized() )
  }

  void do_check_dimension(IMDDimension_const_sptr dimension, const std::string& expectedName, const double expectedMin, const double expectedMax, const int expectedNBins)
  {
    TSM_ASSERT_EQUALS("Name is wrong.", dimension->getName(), expectedName);
    TSM_ASSERT_EQUALS("Id is wrong.", dimension->getDimensionId(), expectedName);
    TSM_ASSERT_DELTA("Minimum is wrong.", dimension->getMinimum(), expectedMin, 0.01);
    TSM_ASSERT_DELTA("Maximum is wrong.", dimension->getMaximum(), expectedMax, 0.01);
    TSM_ASSERT_EQUALS("Number of bins is wrong.", dimension->getNBins(), expectedNBins);
  }

  void do_test_bad_arrays(const std::string& signalArrayName, const std::string& errorSQArrayName="")
  {
    LoadVTK loadVTK;
    loadVTK.setRethrows(true);
    loadVTK.initialize();
    loadVTK.setPropertyValue("Filename", "iron_protein.vtk");
    loadVTK.setPropertyValue("OutputWorkspace", "OutWS");
    loadVTK.setPropertyValue("SignalArrayName", signalArrayName);
    loadVTK.setPropertyValue("ErrorSQArrayName", errorSQArrayName);
    TS_ASSERT_THROWS(loadVTK.execute(), std::invalid_argument&);
  }

  void test_badSignalArray()
  {
    const std::string signalArray = "?!"; // Not a name that exists.
    do_test_bad_arrays(signalArray);
  }

  void test_badErrorSQArray()
  {
    const std::string signalArray = "scalar_array";  // Does exist
    const std::string errorSQArray = "?!"; // Not a name that exists.
    do_test_bad_arrays(signalArray, errorSQArray);
  }

  void test_LoadVTKFile()
  {
    const std::string outWSName = "OutWS";

    LoadVTK loadVTK;
    loadVTK.setRethrows(true);
    loadVTK.initialize();
    loadVTK.setPropertyValue("Filename", "iron_protein.vtk");
    loadVTK.setPropertyValue("OutputWorkspace", outWSName);
    loadVTK.setPropertyValue("SignalArrayName", "scalar_array");
    loadVTK.setPropertyValue("ErrorSQArrayName", "scalar_array");
    loadVTK.execute();

    IMDHistoWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<IMDHistoWorkspace>(outWSName);

    TS_ASSERT_EQUALS(3, outWS->getNumDims());
    do_check_dimension(outWS->getDimension(0), "X", 0, 67, 68); // These numbers are expected min, max, and nbins known from the input file for dim x.
    do_check_dimension(outWS->getDimension(1), "Y", 0, 67, 68); // These numbers are expected min, max, and nbins known from the input file for dim y.
    do_check_dimension(outWS->getDimension(2), "Z", 0, 67, 68); // These numbers are expected min, max, and nbins known from the input file for dim z.

    // Quick check of loaded data.
    TS_ASSERT_EQUALS(10, outWS->getSignalAt(0));
    TS_ASSERT_EQUALS(std::sqrt(10), outWS->getErrorAt(0));

  }

};

#endif
