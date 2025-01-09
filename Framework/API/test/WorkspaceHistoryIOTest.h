// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/AlgorithmHistory.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidFrameworkTestHelpers/NexusTestHelper.h"
#include "MantidKernel/Property.h"
#include "Poco/File.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using Mantid::Types::Core::DateAndTime;

class WorkspaceHistoryIOTest : public CxxTest::TestSuite {
private:
  void failiure_testfile_setup(int testno, NexusTestHelper &testfile) {
    // dummy file info
    std::stringstream output;

    // Environment history
    EnvironmentHistory envHist;
    output << envHist;
    char buffer[25];
    time_t now;
    time(&now);
    strftime(buffer, 25, "%Y-%b-%d %H:%M:%S", localtime(&now));

    // common start
    testfile.createFile("LoadNexusTest.nxs");

    testfile.file->makeGroup("process", "NXprocess", true);

    testfile.file->makeGroup("MantidEnvironment", "NXnote", true);
    testfile.file->writeData("author", "mantid");
    testfile.file->openData("author");
    testfile.file->putAttr("date", std::string(buffer));
    testfile.file->closeData();
    testfile.file->writeData("description", "Mantid Environment data");
    testfile.file->writeData("data", output.str());
    testfile.file->closeGroup();
    // commonend

    if (testno != 6) {
      // first algorithm
      testfile.file->makeGroup("MantidAlgorithm_0", "NXnote", true);
      testfile.file->writeData("author", std::string("mantid"));
      testfile.file->writeData("description", std::string("Mantid Algorithm data"));
      testfile.file->writeData("data", "Algorithm: LoadRaw v1\nExecution Date: 2009-Oct-09 "
                                       "16:56:54\nExecution Duration: 2.3 seconds\nParameters:\n  "
                                       "Name: Filename, Value: "
                                       "/home/dmn58364/Mantid/trunk/Test/Data/GEM38370.raw, "
                                       "Default?: No, Direction: Input\n  Name: OutputWorkspace, "
                                       "Value: GEM38370, Default?: No, Direction: Output\n  Name: "
                                       "SpectrumMin, Value: 1, Default?: Yes, Direction: Input\n  "
                                       "Name: SpectrumMax, Value: 2147483632, Default?: Yes, "
                                       "Direction: Input\n  Name: SpectrumList, Value: , Default?: "
                                       "Yes, Direction: Input\n  Name: Cache, Value: If Slow, "
                                       "Default?: Yes, Direction: Input\n  Name: LoadLogFiles, "
                                       "Value: 1, Default?: Yes, Direction: Input");
      testfile.file->closeGroup();
      // second algorithm is different each test
      switch (testno) {
      case 2: {
        testfile.file->makeGroup("MantidAlgorithm_1", "NXnote", true);
        // missing author
        testfile.file->writeData("description", std::string("Mantid Algorithm data"));
        testfile.file->writeData("data", "Algorithm: LoadRaw v2\nExecution Date: 2009-Oct-09 "
                                         "16:56:54\nExecution Duration: 2.3 seconds\nParameters:\n  "
                                         "Name: Filename, Value: "
                                         "/home/dmn58364/Mantid/trunk/Test/Data/GEM38370.raw, "
                                         "Default?: No, Direction: Input\n  Name: OutputWorkspace, "
                                         "Value: GEM38370, Default?: No, Direction: Output\n  Name: "
                                         "SpectrumMin, Value: 1, Default?: Yes, Direction: Input\n  "
                                         "Name: SpectrumMax, Value: 2147483632, Default?: Yes, "
                                         "Direction: Input\n  Name: SpectrumList, Value: , "
                                         "Default?: Yes, Direction: Input\n  Name: Cache, Value: If "
                                         "Slow, Default?: Yes, Direction: Input\n  Name: "
                                         "LoadLogFiles, Value: 1, Default?: Yes, Direction: Input");
        testfile.file->closeGroup();
        break;
      }
      case 3: {
        testfile.file->makeGroup("MantidAlgorithm_1", "NXnote", true);
        testfile.file->writeData("author", std::string("mantid"));
        // missing description
        testfile.file->writeData("data", "Algorithm: LoadRaw v2\nExecution Date: 2009-Oct-09 "
                                         "16:56:54\nExecution Duration: 2.3 seconds\nParameters:\n  "
                                         "Name: Filename, Value: "
                                         "/home/dmn58364/Mantid/trunk/Test/Data/GEM38370.raw, "
                                         "Default?: No, Direction: Input\n  Name: OutputWorkspace, "
                                         "Value: GEM38370, Default?: No, Direction: Output\n  Name: "
                                         "SpectrumMin, Value: 1, Default?: Yes, Direction: Input\n  "
                                         "Name: SpectrumMax, Value: 2147483632, Default?: Yes, "
                                         "Direction: Input\n  Name: SpectrumList, Value: , "
                                         "Default?: Yes, Direction: Input\n  Name: Cache, Value: If "
                                         "Slow, Default?: Yes, Direction: Input\n  Name: "
                                         "LoadLogFiles, Value: 1, Default?: Yes, Direction: Input");
        testfile.file->closeGroup();
        break;
      }
      case 4: {
        testfile.file->makeGroup("MantidAlgorithm_1", "NXnote", true);
        testfile.file->writeData("author", std::string("mantid"));
        testfile.file->writeData("description", std::string("Mantid Algorithm data"));
        // missing data
        testfile.file->closeGroup();
        break;
      }
      case 5: {
        testfile.file->makeGroup("MantidAlgorithm_1", "NXnote", true);
        testfile.file->writeData("author", std::string("mantid"));
        testfile.file->writeData("description", std::string("Mantid Algorithm data"));
        testfile.file->writeData("data", "some data");
        testfile.file->closeGroup();
        break;
      }
      case 7: {
        testfile.file->makeGroup("MantidAlgorithm_1", "NXnote", true);
        testfile.file->writeData("author", std::string("mantid"));
        testfile.file->writeData("description", std::string("Mantid Algorithm data"));
        testfile.file->writeData("data", "some data\nsome data\nsome data\nsome data\nsome data");
        testfile.file->closeGroup();
        break;
      }
      default: {
        testfile.file->makeGroup("MantidAlgorithm_1", "NXnote", true);
        testfile.file->writeData("author", std::string("mantid"));
        testfile.file->writeData("description", std::string("Mantid Algorithm data"));
        testfile.file->writeData("data", "Algorithm: LoadRaw v2\nExecution Date: 2009-Oct-09 "
                                         "16:56:54\nExecution Duration: 2.3 seconds\nParameters:\n  "
                                         "Name: Filename, Value: "
                                         "/home/dmn58364/Mantid/trunk/Test/Data/GEM38370.raw, "
                                         "Default?: No, Direction: Input\n  Name: OutputWorkspace, "
                                         "Value: GEM38370, Default?: No, Direction: Output\n  Name: "
                                         "SpectrumMin, Value: 1, Default?: Yes, Direction: Input\n  "
                                         "Name: SpectrumMax, Value: 2147483632, Default?: Yes, "
                                         "Direction: Input\n  Name: SpectrumList, Value: , "
                                         "Default?: Yes, Direction: Input\n  Name: Cache, Value: If "
                                         "Slow, Default?: Yes, Direction: Input\n  Name: "
                                         "LoadLogFiles, Value: 1, Default?: Yes, Direction: Input");
        testfile.file->closeGroup();
        break;
      }
      }
      testfile.file->makeGroup("MantidAlgorithm_2", "NXnote", true);
      testfile.file->writeData("author", std::string("mantid"));
      testfile.file->writeData("description", std::string("Mantid Algorithm data"));
      testfile.file->writeData("data", "Algorithm: LoadRaw v3\nExecution Date: 2009-Oct-09 "
                                       "16:56:54\nExecution Duration: 2.3 seconds\nParameters:\n  "
                                       "Name: Filename, Value: "
                                       "/home/dmn58364/Mantid/trunk/Test/Data/GEM38370.raw, "
                                       "Default?: No, Direction: Input\n  Name: OutputWorkspace, "
                                       "Value: GEM38370, Default?: No, Direction: Output\n  Name: "
                                       "SpectrumMin, Value: 1, Default?: Yes, Direction: Input\n  "
                                       "Name: SpectrumMax, Value: 2147483632, Default?: Yes, "
                                       "Direction: Input\n  Name: SpectrumList, Value: , Default?: "
                                       "Yes, Direction: Input\n  Name: Cache, Value: If Slow, "
                                       "Default?: Yes, Direction: Input\n  Name: LoadLogFiles, "
                                       "Value: 1, Default?: Yes, Direction: Input");
      testfile.file->closeGroup();
    } else {
      testfile.file->makeGroup("MantidAlgorithm_0", "NXnote", true);
      testfile.file->writeData("author", std::string("mantid"));
      testfile.file->writeData("description", std::string("Mantid Algorithm data"));
      testfile.file->writeData("data", "some data");
      testfile.file->closeGroup();

      testfile.file->makeGroup("MantidAlgorithm_1", "NXnote", true);
      testfile.file->writeData("author", std::string("mantid"));
      testfile.file->writeData("description", std::string("Mantid Algorithm data"));
      testfile.file->writeData("data", "some data");
      testfile.file->closeGroup();

      testfile.file->makeGroup("MantidAlgorithm_2", "NXnote", true);
      testfile.file->writeData("author", std::string("mantid"));
      testfile.file->writeData("description", std::string("Mantid Algorithm data"));
      testfile.file->writeData("data", "some data");
      testfile.file->closeGroup();
    }
    // common end
    testfile.file->closeGroup();
  }

public:
  void test_SaveNexus() {
    WorkspaceHistory testHistory;
    for (int i = 1; i < 5; i++) {
      AlgorithmHistory algHist("History" + boost::lexical_cast<std::string>(i), 1,
                               boost::uuids::to_string(boost::uuids::random_generator()()), DateAndTime::defaultTime(),
                               -1.0, i);
      testHistory.addHistory(std::make_shared<AlgorithmHistory>(algHist));
    }

    auto savehandle = std::make_shared<::NeXus::File>("WorkspaceHistoryTest_test_SaveNexus.nxs", NXACC_CREATE5);
    TS_ASSERT_THROWS_NOTHING(testHistory.saveNexus(savehandle.get()));
    savehandle->close();

    auto loadhandle = std::make_shared<::NeXus::File>("WorkspaceHistoryTest_test_SaveNexus.nxs");
    std::string rootstring = "/process/";
    for (int i = 1; i < 5; i++) {
      TS_ASSERT_THROWS_NOTHING(
          loadhandle->openPath(rootstring + "MantidAlgorithm_" + boost::lexical_cast<std::string>(i)));
    }
    TS_ASSERT_THROWS_NOTHING(loadhandle->openPath(rootstring + "MantidEnvironment"));
    TS_ASSERT_THROWS_NOTHING(loadhandle->openPath(rootstring + "MantidAlgorithm_4/author"));
    TS_ASSERT_THROWS_NOTHING(loadhandle->openPath(rootstring + "MantidAlgorithm_4/data"));
    TS_ASSERT_THROWS_NOTHING(loadhandle->openPath(rootstring + "MantidAlgorithm_4/description"));
    TS_ASSERT_THROWS_ANYTHING(loadhandle->openPath(rootstring + "MantidAlgorithm_5"));

    loadhandle->close();
    Poco::File("WorkspaceHistoryTest_test_SaveNexus.nxs").remove();
  }

