#ifndef WORKSPACEHISTORYTEST_H_
#define WORKSPACEHISTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAPI/AlgorithmHistory.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/FileFinder.h"
#include "MantidKernel/Property.h"
#include "MantidTestHelpers/NexusTestHelper.h"
#include "Poco/File.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

class WorkspaceHistoryTest : public CxxTest::TestSuite
{
private:

  /// Use a fake algorithm object instead of a dependency on a real one.
  class SimpleSum : public Algorithm
  {
  public:
    SimpleSum() : Algorithm() {}
    virtual ~SimpleSum() {}
    const std::string name() const { return "SimpleSum";} 
    int version() const  { return 1;}                        
    const std::string category() const { return "Dummy";}        

    void init()
    { 
      declareProperty("Input1",2);
      declareProperty("Input2",1);   
      declareProperty("Output1",-1,Direction::Output);
    }
    void exec() 
    {
      const int lhs = getProperty("Input1");
      const int rhs = getProperty("Input2");
      const int sum = lhs+rhs;

      setProperty("Output1", sum);
    }
  };

  class SimpleSum2 : public SimpleSum
  {
  public:
    const std::string name() const { return "SimpleSum2";} 
    int version() const  { return 1;}                        
    const std::string category() const { return "Dummy";}        

    void init()
    { 
      SimpleSum::init();
      declareProperty("Input3",4);
      declareProperty("Output2",-1,Direction::Output);
    }
    void exec() 
    {
      SimpleSum::exec();
      int sum = this->getProperty("Output1");
      setProperty("Output2", sum+1);
    }
  };

