#ifndef LOADNEXUSLOGSTEST_H_
#define LOADNEXUSLOGSTEST_H_

#include "MantidDataHandling/LoadNexusLogs.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/PhysicalConstants.h"

#include <H5Cpp.h>
#include <hdf5.h>

using namespace Mantid;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;

#include <cxxtest/TestSuite.h>
#include "MantidAPI/WorkspaceGroup.h"

//----------------------------------------------------------------------------
// This is a region to prototype the HDF5 file readers

/**
 * @brief file_info
 * @param loc_id
 * @param name
 * @param linfo
 * @param opdata
 * @return
 */
herr_t getFileInfo(hid_t loc_id, const char *name, const H5L_info_t *linfo,
                   void *opdata) {

  // find out if this is a group
  H5O_info_t object_info;
  H5Oget_info_by_name(loc_id, name, &object_info, H5P_DEFAULT);

  std::string entry_type("");
  if (object_info.type == H5O_TYPE_GROUP)
    entry_type = "NXGroup";
  else if (object_info.type == H5O_TYPE_DATASET)
    entry_type = "NXData";
  else if (object_info.type == H5O_TYPE_NAMED_DATATYPE)
    entry_type = "NXNameDataType";
  else if (object_info.type == H5O_TYPE_UNKNOWN)
    entry_type = "UNKNOWN";
  else
    entry_type = "No Definition";

  std::string nameStr(name);

  std::cout << "name: " << nameStr << ", "
            << "type: " << entry_type << "\n";

  if ((object_info).type != H5O_TYPE_GROUP) {
    return 0;
  }

  if (nameStr.find("_events") != std::string::npos) {
    // std::cout << nameStr << std::endl;

    hid_t group = H5Gopen2(loc_id, name, H5P_DEFAULT);
    //   read_data<uint32_t>(group, "event_id", H5T_NATIVE_UINT);
    //  read_data<uint64_t>(group, "event_index", H5T_NATIVE_ULONG);
    //  read_data<float>(group, "event_time_offset", H5T_NATIVE_FLOAT);
    //  read_data<double>(group, "event_time_zero", H5T_NATIVE_DOUBLE);

    H5Gclose(group);
  }

  // open the group

  return 0;
}

/*
 * Operator function.
 */
herr_t file_info(hid_t loc_id, const char *name, const H5L_info_t *linfo,
                 void *opdata) {
  hid_t group;
  /*
   * Open the group using its name.
   */
  group = H5Gopen2(loc_id, name, H5P_DEFAULT);
  /*
   * Display group name.
   */
  std::cout << "Name : " << name << std::endl;

  int *opdata_int = reinterpret_cast<int *>(opdata);
  (*opdata_int) = (*opdata_int) + 1;

  H5Gclose(group);
  return 0;
}

std::map<std::string, std::string> getEntries(hid_t entry_id) {
  std::map<std::string, std::string> entry_map;
  return entry_map;
}

size_t numEvents() { return 0; }

//----------------------------------------------------------------------------