  void test_SaveNexus_NestedHistory() {
    WorkspaceHistory testHistory;
    AlgorithmHistory algHist("ParentHistory", 1, boost::uuids::to_string(boost::uuids::random_generator()()),
                             DateAndTime::defaultTime(), -1.0, 0);
    AlgorithmHistory childHist("ChildHistory", 1, boost::uuids::to_string(boost::uuids::random_generator()()),
                               DateAndTime::defaultTime(), -1.0, 1);

    algHist.addChildHistory(std::make_shared<AlgorithmHistory>(childHist));
    testHistory.addHistory(std::make_shared<AlgorithmHistory>(algHist));

    auto savehandle = std::make_shared<::NeXus::File>("WorkspaceHistoryTest_test_SaveNexus.nxs", NXACC_CREATE5);
    TS_ASSERT_THROWS_NOTHING(testHistory.saveNexus(savehandle.get()));
    savehandle->close();

    auto loadhandle = std::make_shared<::NeXus::File>("WorkspaceHistoryTest_test_SaveNexus.nxs");
    std::string rootstring = "/process/";
    TS_ASSERT_THROWS_NOTHING(loadhandle->openPath(rootstring + "MantidAlgorithm_1/"));
    TS_ASSERT_THROWS_NOTHING(loadhandle->openPath(rootstring + "MantidAlgorithm_1/author"));
    TS_ASSERT_THROWS_NOTHING(loadhandle->openPath(rootstring + "MantidAlgorithm_1/data"));
    TS_ASSERT_THROWS_NOTHING(loadhandle->openPath(rootstring + "MantidAlgorithm_1/description"));

    TS_ASSERT_THROWS_NOTHING(loadhandle->openPath(rootstring + "MantidAlgorithm_1/MantidAlgorithm_2"));
    TS_ASSERT_THROWS_NOTHING(loadhandle->openPath(rootstring + "MantidAlgorithm_1/MantidAlgorithm_2/author"));
    TS_ASSERT_THROWS_NOTHING(loadhandle->openPath(rootstring + "MantidAlgorithm_1/MantidAlgorithm_2/data"));
    TS_ASSERT_THROWS_NOTHING(loadhandle->openPath(rootstring + "MantidAlgorithm_1/MantidAlgorithm_2/description"));

    loadhandle->close();
    Poco::File("WorkspaceHistoryTest_test_SaveNexus.nxs").remove();
  }