  void failiure_testfile_setup(int testno, NexusTestHelper &testfile)
  {
    //dummy file info
    std::stringstream output;

    // Environment history
    EnvironmentHistory envHist;
    output << envHist;
    char buffer [25];
    time_t now;
    time(&now);
    strftime (buffer,25,"%Y-%b-%d %H:%M:%S",localtime(&now));

    //common start
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
      //commonend

      if (testno != 6)
      {
        //first algorithm
        testfile.file->makeGroup("MantidAlgorithm_0", "NXnote", true);
          testfile.file->writeData("author", std::string("mantid"));
          testfile.file->writeData("description", std::string("Mantid Algorithm data"));
          testfile.file->writeData("data", "Algorithm: LoadRaw v1\nExecution Date: 2009-Oct-09 16:56:54\nExecution Duration: 2.3 seconds\nParameters:\n  Name: Filename, Value: /home/dmn58364/Mantid/trunk/Test/Data/GEM38370.raw, Default?: No, Direction: Input\n  Name: OutputWorkspace, Value: GEM38370, Default?: No, Direction: Output\n  Name: SpectrumMin, Value: 1, Default?: Yes, Direction: Input\n  Name: SpectrumMax, Value: 2147483632, Default?: Yes, Direction: Input\n  Name: SpectrumList, Value: , Default?: Yes, Direction: Input\n  Name: Cache, Value: If Slow, Default?: Yes, Direction: Input\n  Name: LoadLogFiles, Value: 1, Default?: Yes, Direction: Input");
        testfile.file->closeGroup();
        //second algorithm is different each test
        switch(testno)
        {
        case 2:
          {
            testfile.file->makeGroup("MantidAlgorithm_1", "NXnote", true);
              //missing author
              testfile.file->writeData("description", std::string("Mantid Algorithm data"));
              testfile.file->writeData("data", "Algorithm: LoadRaw v2\nExecution Date: 2009-Oct-09 16:56:54\nExecution Duration: 2.3 seconds\nParameters:\n  Name: Filename, Value: /home/dmn58364/Mantid/trunk/Test/Data/GEM38370.raw, Default?: No, Direction: Input\n  Name: OutputWorkspace, Value: GEM38370, Default?: No, Direction: Output\n  Name: SpectrumMin, Value: 1, Default?: Yes, Direction: Input\n  Name: SpectrumMax, Value: 2147483632, Default?: Yes, Direction: Input\n  Name: SpectrumList, Value: , Default?: Yes, Direction: Input\n  Name: Cache, Value: If Slow, Default?: Yes, Direction: Input\n  Name: LoadLogFiles, Value: 1, Default?: Yes, Direction: Input");
            testfile.file->closeGroup();
            break;
          }
        case 3:
          {
            testfile.file->makeGroup("MantidAlgorithm_1", "NXnote", true);
              testfile.file->writeData("author", std::string("mantid"));
              //missing description
              testfile.file->writeData("data", "Algorithm: LoadRaw v2\nExecution Date: 2009-Oct-09 16:56:54\nExecution Duration: 2.3 seconds\nParameters:\n  Name: Filename, Value: /home/dmn58364/Mantid/trunk/Test/Data/GEM38370.raw, Default?: No, Direction: Input\n  Name: OutputWorkspace, Value: GEM38370, Default?: No, Direction: Output\n  Name: SpectrumMin, Value: 1, Default?: Yes, Direction: Input\n  Name: SpectrumMax, Value: 2147483632, Default?: Yes, Direction: Input\n  Name: SpectrumList, Value: , Default?: Yes, Direction: Input\n  Name: Cache, Value: If Slow, Default?: Yes, Direction: Input\n  Name: LoadLogFiles, Value: 1, Default?: Yes, Direction: Input");
            testfile.file->closeGroup();
            break;
          }
        case 4:
          {
            testfile.file->makeGroup("MantidAlgorithm_1", "NXnote", true);
              testfile.file->writeData("author", std::string("mantid"));
              testfile.file->writeData("description", std::string("Mantid Algorithm data"));
              //missing data
            testfile.file->closeGroup();
            break;
          }
        case 5:
          {
            testfile.file->makeGroup("MantidAlgorithm_1", "NXnote", true);
              testfile.file->writeData("author", std::string("mantid"));
              testfile.file->writeData("description", std::string("Mantid Algorithm data"));
              testfile.file->writeData("data", "some data");
            testfile.file->closeGroup();
            break;
          }
        case 7:
          {
            testfile.file->makeGroup("MantidAlgorithm_1", "NXnote", true);
              testfile.file->writeData("author", std::string("mantid"));
              testfile.file->writeData("description", std::string("Mantid Algorithm data"));
              testfile.file->writeData("data", "some data\nsome data\nsome data\nsome data\nsome data");
            testfile.file->closeGroup();
            break;
          }
        default:
          {
            testfile.file->makeGroup("MantidAlgorithm_1", "NXnote", true);
              testfile.file->writeData("author", std::string("mantid"));
              testfile.file->writeData("description", std::string("Mantid Algorithm data"));
              testfile.file->writeData("data", "Algorithm: LoadRaw v2\nExecution Date: 2009-Oct-09 16:56:54\nExecution Duration: 2.3 seconds\nParameters:\n  Name: Filename, Value: /home/dmn58364/Mantid/trunk/Test/Data/GEM38370.raw, Default?: No, Direction: Input\n  Name: OutputWorkspace, Value: GEM38370, Default?: No, Direction: Output\n  Name: SpectrumMin, Value: 1, Default?: Yes, Direction: Input\n  Name: SpectrumMax, Value: 2147483632, Default?: Yes, Direction: Input\n  Name: SpectrumList, Value: , Default?: Yes, Direction: Input\n  Name: Cache, Value: If Slow, Default?: Yes, Direction: Input\n  Name: LoadLogFiles, Value: 1, Default?: Yes, Direction: Input");
            testfile.file->closeGroup();
            break;
          }
        }
        testfile.file->makeGroup("MantidAlgorithm_2", "NXnote", true);
          testfile.file->writeData("author", std::string("mantid"));
          testfile.file->writeData("description", std::string("Mantid Algorithm data"));
          testfile.file->writeData("data", "Algorithm: LoadRaw v3\nExecution Date: 2009-Oct-09 16:56:54\nExecution Duration: 2.3 seconds\nParameters:\n  Name: Filename, Value: /home/dmn58364/Mantid/trunk/Test/Data/GEM38370.raw, Default?: No, Direction: Input\n  Name: OutputWorkspace, Value: GEM38370, Default?: No, Direction: Output\n  Name: SpectrumMin, Value: 1, Default?: Yes, Direction: Input\n  Name: SpectrumMax, Value: 2147483632, Default?: Yes, Direction: Input\n  Name: SpectrumList, Value: , Default?: Yes, Direction: Input\n  Name: Cache, Value: If Slow, Default?: Yes, Direction: Input\n  Name: LoadLogFiles, Value: 1, Default?: Yes, Direction: Input");
        testfile.file->closeGroup();
      }
      else
      {
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
    //common end
    testfile.file->closeGroup();
  }

public:

  void test_New_History_Is_Empty()
  {
    WorkspaceHistory history;
    TS_ASSERT_EQUALS(history.size(), 0);
    TS_ASSERT_EQUALS(history.empty(), true);
  }

  void test_Adding_History_Entry()
  {
    WorkspaceHistory history;
    TS_ASSERT_EQUALS(history.size(), 0);
    TS_ASSERT_EQUALS(history.empty(), true);

    AlgorithmHistory alg1("FirstAlgorithm", 2);
    alg1.addProperty("FirstAlgProperty", "1",false, Mantid::Kernel::Direction::Input);
    history.addHistory(alg1);
    TS_ASSERT_EQUALS(history.size(), 1);
    TS_ASSERT_EQUALS(history.empty(), false);

    const WorkspaceHistory::AlgorithmHistories & algs = history.getAlgorithmHistories();
    TS_ASSERT_EQUALS(algs.size(), 1);
    TS_ASSERT_EQUALS(history.getAlgorithmHistory(0).name(), "FirstAlgorithm");
    TS_ASSERT_EQUALS((*algs.begin()).name(), "FirstAlgorithm");

  }

  void test_Asking_For_A_Given_Algorithm_Returns_The_Correct_One()
  {
    Mantid::API::AlgorithmFactory::Instance().subscribe<SimpleSum>();
    Mantid::API::AlgorithmFactory::Instance().subscribe<SimpleSum2>();

    SimpleSum simplesum;
    simplesum.initialize();
    simplesum.setPropertyValue("Input1", "5");
    simplesum.execute();
    SimpleSum2 simplesum2;
    simplesum2.initialize();
    simplesum2.setPropertyValue("Input3", "10");
    simplesum2.execute();

    WorkspaceHistory history;
    AlgorithmHistory alg1(&simplesum,Mantid::Kernel::DateAndTime::defaultTime(), 1.0, 0);
    AlgorithmHistory alg2(&simplesum2,Mantid::Kernel::DateAndTime::defaultTime(), 1.0, 1);
    history.addHistory(alg1);
    history.addHistory(alg2);

    AlgorithmHistory second = history.getAlgorithmHistory(1);
    TS_ASSERT_EQUALS(second.name(), "SimpleSum2");
    
    IAlgorithm_sptr first = history.getAlgorithm(0);
    TS_ASSERT_EQUALS(first->name(), "SimpleSum");
    TS_ASSERT_EQUALS(first->getPropertyValue("Input1"),"5");
    TS_ASSERT_EQUALS(first->getPropertyValue("Output1"),"6");

    // Last algorithm
    IAlgorithm_sptr lastAlg = history.lastAlgorithm();
    TS_ASSERT_EQUALS(lastAlg->name(), "SimpleSum2");
        
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("SimpleSum",1);
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("SimpleSum2",1);
  }

  void test_Empty_History_Throws_When_Retrieving_Attempting_To_Algorithms()
  {
    WorkspaceHistory emptyHistory;
    TS_ASSERT_THROWS(emptyHistory.lastAlgorithm(), std::out_of_range);
    TS_ASSERT_THROWS(emptyHistory.getAlgorithm(1), std::out_of_range);
  }

  void test_SaveNexus()
  {
    WorkspaceHistory testHistory;
    for (int i = 1; i < 5; i++)
    {
      testHistory.addHistory(AlgorithmHistory("History" + boost::lexical_cast<std::string>(i), 1,DateAndTime::defaultTime(),-1.0,i));
    }

    auto savehandle = boost::make_shared< ::NeXus::File >("WorkspaceHistoryTest_test_SaveNexus.nxs",NXACC_CREATE5);
    TS_ASSERT_THROWS_NOTHING(testHistory.saveNexus(savehandle.get()));
    savehandle->close();

    auto loadhandle = boost::make_shared< ::NeXus::File >("WorkspaceHistoryTest_test_SaveNexus.nxs");
    std::string rootstring = "/process/";
    for (int i = 1; i < 5; i++)
    {
      TS_ASSERT_THROWS_NOTHING(loadhandle->openPath(rootstring + "MantidAlgorithm_" + boost::lexical_cast<std::string>(i)));
    }
    TS_ASSERT_THROWS_NOTHING(loadhandle->openPath(rootstring + "MantidEnvironment"));
    TS_ASSERT_THROWS_NOTHING(loadhandle->openPath(rootstring + "MantidAlgorithm_4/author"));
    TS_ASSERT_THROWS_NOTHING(loadhandle->openPath(rootstring + "MantidAlgorithm_4/data"));
    TS_ASSERT_THROWS_NOTHING(loadhandle->openPath(rootstring + "MantidAlgorithm_4/description"));
    TS_ASSERT_THROWS_ANYTHING(loadhandle->openPath(rootstring + "MantidAlgorithm_5"));

    loadhandle->close();
    Poco::File("WorkspaceHistoryTest_test_SaveNexus.nxs").remove();
  }

  void test_SaveNexus_Empty()
  {
    WorkspaceHistory testHistory;

    auto savehandle = boost::make_shared< ::NeXus::File >("WorkspaceHistoryTest_test_SaveNexus.nxs",NXACC_CREATE5);
    TS_ASSERT_THROWS_NOTHING(testHistory.saveNexus(savehandle.get()));
    savehandle->close();

    auto loadhandle = boost::make_shared< ::NeXus::File >("WorkspaceHistoryTest_test_SaveNexus.nxs");
    std::string rootstring = "/process/";
    TS_ASSERT_THROWS_NOTHING(loadhandle->openPath(rootstring));
    TS_ASSERT_THROWS_NOTHING(loadhandle->openPath(rootstring + "MantidEnvironment"));
    TS_ASSERT_THROWS_ANYTHING(loadhandle->openPath(rootstring + "MantidAlgorithm_1"));
    
    loadhandle->close();
    Poco::File("WorkspaceHistoryTest_test_SaveNexus.nxs").remove();
  }

  void test_LoadNexus()
  {
    std::string filename = FileFinder::Instance().getFullPath("GEM38370_Focussed_Legacy.nxs");
    auto loadhandle = boost::make_shared< ::NeXus::File >(filename);
    loadhandle->openPath("/mantid_workspace_1");

    WorkspaceHistory emptyHistory;
    TS_ASSERT_THROWS_NOTHING(emptyHistory.loadNexus(loadhandle.get()));

    const auto & histories = emptyHistory.getAlgorithmHistories();
    TS_ASSERT_EQUALS(3,histories.size());
    
    const auto & history = emptyHistory.getAlgorithmHistory(0);

    TS_ASSERT_EQUALS("LoadRaw", history.name());
    TS_ASSERT_EQUALS(3, history.version());
    TS_ASSERT_EQUALS(DateAndTime("2009-10-09T16:56:54"), history.executionDate());
    TS_ASSERT_EQUALS(2.3, history.executionDuration());
    loadhandle->close();

  }

  void test_LoadNexus_Blank_File()
  {
    std::string rootstring = "/process/";
    //first file - clean - contains nothing
    NexusTestHelper testfile(true);
    testfile.createFile("LoadNexusTest.nxs");

    WorkspaceHistory history;
    //will throw nothing as it will return with only a warning, no exception
    TS_ASSERT_THROWS_NOTHING(history.loadNexus(testfile.file));
    const auto & histories = history.getAlgorithmHistories();
    
    TS_ASSERT_EQUALS(0,histories.size());
    TS_ASSERT_THROWS_ANYTHING(testfile.file->openPath(rootstring));
    TS_ASSERT_THROWS_ANYTHING(testfile.file->openPath(rootstring + "MantidEnvironment"));
    TS_ASSERT_THROWS_ANYTHING(testfile.file->openPath(rootstring + "MantidAlgorithm_1"));

  }

  void test_LoadNexus_Missing_Author()
  {
    //second file - 3 algorithms one missing an author
    NexusTestHelper testfile(true);
    failiure_testfile_setup(2, testfile);
    WorkspaceHistory history;
    TS_ASSERT_THROWS_NOTHING(history.loadNexus(testfile.file));
    const auto & histories = history.getAlgorithmHistories();
    //three will still exist as it doesn't really care about the author
    TS_ASSERT_EQUALS(3,histories.size());
  }

  void test_LoadNexus_Missing_Description()
  {
    //third file - 3 algorithms one missing a description
    NexusTestHelper testfile(true);
    failiure_testfile_setup(3, testfile);
    WorkspaceHistory history;
    TS_ASSERT_THROWS_NOTHING(history.loadNexus(testfile.file));
    const auto & histories = history.getAlgorithmHistories();
    
    //three will still exist as it doesn't really care about the author
    TS_ASSERT_EQUALS(3,histories.size());
  }

  void test_LoadNexus_Missing_Data()
  {
    //fourth file - 3 algorithms one missing data
    NexusTestHelper testfile(true);
    failiure_testfile_setup(4, testfile);
    WorkspaceHistory history;
    //this WILL throw as it looks for a data field using ::NeXus::File::readData() and it won't be found
    TS_ASSERT_THROWS_ANYTHING(history.loadNexus(testfile.file));
    const auto & histories = history.getAlgorithmHistories();
    //only one will exist as it will throw on the second (wihtout the data) and skip the rest
    TS_ASSERT_EQUALS(1,histories.size());
  }

  void test_LoadNexus_Short_Data()
  {
    //fifth file - 3 algorithms one wiht only one line of data
    NexusTestHelper testfile(true);
    failiure_testfile_setup(5, testfile);
    WorkspaceHistory history;
    //won't throw as it'll simply ignore the bad data
    TS_ASSERT_THROWS_NOTHING(history.loadNexus(testfile.file));
    const auto & histories = history.getAlgorithmHistories();
    
    //only two will exist as it will ignore the second (with only the single line) and continue as normal
    TS_ASSERT_EQUALS(2,histories.size());
  }

  void test_LoadNexus_All_Short_Data()
  {
    //sixth file - 3 algorithms all with only one line of data
    NexusTestHelper testfile(true);
    failiure_testfile_setup(6, testfile);
    WorkspaceHistory history;
    //nothign will throw but nothing will be loaded either as the data is invalid
    TS_ASSERT_THROWS_NOTHING(history.loadNexus(testfile.file));
    const auto & histories = history.getAlgorithmHistories();
    //size should be zero as nothing went in the file
    TS_ASSERT_EQUALS(0,histories.size());
  }

  void test_LoadNexus_Bad_Formatting()
  {
    //seventh file - 3 algorithms, one with bad formatting
    NexusTestHelper testfile(true);
    failiure_testfile_setup(7, testfile);
    WorkspaceHistory history;
    //this will throw on the second due to the unformatted data - the function expects well formatted data
    TS_ASSERT_THROWS_ANYTHING(history.loadNexus(testfile.file));
    const auto & histories = history.getAlgorithmHistories();
    //only one will exist as it will throw on the second (with the bad data) and skip the rest
    TS_ASSERT_EQUALS(1,histories.size());
  }
};



#endif
