#ifndef MANTID_MDALGORITHMS_ACCUMULATEMDTEST_H_
#define MANTID_MDALGORITHMS_ACCUMULATEMDTEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/ConfigService.h"
#include "MantidMDAlgorithms/AccumulateMD.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <Poco/File.h>
#include <Poco/Path.h>
#include <cxxtest/TestSuite.h>

using Mantid::MDAlgorithms::AccumulateMD;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

using Mantid::DataObjects::MDEventsTestHelper::makeAnyMDEW;

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
    size_t grow_to = 8;
    Mantid::MDAlgorithms::padParameterVector(test_param_vector, grow_to);

    TS_ASSERT_EQUALS(test_param_vector.size(), 8);
    TS_ASSERT_EQUALS(test_param_vector[4], 0.0);
  }

  void test_pad_parameter_vector_values() {
    std::vector<double> test_param_vector(1, 3.7);
    size_t grow_to = 8;
    Mantid::MDAlgorithms::padParameterVector(test_param_vector, grow_to);

    TS_ASSERT_EQUALS(test_param_vector.size(), 8);
    TS_ASSERT_EQUALS(test_param_vector[4], 3.7);
  }

  void test_filter_to_existing_sources_file_nonexist() {

    // Create vector for other parameters
    std::vector<double> psi(1, 0.0);
    std::vector<double> gl(1, 0.0);
    std::vector<double> gs(1, 0.0);
    std::vector<double> efix(1, 0.0);

    // Add absolute path to a file which doesn't exist
    Poco::Path filepath =
        Poco::Path(Mantid::Kernel::ConfigService::Instance().getTempDir(),
                   "ACCUMULATEMDTEST_NONEXISTENTFILE");

    // Create vector of data_sources to filter
    std::vector<std::string> data_sources{filepath.toString()};

    Mantid::MDAlgorithms::filterToExistingSources(data_sources, psi, gl, gs,
                                                  efix);

    TS_ASSERT(data_sources.empty());
  }

  void test_filter_to_existing_sources_workspace_nonexist() {

    // Create vector for other parameters
    std::vector<double> psi(1, 0.0);
    std::vector<double> gl(1, 0.0);
    std::vector<double> gs(1, 0.0);
    std::vector<double> efix(1, 0.0);

    // Create vector of data_sources to filter
    std::vector<std::string> data_sources{
        "ACCUMULATEMDTEST_NONEXISTENTWORKSPACE"};

    Mantid::MDAlgorithms::filterToExistingSources(data_sources, psi, gl, gs,
                                                  efix);

    TS_ASSERT(data_sources.empty());
  }

  void test_filter_to_existing_sources_workspace_exist() {

    // Create vector for other parameters
    std::vector<double> psi(1, 0.0);
    std::vector<double> gl(1, 0.0);
    std::vector<double> gs(1, 0.0);
    std::vector<double> efix(1, 0.0);

    // Create a cheap workspace
    std::string ws_name = "ACCUMULATEMDTEST_EXISTENTWORKSPACE";
    auto bkg_ws = WorkspaceCreationHelper::create1DWorkspaceRand(1, true);
    // add to ADS (no choice but to use ADS here)
    AnalysisDataService::Instance().add(ws_name, bkg_ws);

    // Create vector of data_sources to filter
    std::vector<std::string> data_sources{ws_name};

    Mantid::MDAlgorithms::filterToExistingSources(data_sources, psi, gl, gs,
                                                  efix);

    TS_ASSERT(!data_sources.empty());

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(ws_name);
  }

  void test_filter_to_existing_sources_file_exist() {

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
    // Create vector of data_sources to filter
    std::vector<std::string> data_sources{filepath.toString()};

    Mantid::MDAlgorithms::filterToExistingSources(data_sources, psi, gl, gs,
                                                  efix);

    TS_ASSERT(!data_sources.empty());

    // Remove the temporary file
    existent_file.remove();
  }

  void test_filter_to_new_none_new() {
    std::vector<std::string> input_data{"test1", "test2", "test3"};
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
    std::vector<std::string> input_data{"test1", "test2", "test3", "test4",
                                        "test5"};
    std::vector<std::string> current_data{"test1", "test3", "test4"};

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
    std::string data_sources = "test1,test2,test3";
    std::unordered_set<std::string> data_sources_set;
    Mantid::MDAlgorithms::insertDataSources(data_sources, data_sources_set);

    // Check set contains "test1", "test2" and "test3"
    auto iter = data_sources_set.find("test1");
    TS_ASSERT(iter != data_sources_set.end());

    iter = data_sources_set.find("test2");
    TS_ASSERT(iter != data_sources_set.end());

    iter = data_sources_set.find("test3");
    TS_ASSERT(iter != data_sources_set.end());
  }

  void test_insert_data_sources_with_whitespace() {
    std::string data_sources = " test1,test2 , test3";
    std::unordered_set<std::string> data_sources_set;
    Mantid::MDAlgorithms::insertDataSources(data_sources, data_sources_set);

    // Check set contains "test1", "test2" and "test3"
    auto iter = data_sources_set.find("test1");
    TS_ASSERT(iter != data_sources_set.end());

    iter = data_sources_set.find("test2");
    TS_ASSERT(iter != data_sources_set.end());

    iter = data_sources_set.find("test3");
    TS_ASSERT(iter != data_sources_set.end());
  }

  void test_algorithm_success_append_data() {

    auto sim_alg = Mantid::API::AlgorithmManager::Instance().create(
        "CreateSimulationWorkspace");
    sim_alg->initialize();
    sim_alg->setPropertyValue("Instrument", "MAR");
    sim_alg->setPropertyValue("BinParams", "-3,1,3");
    sim_alg->setPropertyValue("UnitX", "DeltaE");
    sim_alg->setPropertyValue("OutputWorkspace", "data_source_1");
    sim_alg->execute();

    sim_alg->setPropertyValue("OutputWorkspace", "data_source_2");
    sim_alg->execute();

    auto log_alg =
        Mantid::API::AlgorithmManager::Instance().create("AddSampleLog");
    log_alg->initialize();
    log_alg->setProperty("Workspace", "data_source_1");
    log_alg->setPropertyValue("LogName", "Ei");
    log_alg->setPropertyValue("LogText", "3.0");
    log_alg->setPropertyValue("LogType", "Number");
    log_alg->execute();

    log_alg->setProperty("Workspace", "data_source_2");
    log_alg->execute();

    auto create_alg =
        Mantid::API::AlgorithmManager::Instance().create("CreateMD");
    create_alg->setRethrows(true);
    create_alg->initialize();
    create_alg->setPropertyValue("OutputWorkspace", "md_sample_workspace");
    create_alg->setPropertyValue("DataSources", "data_source_1");
    create_alg->setPropertyValue("Alatt", "1,1,1");
    create_alg->setPropertyValue("Angdeg", "90,90,90");
    create_alg->setPropertyValue("Efix", "12.0");
    create_alg->setPropertyValue("u", "1,0,0");
    create_alg->setPropertyValue("v", "0,1,0");
    create_alg->execute();
    // IMDEventWorkspace_sptr in_ws =
    // create_alg->getProperty("OutputWorkspace");
    IMDEventWorkspace_sptr in_ws =
        boost::dynamic_pointer_cast<IMDEventWorkspace>(
            AnalysisDataService::Instance().retrieve("md_sample_workspace"));

    AccumulateMD acc_alg;
    acc_alg.initialize();
    acc_alg.setPropertyValue("InputWorkspace", "md_sample_workspace");
    acc_alg.setPropertyValue("OutputWorkspace", "accumulated_workspace");
    acc_alg.setPropertyValue("DataSources", "data_source_2");
    acc_alg.setPropertyValue("Alatt", "1.4165,1.4165,1.4165");
    acc_alg.setPropertyValue("Angdeg", "90,90,90");
    acc_alg.setPropertyValue("u", "1,0,0");
    acc_alg.setPropertyValue("v", "0,1,0");
    TS_ASSERT_THROWS_NOTHING(acc_alg.execute());
    // IMDEventWorkspace_sptr out_ws = acc_alg.getProperty("OutputWorkspace");
    IMDEventWorkspace_sptr out_ws =
        boost::dynamic_pointer_cast<IMDEventWorkspace>(
            AnalysisDataService::Instance().retrieve("accumulated_workspace"));

    // Should have the same number of events in output as the sum of the inputs
    TS_ASSERT_EQUALS(2 * in_ws->getNEvents(), out_ws->getNEvents());
  }

  void test_algorithm_success_clean() {

    auto sim_alg = Mantid::API::AlgorithmManager::Instance().create(
        "CreateSimulationWorkspace");
    sim_alg->initialize();
    sim_alg->setPropertyValue("Instrument", "MAR");
    sim_alg->setPropertyValue("BinParams", "-3,1,3");
    sim_alg->setPropertyValue("UnitX", "DeltaE");
    sim_alg->setPropertyValue("OutputWorkspace", "data_source_1");
    sim_alg->execute();

    sim_alg->setPropertyValue("OutputWorkspace", "data_source_2");
    sim_alg->execute();

    auto log_alg =
        Mantid::API::AlgorithmManager::Instance().create("AddSampleLog");
    log_alg->initialize();
    log_alg->setProperty("Workspace", "data_source_1");
    log_alg->setPropertyValue("LogName", "Ei");
    log_alg->setPropertyValue("LogText", "3.0");
    log_alg->setPropertyValue("LogType", "Number");
    log_alg->execute();

    log_alg->setProperty("Workspace", "data_source_2");
    log_alg->execute();

    auto create_alg =
        Mantid::API::AlgorithmManager::Instance().create("CreateMD");
    create_alg->setRethrows(true);
    create_alg->initialize();
    create_alg->setPropertyValue("OutputWorkspace", "md_sample_workspace");
    create_alg->setPropertyValue("DataSources", "data_source_1");
    create_alg->setPropertyValue("Alatt", "1,1,1");
    create_alg->setPropertyValue("Angdeg", "90,90,90");
    create_alg->setPropertyValue("Efix", "12.0");
    create_alg->setPropertyValue("u", "1,0,0");
    create_alg->setPropertyValue("v", "0,1,0");
    create_alg->execute();
    IMDEventWorkspace_sptr in_ws =
        boost::dynamic_pointer_cast<IMDEventWorkspace>(
            AnalysisDataService::Instance().retrieve("md_sample_workspace"));

    AccumulateMD acc_alg;
    acc_alg.initialize();
    acc_alg.setPropertyValue("InputWorkspace", "md_sample_workspace");
    acc_alg.setPropertyValue("OutputWorkspace", "accumulated_workspace");
    acc_alg.setPropertyValue("DataSources", "data_source_2");
    acc_alg.setPropertyValue("Alatt", "1.4165,1.4165,1.4165");
    acc_alg.setPropertyValue("Angdeg", "90,90,90");
    acc_alg.setPropertyValue("u", "1,0,0");
    acc_alg.setPropertyValue("v", "0,1,0");
    acc_alg.setProperty("Clean", true);
    TS_ASSERT_THROWS_NOTHING(acc_alg.execute());
    IMDEventWorkspace_sptr out_ws =
        boost::dynamic_pointer_cast<IMDEventWorkspace>(
            AnalysisDataService::Instance().retrieve("accumulated_workspace"));

    // Should only have the same number of events as data_source_2 this time
    // as create from clean so lost data in data_source_1
    TS_ASSERT_EQUALS(in_ws->getNEvents(), out_ws->getNEvents());
  }
};

#endif /* MANTID_MDALGORITHMS_ACCUMULATEMDTEST_H_ */