  void test_SaveNexus_Empty() {
    WorkspaceHistory testHistory;

    auto savehandle = std::make_shared<::NeXus::File>("WorkspaceHistoryTest_test_SaveNexus.nxs", NXACC_CREATE5);
    TS_ASSERT_THROWS_NOTHING(testHistory.saveNexus(savehandle.get()));
    savehandle->close();

    auto loadhandle = std::make_shared<::NeXus::File>("WorkspaceHistoryTest_test_SaveNexus.nxs");
    std::string rootstring = "/process/";
    TS_ASSERT_THROWS_NOTHING(loadhandle->openPath(rootstring));
    TS_ASSERT_THROWS_NOTHING(loadhandle->openPath(rootstring + "MantidEnvironment"));
    TS_ASSERT_THROWS_ANYTHING(loadhandle->openPath(rootstring + "MantidAlgorithm_1"));

    loadhandle->close();
    Poco::File("WorkspaceHistoryTest_test_SaveNexus.nxs").remove();
  }

  void test_LoadNexus() {
    std::string filename = FileFinder::Instance().getFullPath("GEM38370_Focussed_Legacy.nxs");
    auto loadhandle = std::make_shared<::NeXus::File>(filename);
    loadhandle->openPath("/mantid_workspace_1");

    WorkspaceHistory emptyHistory;
    TS_ASSERT_THROWS_NOTHING(emptyHistory.loadNexus(loadhandle.get()));

    const auto &histories = emptyHistory.getAlgorithmHistories();
    TS_ASSERT_EQUALS(3, histories.size());

    const auto history = emptyHistory.getAlgorithmHistory(0);

    TS_ASSERT_EQUALS("LoadRaw", history->name());
    TS_ASSERT_EQUALS(3, history->version());
    TS_ASSERT_EQUALS(DateAndTime("2009-10-09T16:56:54"), history->executionDate());
    TS_ASSERT_EQUALS(2.3, history->executionDuration());
    loadhandle->close();
  }