class LoadNexusLogsTest : public CxxTest::TestSuite {
public:
  void Ptest_File_With_DASLogs() {
    Mantid::API::FrameworkManager::Instance();
    LoadNexusLogs ld;
    std::string outws_name = "REF_L_instrument";
    ld.initialize();
    ld.setPropertyValue("Filename", "REF_L_32035.nxs");
    MatrixWorkspace_sptr ws = createTestWorkspace();
    // Put it in the object.
    ld.setProperty("Workspace", ws);
    ld.execute();
    TS_ASSERT(ld.isExecuted());

    double val;
    Run &run = ws->mutableRun();
    // Do we have all we expect
    const std::vector<Property *> &logs = run.getLogData();
    TS_ASSERT_EQUALS(logs.size(), 74);
    Property *prop;
    TimeSeriesProperty<double> *dProp;

    prop = run.getLogData("Speed3");
    TS_ASSERT(prop);
    // TS_ASSERT_EQUALS( prop->value(), "60");
    TS_ASSERT_EQUALS(prop->units(), "Hz");

    prop = run.getLogData("PhaseRequest1");
    dProp = dynamic_cast<TimeSeriesProperty<double> *>(prop);
    TS_ASSERT(dProp);
    val = dProp->nthValue(0);
    TS_ASSERT_DELTA(val, 13712.77, 1e-2);
    TS_ASSERT_EQUALS(prop->units(), "microsecond");

    TimeSeriesProperty<double> *tsp;

    prop = run.getLogData("Phase1");
    tsp = dynamic_cast<TimeSeriesProperty<double> *>(prop);
    TS_ASSERT(tsp);
    TS_ASSERT_EQUALS(tsp->units(), "microsecond");
    TS_ASSERT_DELTA(tsp->nthValue(1), 13715.55, 2);

    // The time diff between the 0th and 1st entry is 0.328 seconds
    TS_ASSERT_DELTA(
        Kernel::DateAndTime::secondsFromDuration(tsp->nthInterval(0).length()),
        0.328, 0.01);

    // Now the stats
  }

  void Ptest_File_With_Runlog_And_Selog() {
    LoadNexusLogs loader;
    loader.initialize();
    MatrixWorkspace_sptr testWS = createTestWorkspace();
    TS_ASSERT_THROWS_NOTHING(loader.setProperty("Workspace", testWS));
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("Filename", "LOQ49886.nxs"));
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    const API::Run &run = testWS->run();
    const std::vector<Property *> &logs = run.getLogData();
    TS_ASSERT_EQUALS(logs.size(),
                     34); // 33 logs in file + 1 synthetic nperiods log

    TimeSeriesProperty<std::string> *slog =
        dynamic_cast<TimeSeriesProperty<std::string> *>(
            run.getLogData("icp_event"));
    TS_ASSERT(slog);
    std::string str = slog->value();
    TS_ASSERT_EQUALS(str.size(), 1023);
    TS_ASSERT_EQUALS(str.substr(0, 37),
                     "2009-Apr-28 09:20:29  CHANGE_PERIOD 1");

    slog = dynamic_cast<TimeSeriesProperty<std::string> *>(
        run.getLogData("icp_debug"));
    TS_ASSERT(slog);
    TS_ASSERT_EQUALS(slog->size(), 50);

    TimeSeriesProperty<int> *ilog =
        dynamic_cast<TimeSeriesProperty<int> *>(run.getLogData("total_counts"));
    TS_ASSERT(ilog);
    TS_ASSERT_EQUALS(ilog->size(), 172);

    ilog = dynamic_cast<TimeSeriesProperty<int> *>(run.getLogData("period"));
    TS_ASSERT(ilog);
    TS_ASSERT_EQUALS(ilog->size(), 172);

