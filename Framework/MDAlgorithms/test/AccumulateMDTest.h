#ifndef MANTID_MDALGORITHMS_ACCUMULATEMDTEST_H_
#define MANTID_MDALGORITHMS_ACCUMULATEMDTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidMDAlgorithms/AccumulateMD.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidKernel/ConfigService.h"
#include <Poco/Path.h>
#include <Poco/File.h>

using Mantid::MDAlgorithms::AccumulateMD;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class AccumulateMDTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AccumulateMDTest *createSuite() { return new AccumulateMDTest(); }

  static void destroySuite(AccumulateMDTest *suite) { delete suite; }

  void test_Init() {
    AccumulateMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
  }

  void test_pad_parameter_vector_empty() {
    std::vector<double> test_param_vector;
    unsigned long grow_to = 8;
    Mantid::MDAlgorithms::padParameterVector(test_param_vector, grow_to);

    TS_ASSERT_EQUALS(test_param_vector.size(), 8);
    TS_ASSERT_EQUALS(test_param_vector[4], 0.0);
  }

  void test_pad_parameter_vector_values() {
    std::vector<double> test_param_vector(1, 3.7);
    unsigned long grow_to = 8;
    Mantid::MDAlgorithms::padParameterVector(test_param_vector, grow_to);

    TS_ASSERT_EQUALS(test_param_vector.size(), 8);
    TS_ASSERT_EQUALS(test_param_vector[4], 3.7);
  }

  void test_filter_to_existing_sources_file_nonexist() {
    // Create vector of data_sources to filter
    std::vector<std::string> data_sources;

    // Create vector for other parameters
    std::vector<double> psi(1, 0.0);
    std::vector<double> gl(1, 0.0);
    std::vector<double> gs(1, 0.0);
    std::vector<double> efix(1, 0.0);

    // Add absolute path to a file which doesn't exist
    Poco::Path filepath =
        Poco::Path(Mantid::Kernel::ConfigService::Instance().getTempDir(),
                   "ACCUMULATEMDTEST_NONEXISTENTFILE");
    data_sources.push_back(filepath.toString());

    Mantid::MDAlgorithms::filterToExistingSources(data_sources, psi, gl, gs,
                                                  efix);

    TS_ASSERT(data_sources.empty());
  }

  void test_filter_to_existing_sources_workspace_nonexist() {
    // Create vector of data_sources to filter
    std::vector<std::string> data_sources;

    // Create vector for other parameters
    std::vector<double> psi(1, 0.0);
    std::vector<double> gl(1, 0.0);
    std::vector<double> gs(1, 0.0);
    std::vector<double> efix(1, 0.0);

    data_sources.push_back("ACCUMULATEMDTEST_NONEXISTENTWORKSPACE");

    Mantid::MDAlgorithms::filterToExistingSources(data_sources, psi, gl, gs,
                                                  efix);

    TS_ASSERT(data_sources.empty());
  }

  void test_filter_to_existing_sources_workspace_exist() {
    // Create vector of data_sources to filter
    std::vector<std::string> data_sources;

    // Create vector for other parameters
    std::vector<double> psi(1, 0.0);
    std::vector<double> gl(1, 0.0);
    std::vector<double> gs(1, 0.0);
    std::vector<double> efix(1, 0.0);

    // Create a cheap workspace
    std::string ws_name = "ACCUMULATEMDTEST_EXISTENTWORKSPACE";
    auto bkgWS = WorkspaceCreationHelper::Create1DWorkspaceRand(1);
    // add to ADS
    AnalysisDataService::Instance().add(ws_name, bkgWS);

    data_sources.push_back(ws_name);

    Mantid::MDAlgorithms::filterToExistingSources(data_sources, psi, gl, gs,
                                                  efix);

    TS_ASSERT(!data_sources.empty());

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(ws_name);
  }

  void test_filter_to_existing_sources_file_exist() {
    // Create vector of data_sources to filter
    std::vector<std::string> data_sources;

    // Create vector for other parameters
    std::vector<double> psi(1, 0.0);
    std::vector<double> gl(1, 0.0);
    std::vector<double> gs(1, 0.0);
    std::vector<double> efix(1, 0.0);

    // Create a temporary file to find
    Poco::Path filepath =
        Poco::Path(Mantid::Kernel::ConfigService::Instance().getTempDir(),
                   "ACCUMULATEMDTEST_EXISTENTFILE");
    Poco::File existent_file(filepath);
    existent_file.createFile();
    data_sources.push_back(filepath.toString());

    Mantid::MDAlgorithms::filterToExistingSources(data_sources, psi, gl, gs,
                                                  efix);

    TS_ASSERT(!data_sources.empty());

    // Remove the temporary file
    existent_file.remove();
  }

  void test_filter_to_new_none_new() {
    std::vector<std::string> input_data;
    input_data.push_back("test1");
    input_data.push_back("test2");
    input_data.push_back("test3");
    std::vector<std::string> current_data = input_data;

    // Create vector for other parameters
    std::vector<double> psi(3, 0.0);
    std::vector<double> gl(3, 0.0);
    std::vector<double> gs(3, 0.0);
    std::vector<double> efix(3, 0.0);

    Mantid::MDAlgorithms::filterToNew(input_data, current_data, psi, gl, gs,
                                      efix);

    // Two input vectors were identical, so we should get an empty vector back
    TS_ASSERT(input_data.empty());

    // Parameter vectors should also have been emptied
    TS_ASSERT(psi.empty());
    TS_ASSERT(gl.empty());
    TS_ASSERT(gs.empty());
    TS_ASSERT(efix.empty());
  }

  void test_filter_to_new() {
    std::vector<std::string> input_data;
    input_data.push_back("test1");
    input_data.push_back("test2");
    input_data.push_back("test3");
    input_data.push_back("test4");
    input_data.push_back("test5");

    std::vector<std::string> current_data;
    current_data.push_back("test1");
    current_data.push_back("test3");
    current_data.push_back("test4");

    // Create vector for other parameters
    std::vector<double> psi(5, 0.0);
    std::vector<double> gl(5, 0.0);
    std::vector<double> gs(5, 0.0);
    std::vector<double> efix(5, 0.0);

    Mantid::MDAlgorithms::filterToNew(input_data, current_data, psi, gl, gs,
                                      efix);

    // test2 and test5 is new data (it is in input_data but not current_data)
    // and so should be returned in the vector
    TS_ASSERT_EQUALS(input_data[0], "test2");
    TS_ASSERT_EQUALS(input_data[1], "test5");

    // Parameter vectors should have been reduced to the same size
    TS_ASSERT_EQUALS(psi.size(), input_data.size());
    TS_ASSERT_EQUALS(gl.size(), input_data.size());
    TS_ASSERT_EQUALS(gs.size(), input_data.size());
    TS_ASSERT_EQUALS(efix.size(), input_data.size());
  }

  void test_insert_data_sources() {
    std::string dataSources = "test1,test2,test3";
    std::set<std::string> dataSourcesSet;
    Mantid::MDAlgorithms::insertDataSources(dataSources, dataSourcesSet);

    // Check set contains "test1", "test2" and "test3"
    std::set<std::string>::iterator iter;
    iter = dataSourcesSet.find("test1");
    TS_ASSERT(iter != dataSourcesSet.end());

    iter = dataSourcesSet.find("test2");
    TS_ASSERT(iter != dataSourcesSet.end());

    iter = dataSourcesSet.find("test3");
    TS_ASSERT(iter != dataSourcesSet.end());
  }

  void test_insert_data_sources_with_whitespace() {
    std::string dataSources = " test1,test2 , test3";
    std::set<std::string> dataSourcesSet;
    Mantid::MDAlgorithms::insertDataSources(dataSources, dataSourcesSet);

    // Check set contains "test1", "test2" and "test3"
    std::set<std::string>::iterator iter;
    iter = dataSourcesSet.find("test1");
    TS_ASSERT(iter != dataSourcesSet.end());

    iter = dataSourcesSet.find("test2");
    TS_ASSERT(iter != dataSourcesSet.end());

    iter = dataSourcesSet.find("test3");
    TS_ASSERT(iter != dataSourcesSet.end());
  }

  void test_get_historical_data_sources() {
    // Create workspace with CreateMD
    // Add workspace to ADS
    // Append data with AccumulateMD
    // const WorkspaceHistory wsHistory = input_ws->getHistory();
    // std::vector<std::string> current_data =
    // getHistoricalDataSources(wsHistory);
    // Test all the data sources from CreateMD and AccumulateMD are in resultant
    // vector
  }
};

#endif /* MANTID_MDALGORITHMS_ACCUMULATEMDTEST_H_ */