  void test_LoadNexus_NestedHistory() {
    std::string filename = FileFinder::Instance().getFullPath("HistoryTest_CreateTransmissionAuto.nxs");
    auto loadhandle = std::make_shared<::NeXus::File>(filename);
    loadhandle->openPath("/mantid_workspace_1");

    WorkspaceHistory wsHistory;
    TS_ASSERT_THROWS_NOTHING(wsHistory.loadNexus(loadhandle.get()));

    const auto &histories = wsHistory.getAlgorithmHistories();
    TS_ASSERT_EQUALS(3, histories.size());

    auto history = wsHistory.getAlgorithmHistory(1);

    TS_ASSERT_EQUALS("CreateTransmissionWorkspaceAuto", history->name());
    TS_ASSERT_EQUALS(1, history->version());

    const auto childHistory = history->getChildAlgorithmHistory(0);

    TS_ASSERT_EQUALS("CreateTransmissionWorkspace", childHistory->name());
    TS_ASSERT_EQUALS(1, childHistory->version());

    history = wsHistory.getAlgorithmHistory(2);

    TS_ASSERT_EQUALS("SaveNexusProcessed", history->name());
    TS_ASSERT_EQUALS(1, history->version());

    loadhandle->close();
  }

  void test_LoadNexus_Blank_File() {
    std::string rootstring = "/process/";
    // first file - clean - contains nothing
    NexusTestHelper testfile(true);
    testfile.createFile("LoadNexusTest.nxs");

    WorkspaceHistory history;
    // will throw nothing as it will return with only a warning, no exception
    TS_ASSERT_THROWS_NOTHING(history.loadNexus(testfile.file.get()));
    const auto &histories = history.getAlgorithmHistories();

    TS_ASSERT_EQUALS(0, histories.size());
    TS_ASSERT_THROWS_ANYTHING(testfile.file->openPath(rootstring));
    TS_ASSERT_THROWS_ANYTHING(testfile.file->openPath(rootstring + "MantidEnvironment"));
    TS_ASSERT_THROWS_ANYTHING(testfile.file->openPath(rootstring + "MantidAlgorithm_1"));
  }