    TimeSeriesProperty<double> *dlog =
        dynamic_cast<TimeSeriesProperty<double> *>(
            run.getLogData("proton_charge"));
    TS_ASSERT(dlog);
    TS_ASSERT_EQUALS(dlog->size(), 172);
  }

  void Ptest_extract_nperiod_log_from_event_nexus() {

    auto testWS = createTestWorkspace();
    auto run = testWS->run();
    TSM_ASSERT("Should not have nperiods until we run LoadNexusLogs",
               !run.hasProperty("nperiods"));
    LoadNexusLogs loader;

    loader.setChild(true);
    loader.initialize();
    loader.setProperty("Workspace", testWS);
    loader.setPropertyValue("Filename", "LARMOR00003368.nxs");
    loader.execute();
    run = testWS->run();

    const bool hasNPeriods = run.hasProperty("nperiods");
    TSM_ASSERT("Should have nperiods now we have run LoadNexusLogs",
               hasNPeriods);
    if (hasNPeriods) {
      const int nPeriods = run.getPropertyValueAsType<int>("nperiods");
      TSM_ASSERT_EQUALS("Wrong number of periods extracted", nPeriods, 4);
    }
  }

  void Ptest_extract_periods_log_from_event_nexus() {

    auto testWS = createTestWorkspace();
    auto run = testWS->run();
    TSM_ASSERT("Should not have nperiods until we run LoadNexusLogs",
               !run.hasProperty("nperiods"));
    LoadNexusLogs loader;

    loader.setChild(true);
    loader.initialize();
    loader.setProperty("Workspace", testWS);
    loader.setPropertyValue("Filename", "LARMOR00003368.nxs");
    loader.execute();
    run = testWS->run();

    const bool hasPeriods = run.hasProperty("period_log");
    TSM_ASSERT("Should have period_log now we have run LoadNexusLogs",
               hasPeriods);

    auto *temp = run.getProperty("period_log");
    auto *periodLog = dynamic_cast<TimeSeriesProperty<int> *>(temp);
    TSM_ASSERT("Period log should be an int time series property", periodLog);

    std::vector<int> periodValues = periodLog->valuesAsVector();
    std::unordered_set<int> uniquePeriods(periodValues.begin(),
                                          periodValues.end());
    TSM_ASSERT_EQUALS("Should have 4 periods in total", 4,
                      uniquePeriods.size());
  }

  void Ptest_log_non_default_entry() {
    auto testWS = createTestWorkspace();
    LoadNexusLogs loader;

    // default entry Off-Off
    loader.setChild(true);
    loader.initialize();
    loader.setProperty("Workspace", testWS);
    loader.setPropertyValue("Filename", "REF_M_9709_event.nxs");
    loader.execute();
    auto run = testWS->run();
    TimeSeriesProperty<double> *pclog =
        dynamic_cast<TimeSeriesProperty<double> *>(
            run.getLogData("proton_charge"));
    TS_ASSERT(pclog);
    TS_ASSERT_EQUALS(pclog->size(), 23806);
    TS_ASSERT(pclog->getStatistics().duration > 4e9);

    // 3rd entry On-Off
    testWS = createTestWorkspace();
    loader.setChild(true);
    loader.initialize();
    loader.setProperty("Workspace", testWS);
    loader.setPropertyValue("Filename", "REF_M_9709_event.nxs");
    loader.setProperty("NXentryName", "entry-On_Off");
    loader.execute();
    run = testWS->run();
    pclog = dynamic_cast<TimeSeriesProperty<double> *>(
        run.getLogData("proton_charge"));
    TS_ASSERT(pclog);
    TS_ASSERT_EQUALS(pclog->size(), 24150);
    TS_ASSERT(pclog->getStatistics().duration < 3e9);
  }

  void test_h5_cpp() {
    std::string filename("/home/wzz/Projects/workspaces/Mantid/"
                         "15840_LoadEventNexus/PG3_29571.nxs.h5");

    H5::H5File mfile(filename, H5F_ACC_RDONLY);
    std::cout << "Is HDF5?: " << mfile.isHdf5(filename.c_str()) << "\n";

    std::vector<std::string> child_names;
    size_t num_children(0);
    herr_t idx = H5Literate(mfile.getId(), H5_INDEX_NAME, H5_ITER_INC, NULL,
                            file_info, &num_children);
    std::cout << "under file number of children = " << num_children << "\n";

    std::cout << "number object under root = " << mfile.getNumObjs() << "\n";
    std::cout << "first object name = " << mfile.getObjnameByIdx(0) << "\n";

    std::cout << mfile.getNumAttrs() << ", " << mfile.getNumObjs() << "\n";

    hid_t obj_counts = mfile.getObjCount(H5F_OBJ_GROUP);
    std::cout << obj_counts << "\n";
    obj_counts = mfile.getObjCount(H5F_OBJ_ALL);
    std::cout << obj_counts << "\n";
    std::cout << mfile.getObjCount(H5F_OBJ_FILE) << "\n";
    std::cout << mfile.getObjCount(H5F_OBJ_DATASET) << "\n";
    std::cout << mfile.getObjCount(H5F_OBJ_DATATYPE) << "\n";
    std::cout << mfile.getObjCount(H5F_OBJ_ATTR) << "\n";

    hid_t oid_list[1000];
    mfile.getObjIDs(H5F_OBJ_ALL, 1000, &oid_list[0]);
    std::cout << "obj ID: " << oid_list[0] << "\n";
    /* this cause fault!
    std::string obj_name = mfile.getObjnameByIdx(oid_list[0]);
    std::cout << "obj name: " << obj_name << "\n";
    */

    H5::Group group_entry = mfile.openGroup("entry");
    hid_t entry_id = group_entry.getId();
    std::cout << "entry ID: " << entry_id << "\n";
    auto num_entries = group_entry.getNumObjs();
    for (size_t i = 0; i < num_entries; ++i)
      std::cout << group_entry.getObjnameByIdx(i) << "\n";

    mfile.close();

    TS_ASSERT_EQUALS(1, 3);
  }

  void Ptest_hdf5_lib() {
    std::string filename("/home/wzz/Projects/workspaces/Mantid/"
                         "15840_LoadEventNexus/PG3_29571.nxs.h5");

    /* Open an existing file. */
    hid_t file_id = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);

    // open a group
    hid_t entry_id = H5Gopen(file_id, "/entry", H5P_DEFAULT); // SNS

    // number of objects
    hsize_t numobjs;
    H5Gget_num_objs(entry_id, &numobjs);
    TS_ASSERT_EQUALS(numobjs, 100);

    // info
    H5G_info_t info;
    H5Gget_info(entry_id, &info);
    std::cout << info.nlinks << ", " << info.max_corder << "\n";

    hsize_t index;
    //  H5Literate(entry_id, H5_INDEX_NAME, H5_ITER_INC, &index, file_info,
    //  NULL);
    H5Literate(entry_id, H5_INDEX_NAME, H5_ITER_INC, NULL, getFileInfo, NULL);

    // close a group
    H5Gclose(entry_id);

    H5close();

    /*
    // hid_t entry_id1 = H5Gopen(file_id, "/raw_data_1", H5P_DEFAULT); // ISIS
    hid_t entry_id1 = H5Gopen(file_id, "/mantid_workspace_1", H5P_DEFAULT); //
    ISIS

    // put together a list of NXevent_data
    // H5Literate(entry_id1, H5_INDEX_NAME, H5_ITER_INC, NULL, file_info, NULL);

    hid_t entry_logs = H5Gopen(file_id, "/mantid_workspace_1/logs",
    H5P_DEFAULT);

    // TRYING: reading log files
    std::string nxsname = read_string_data(entry_id1, "logs/Filename/value");
    std::cout << "Nexus :" << nxsname << "\n\n";

    std::string currperiod = read_string_data(entry_logs,
    "current_period/value");
    std::cout << "Period: " << currperiod << "\n\n";
    // std::string nxsname = read_string_data(file_id,
    "/mantid_workspace_1/workspace/axis1"); ///Filename");
    // read_data<std::string>(entry_logs, "Filename", H5T_NATIVE_UINT);
    //
    //

    // READ DATA:
    hid_t group = H5Gopen2(entry_id1, "workspace", H5P_DEFAULT);
    std::vector<uint32_t> axis1 =  read_data<uint32_t>(group, "axis1",
    H5T_NATIVE_UINT);
    std::cout << axis1.size() << "\n";


    H5Gclose(entry_id1);

    herr_t status = H5Fclose(file_id);
    */

    TS_ASSERT_EQUALS(1, 2);
  }

private:
  API::MatrixWorkspace_sptr createTestWorkspace() {
    return WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);
  }
};

#endif /* LOADNEXUSLOGS_H_*/
