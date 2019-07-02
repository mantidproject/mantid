// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef LoadRawSpectrum0Test_H_
#define LoadRawSpectrum0Test_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/LoadRawSpectrum0.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Unit.h"

#include <Poco/Path.h>
#include <boost/lexical_cast.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class LoadRawSpectrum0Test : public CxxTest::TestSuite {
public:
  static LoadRawSpectrum0Test *createSuite() {
    return new LoadRawSpectrum0Test();
  }
  static void destroySuite(LoadRawSpectrum0Test *suite) { delete suite; }

  LoadRawSpectrum0Test() {
    // Path to test input file assumes Test directory checked out from SVN
    inputFile = "HET15869.raw";
  }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
  }

  void testExec() {
    /* std::string s;
     std::getline(std::cin,s);*/
    if (!loader.isInitialized())
      loader.initialize();

    // Should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(loader.execute(), const std::runtime_error &);

    // Now set it...
    loader.setPropertyValue("Filename", inputFile);

    outputSpace = "outer";
    loader.setPropertyValue("OutputWorkspace", outputSpace);

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieve(outputSpace));
    Workspace2D_sptr output2D =
        boost::dynamic_pointer_cast<Workspace2D>(output);
    // Should be 2584 for file HET15869.RAW
    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 1);
    // Check two X vectors are the same

    // Check one particular value
    TS_ASSERT_EQUALS(output2D->dataY(0)[777], 355);
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS(output2D->dataE(0)[777],
                     std::sqrt(output2D->dataY(0)[777]));
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS(output2D->dataX(0)[777], 554.1875);

    // Check the unit has been set correctly
    TS_ASSERT_EQUALS(output2D->getAxis(0)->unit()->unitID(), "TOF")
    TS_ASSERT(!output2D->isDistribution())

    // Check the proton charge has been set correctly
    TS_ASSERT_DELTA(output2D->run().getProtonCharge(), 171.0353, 0.0001)
    AnalysisDataService::Instance().remove(outputSpace);
  }

  void testMultiPeriod() {
    LoadRawSpectrum0 loader5;
    loader5.initialize();
    loader5.setPropertyValue("Filename", "CSP78173.raw");
    loader5.setPropertyValue("OutputWorkspace", "multiperiod");

    TS_ASSERT_THROWS_NOTHING(loader5.execute())
    TS_ASSERT(loader5.isExecuted())

    WorkspaceGroup_sptr work_out;
    TS_ASSERT_THROWS_NOTHING(
        work_out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
            "multiperiod"));

    Workspace_sptr wsSptr =
        AnalysisDataService::Instance().retrieve("multiperiod");
    WorkspaceGroup_sptr sptrWSGrp =
        boost::dynamic_pointer_cast<WorkspaceGroup>(wsSptr);
    std::vector<std::string> wsNamevec;
    wsNamevec = sptrWSGrp->getNames();
    int period = 1;
    std::vector<std::string>::const_iterator it = wsNamevec.begin();
    for (; it != wsNamevec.end(); it++) {
      std::stringstream count;
      count << period;
      std::string wsName = "multiperiod_" + count.str();
      TS_ASSERT_EQUALS(*it, wsName)
      period++;
    }
    std::vector<std::string>::const_iterator itr1 = wsNamevec.begin();
    int expectedPeriod = 0;
    for (; itr1 != wsNamevec.end(); itr1++) {
      MatrixWorkspace_sptr outsptr;
      TS_ASSERT_THROWS_NOTHING(
          outsptr = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
              (*itr1)));
      doTestMultiPeriodWorkspace(outsptr, ++expectedPeriod);
    }
    std::vector<std::string>::const_iterator itr = wsNamevec.begin();

    MatrixWorkspace_sptr outsptr1;
    TS_ASSERT_THROWS_NOTHING(
        outsptr1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            (*itr)));
    MatrixWorkspace_sptr outsptr2;
    TS_ASSERT_THROWS_NOTHING(
        outsptr2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            (*++itr)));

    TS_ASSERT_EQUALS(outsptr1->dataX(0), outsptr2->dataX(0))

    TS_ASSERT_EQUALS(&(outsptr1->sample()), &(outsptr2->sample()))
    TS_ASSERT_DIFFERS(&(outsptr1->run()), &(outsptr2->run()))
  }

private:
  /// Helper method to run common set of tests on a workspace in a multi-period
  /// group.
  void doTestMultiPeriodWorkspace(MatrixWorkspace_sptr workspace,
                                  int expected_period) {
    // Check the current period property.
    const Mantid::API::Run &run = workspace->run();
    Property *prop = run.getLogData("current_period");
    PropertyWithValue<int> *current_period_property =
        dynamic_cast<PropertyWithValue<int> *>(prop);
    TS_ASSERT(current_period_property != nullptr);
    int actual_period;
    actual_period = boost::lexical_cast<int>(current_period_property->value());
    TS_ASSERT_EQUALS(expected_period, actual_period);
    // Check the period n property.
    std::stringstream stream;
    stream << "period " << actual_period;
    TSM_ASSERT_THROWS_NOTHING("period number series could not be found.",
                              run.getLogData(stream.str()));
  }

  LoadRawSpectrum0 loader;
  std::string inputFile;
  std::string outputSpace;
};

#endif /*LoadRawSpectrum0Test_H_*/