  void test_LoadNexus_Missing_Author() {
    // second file - 3 algorithms one missing an author
    NexusTestHelper testfile(true);
    failiure_testfile_setup(2, testfile);
    WorkspaceHistory history;
    TS_ASSERT_THROWS_NOTHING(history.loadNexus(testfile.file.get()));
    const auto &histories = history.getAlgorithmHistories();
    // three will still exist as it doesn't really care about the author
    TS_ASSERT_EQUALS(3, histories.size());
  }

  void test_LoadNexus_Missing_Description() {
    // third file - 3 algorithms one missing a description
    NexusTestHelper testfile(true);
    failiure_testfile_setup(3, testfile);
    WorkspaceHistory history;
    TS_ASSERT_THROWS_NOTHING(history.loadNexus(testfile.file.get()));
    const auto &histories = history.getAlgorithmHistories();

    // three will still exist as it doesn't really care about the author
    TS_ASSERT_EQUALS(3, histories.size());
  }

  void test_LoadNexus_Missing_Data() {
    // fourth file - 3 algorithms one missing data
    NexusTestHelper testfile(true);
    failiure_testfile_setup(4, testfile);
    WorkspaceHistory history;
    // this WILL throw as it looks for a data field using
    // ::NeXus::File::readData() and it won't be found
    TS_ASSERT_THROWS_ANYTHING(history.loadNexus(testfile.file.get()));
    const auto &histories = history.getAlgorithmHistories();
    // only one will exist as it will throw on the second (wihtout the data) and
    // skip the rest
    TS_ASSERT_EQUALS(1, histories.size());
  }

  void test_LoadNexus_Short_Data() {
    // fifth file - 3 algorithms one wiht only one line of data
    NexusTestHelper testfile(true);
    failiure_testfile_setup(5, testfile);
    WorkspaceHistory history;
    // won't throw as it'll simply ignore the bad data
    TS_ASSERT_THROWS_NOTHING(history.loadNexus(testfile.file.get()));
    const auto &histories = history.getAlgorithmHistories();

    // only two will exist as it will ignore the second (with only the single
    // line) and continue as normal
    TS_ASSERT_EQUALS(2, histories.size());
  }

  void test_LoadNexus_All_Short_Data() {
    // sixth file - 3 algorithms all with only one line of data
    NexusTestHelper testfile(true);
    failiure_testfile_setup(6, testfile);
    WorkspaceHistory history;
    // nothign will throw but nothing will be loaded either as the data is
    // invalid
    TS_ASSERT_THROWS_NOTHING(history.loadNexus(testfile.file.get()));
    const auto &histories = history.getAlgorithmHistories();
    // size should be zero as nothing went in the file
    TS_ASSERT_EQUALS(0, histories.size());
  }

  void test_LoadNexus_Bad_Formatting() {
    // seventh file - 3 algorithms, one with bad formatting
    NexusTestHelper testfile(true);
    failiure_testfile_setup(7, testfile);
    WorkspaceHistory history;
    // this will throw on the second due to the unformatted data - the function
    // expects well formatted data
    TS_ASSERT_THROWS_ANYTHING(history.loadNexus(testfile.file.get()));
    const auto &histories = history.getAlgorithmHistories();
    // only one will exist as it will throw on the second (with the bad data)
    // and skip the rest
    TS_ASSERT_EQUALS(1, histories.size());
  }
};
