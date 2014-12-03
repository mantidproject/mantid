#ifndef ALGORITHMTEST_H_
#define ALGORITHMTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/Property.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidTestHelpers/FakeObjects.h"
#include "MantidKernel/ReadLock.h"
#include "MantidKernel/WriteLock.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "FakeAlgorithms.h"
#include <map>

using namespace Mantid::Kernel; 
using namespace Mantid::API;

class StubbedWorkspaceAlgorithm : public Algorithm
{
public:
  StubbedWorkspaceAlgorithm() : Algorithm() {}
  virtual ~StubbedWorkspaceAlgorithm() {}

  const std::string name() const { return "StubbedWorkspaceAlgorithm";}
  int version() const  { return 1;}
  const std::string category() const { return "Cat;Leopard;Mink";}
  const std::string summary() const { return "Test summary"; }
  void init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace1", "", Direction::Input));
    declareProperty(new WorkspaceProperty<>("InputWorkspace2", "", Direction::Input, PropertyMode::Optional));
    declareProperty(new WorkspaceProperty<>("InOutWorkspace", "", Direction::InOut, PropertyMode::Optional));
    declareProperty("Number", 0.0);
    declareProperty(new WorkspaceProperty<>("OutputWorkspace1","",Direction::Output));
    declareProperty(new WorkspaceProperty<>("OutputWorkspace2","",Direction::Output, PropertyMode::Optional));
  }
  void exec()
  {
    boost::shared_ptr<WorkspaceTester> out1(new WorkspaceTester());
    out1->init(10,10,10);
    boost::shared_ptr<WorkspaceTester> out2(new WorkspaceTester());
    out2->init(10,10,10);
    std::string outName = getPropertyValue("InputWorkspace1")
            + "+" + getPropertyValue("InputWorkspace2")
            + "+" + getPropertyValue("InOutWorkspace");
    out1->setTitle(outName);
    out2->setTitle(outName);
    double val = getProperty("Number");
    out1->dataY(0)[0] = val;
    setProperty("OutputWorkspace1", out1);
    setProperty("OutputWorkspace2", out2);
    setProperty("OutputWorkspace2", out2);
  }
};
DECLARE_ALGORITHM( StubbedWorkspaceAlgorithm)


class StubbedWorkspaceAlgorithm2 : public Algorithm
{
public:
  StubbedWorkspaceAlgorithm2() : Algorithm() {}
  virtual ~StubbedWorkspaceAlgorithm2() {}

  const std::string name() const { return "StubbedWorkspaceAlgorithm2";}
  int version() const  { return 2;}
  const std::string category() const { return "Cat;Leopard;Mink";}
  const std::string summary() const { return "Test summary"; }
  void init()
  {
    declareProperty(new WorkspaceProperty<>("NonLockingInputWorkspace","",Direction::Input, PropertyMode::Optional, LockMode::NoLock));
    declareProperty(new WorkspaceProperty<>("NonLockingOutputWorkspace","",Direction::Output, PropertyMode::Optional, LockMode::NoLock));
  }
  void exec()
  {
  }
};
DECLARE_ALGORITHM(StubbedWorkspaceAlgorithm2)

class AlgorithmWithValidateInputs : public Algorithm
{
public:
  AlgorithmWithValidateInputs() : Algorithm() {}
  virtual ~AlgorithmWithValidateInputs() {}
  const std::string name() const { return "StubbedWorkspaceAlgorithm2";}
  int version() const  { return 1;}
  const std::string category() const { return "Cat;Leopard;Mink";}
  const std::string summary() const { return "Test summary"; }
  const std::string workspaceMethodName() const { return "methodname"; }
  const std::string workspaceMethodOnTypes() const { return "MatrixWorkspace;ITableWorkspace"; }
  const std::string workspaceMethodInputProperty() const { return "InputWorkspace"; }
  
  void init()
  {
    declareProperty("PropertyA", 12);
    declareProperty("PropertyB", 12);
  }
  void exec()
  {  }
  std::map<std::string, std::string> validateInputs()
  {
    std::map<std::string, std::string> out;
    int A = getProperty("PropertyA");
    int B = getProperty("PropertyB");
    if (B < A)
      out["PropertyB"] = "B must be >= A!";
    return out;
  }
};
DECLARE_ALGORITHM(AlgorithmWithValidateInputs)

/**
 * Algorithm which fails on specified workspace
 */
class FailingAlgorithm : public Algorithm
{
public:
  FailingAlgorithm() : Algorithm() {}
  virtual ~FailingAlgorithm() {}
  const std::string name() const { return "FailingAlgorithm"; }
  int version() const { return 1; }
  const std::string summary() const { return "Test summary"; }
  static const std::string FAIL_MSG;

  void init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input));
    declareProperty("WsNameToFail", "");
  }

  void exec()
  {
    std::string wsNameToFail = getPropertyValue("WsNameToFail");
    std::string wsName = getPropertyValue("InputWorkspace");

    if ( wsName == wsNameToFail )
    {
      throw std::runtime_error(FAIL_MSG);
    }
  }
};

const std::string FailingAlgorithm::FAIL_MSG("Algorithm failed as requested");

DECLARE_ALGORITHM(FailingAlgorithm)

class AlgorithmTest : public CxxTest::TestSuite
{
public:

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AlgorithmTest *createSuite() { return new AlgorithmTest(); }
  static void destroySuite( AlgorithmTest *suite ) { delete suite; }

  AlgorithmTest()
  {
    Mantid::API::FrameworkManager::Instance();
    AnalysisDataService::Instance();
    Mantid::API::AlgorithmFactory::Instance().subscribe<ToyAlgorithm>();
    Mantid::API::AlgorithmFactory::Instance().subscribe<ToyAlgorithmTwo>();
  }

  ~AlgorithmTest()
  {
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("ToyAlgorithm",1);
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("ToyAlgorithmTwo", 1);
  }
  
  void testAlgorithm()
  {
    std::string theName = alg.name();
    TS_ASSERT( ! theName.compare("ToyAlgorithm") );
    int theVersion = alg.version();
    TS_ASSERT_EQUALS( theVersion,1 );
    TS_ASSERT( ! alg.isInitialized() );
    TS_ASSERT( ! alg.isExecuted() );
  }

  void testName()
  {
    std::string theName = alg.name();
    TS_ASSERT( ! theName.compare("ToyAlgorithm") );
  }

  void testVersion()
  {
    int theVersion = alg.version();
    TS_ASSERT_EQUALS( theVersion,1 );
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( alg.category(),"Cat" );
    TS_ASSERT_EQUALS( algv2.category(),"Cat,Leopard,Mink" );
  }

  void testCategories()
  {
    std::vector<std::string> result;
    result.push_back("Cat");
    TS_ASSERT_EQUALS( alg.categories(),result );
    result.push_back("Leopard");
    result.push_back("Mink");
    TS_ASSERT_EQUALS( algv2.categories(),result );
    TS_ASSERT_EQUALS( algv3.categories(),result );
  }

  void testAlias()
  {
    TS_ASSERT_EQUALS( alg.alias(), "Dog");
  }

  void testIsChild()
  {
    TS_ASSERT_EQUALS(false, alg.isChild());
    alg.setChild(true);
    TS_ASSERT_EQUALS(true, alg.isChild());
    alg.setChild(false);
    TS_ASSERT_EQUALS(false, alg.isChild());
  }

  void testAlgStartupLogging()
  {
    TSM_ASSERT_EQUALS("Default logging should be true", true, alg.getAlgStartupLogging());
    alg.setAlgStartupLogging(false);
    TSM_ASSERT_EQUALS("After setting logging should be false", false, alg.getAlgStartupLogging());
    alg.setAlgStartupLogging(true);
    TSM_ASSERT_EQUALS("After setting logging it back it should be true", true, alg.getAlgStartupLogging());
  }

  void testInitialize()
  {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );
  }

  void testExecute()
  {
    ToyAlgorithm myAlg;
    TS_ASSERT_THROWS(myAlg.execute(),std::runtime_error);
    TS_ASSERT( ! myAlg.isExecuted() );
    TS_ASSERT_THROWS_NOTHING( myAlg.initialize());
    TS_ASSERT_THROWS_NOTHING(myAlg.execute() );
    TS_ASSERT( myAlg.isExecuted() );
  }


  void testSetPropertyValue()
  {
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("prop1","val") )
    TS_ASSERT_THROWS( alg.setPropertyValue("prop3","1"), Exception::NotFoundError )
  }

  void testExistsProperty()
  {
    TS_ASSERT( alg.existsProperty("prop1") )
    TS_ASSERT( ! alg.existsProperty("notThere") )
  }
  
  void testGetPropertyValue()
  {
    std::string value;
    TS_ASSERT_THROWS_NOTHING( value = alg.getPropertyValue("prop2") )
    TS_ASSERT( ! value.compare("1") )
    TS_ASSERT_THROWS(alg.getPropertyValue("ghjkgh"), Exception::NotFoundError )    
  }
  
  void testGetProperties()
  {
    std::vector<Property*> vec = alg.getProperties();
    TS_ASSERT( ! vec.empty() )
    TS_ASSERT( vec.size() == 2 )
    TS_ASSERT( ! vec[0]->name().compare("prop1") )
  }

  /** The check in validateInputs() makes the algo throw if there is anything wrong */
  void test_validateInputs_makesAlgorithmFail()
  {
    AlgorithmWithValidateInputs alg;
    alg.initialize();
    alg.setProperty("PropertyA", 12);
    alg.setProperty("PropertyB", 5);
    TS_ASSERT_THROWS_ANYTHING( alg.execute() );
    TS_ASSERT( !alg.isExecuted() );

    alg.setProperty("PropertyB", 15);
    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );
  }

  void test_WorkspaceMethodFunctionsReturnEmptyByDefault()
  {
    StubbedWorkspaceAlgorithm alg;

    TS_ASSERT_EQUALS("", alg.workspaceMethodName());
    TS_ASSERT_EQUALS(std::vector<std::string>(), alg.workspaceMethodOn());
    TS_ASSERT_EQUALS("", alg.workspaceMethodInputProperty());
  }

  void test_WorkspaceMethodsReturnTypesCorrectly()
  {
    AlgorithmWithValidateInputs alg;

    TS_ASSERT_EQUALS("methodname", alg.workspaceMethodName());
    auto types = alg.workspaceMethodOn();
    TS_ASSERT_EQUALS(2, types.size());
    if(types.size() == 2)
    {
      TS_ASSERT_EQUALS("MatrixWorkspace", types[0]);
      TS_ASSERT_EQUALS("ITableWorkspace", types[1]);
    }
    TS_ASSERT_EQUALS("InputWorkspace", alg.workspaceMethodInputProperty());
  }

  void testStringization()
  {
    //Set the properties so that we know what they are
    alg.setPropertyValue("prop1", "value1");
    alg.setProperty("prop2", 5);
    std::string expected = "ToyAlgorithm.1(prop1=value1,prop2=5)";
    TS_ASSERT_EQUALS(alg.toString(), expected);
  }

  void test_From_String_With_Invalid_Input_Throws()
  {
    const std::string input = "()";
    TS_ASSERT_THROWS(Algorithm::fromString(input), std::runtime_error );
  }

  void test_Construction_Via_Valid_String_With_No_Properties()
  {
    IAlgorithm_sptr testAlg = runFromString("ToyAlgorithm");
    TS_ASSERT_EQUALS(testAlg->name(), "ToyAlgorithm");
    TS_ASSERT_EQUALS(testAlg->version(), 2);
  }

  void test_Construction_Via_Valid_String_With_Version()
  {
    IAlgorithm_sptr testAlg = runFromString("ToyAlgorithm.1");
    TS_ASSERT_EQUALS(testAlg->name(), "ToyAlgorithm");
    TS_ASSERT_EQUALS(testAlg->version(), 1);
    
    // No brackets
    testAlg = runFromString("ToyAlgorithm.1");
    TS_ASSERT_EQUALS(testAlg->name(), "ToyAlgorithm");
    TS_ASSERT_EQUALS(testAlg->version(), 1);
  }

  void test_Construction_Via_Valid_String_With_Version_And_Empty_Props()
  {
    IAlgorithm_sptr testAlg = runFromString("ToyAlgorithm.1()");
    TS_ASSERT_EQUALS(testAlg->name(), "ToyAlgorithm");
    TS_ASSERT_EQUALS(testAlg->version(), 1);
    
    // No brackets
    testAlg = runFromString("ToyAlgorithm.1");
    TS_ASSERT_EQUALS(testAlg->name(), "ToyAlgorithm");
    TS_ASSERT_EQUALS(testAlg->version(), 1);
  }


  void test_Construction_Via_Valid_String_With_Set_Properties_And_Version()
  {
    IAlgorithm_sptr testAlg = runFromString("ToyAlgorithm.2(prop1=val1,prop2=8,prop3=10.0,Binning=0.2,0.2,1.4)");
    TS_ASSERT_EQUALS(testAlg->name(), "ToyAlgorithm");
    TS_ASSERT_EQUALS(testAlg->version(), 2);
    
    // On gcc we get ambiguous function calls doing
    // std::string s;
    // s = getProperty(...);
    // so we have to do this
    try
    {
      std::string prop1 = testAlg->getProperty("prop1");
      TS_ASSERT_EQUALS(prop1,"val1");
    }
    catch(...)
    {
      TS_FAIL("Cannot retrieve property 'prop1'");
    }
    try
    {
      int prop2 = testAlg->getProperty("prop2");
      TS_ASSERT_EQUALS(prop2, 8);
    }
    catch(...)
    {
       TS_FAIL("Cannot retrieve property 'prop2'");
    }
    try
    {
      double prop3 = testAlg->getProperty("prop3");
      TS_ASSERT_EQUALS(prop3, 10.0);
    }
    catch(...)
    {
      TS_FAIL("Cannot retrieve property 'prop3'");
    }
    try
    {
      std::vector<double> prop3 = testAlg->getProperty("Binning");
      TS_ASSERT_EQUALS(prop3.size(), 3);
    }
    catch(...)
    {
       TS_FAIL("Cannot retrieve property 'Binning'");
    }
  }

  void test_Construction_Via_Valid_String_With_Single_Property_And_Version()
  {
    IAlgorithm_sptr testAlg = runFromString("ToyAlgorithm.2(prop3=10.0)");
    TS_ASSERT_EQUALS(testAlg->name(), "ToyAlgorithm");
    TS_ASSERT_EQUALS(testAlg->version(), 2);

    try
    {
      double prop3 = testAlg->getProperty("prop3");
      TS_ASSERT_EQUALS(prop3, 10.0);
    }
    catch(...)
    {
      TS_FAIL("Cannot retrieve property 'prop3'");
    }
    try
    {
      std::vector<double> prop3 = testAlg->getProperty("Binning");
      TS_ASSERT_EQUALS(prop3.size(), 3);
    }
    catch(...)
    {
       TS_FAIL("Cannot retrieve property 'Binning'");
    }
  }

  void test_Construction_Via_Valid_String_With_Single_Property_Array()
  {
    IAlgorithm_sptr testAlg = runFromString("ToyAlgorithm.2(Binning=0.2,0.2,1.4)");
    TS_ASSERT_EQUALS(testAlg->name(), "ToyAlgorithm");
    TS_ASSERT_EQUALS(testAlg->version(), 2);

    try
    {
      std::vector<double> prop3 = testAlg->getProperty("Binning");
      TS_ASSERT_EQUALS(prop3.size(), 3);
      TS_ASSERT_EQUALS(prop3[2], 1.4);
    }
    catch(...)
    {
       TS_FAIL("Cannot retrieve property 'Binning'");
    }
  }

  void test_Construction_Via_Valid_String_With_Empty_Properties()
  {
    IAlgorithm_sptr testAlg = runFromString("ToyAlgorithm()");
    TS_ASSERT_EQUALS(testAlg->name(), "ToyAlgorithm");
    TS_ASSERT_EQUALS(testAlg->version(), 2);
     try
    {
      std::string prop1 = testAlg->getProperty("prop1");
      TS_ASSERT_EQUALS(prop1,"value");
    }
    catch(...)
    {
      TS_FAIL("Cannot retrieve property 'prop1'");
    }
    try
    {
      int prop2 = testAlg->getProperty("prop2");
      TS_ASSERT_EQUALS(prop2, 1);
    }
    catch(...)
    {
       TS_FAIL("Cannot retrieve property 'prop2'");
    }
    try
    {
      std::vector<double> prop3 = testAlg->getProperty("Binning");
      TS_ASSERT_EQUALS(prop3.size(), 3);
    }
    catch(...)
    {
       TS_FAIL("Cannot retrieve property 'Binning'");
    }

  }


  //------------------------------------------------------------------------
  /** Test of setting read and/or write locks
   * for various combinations of input/output workspaces.
   */
  void do_test_locking(std::string in1, std::string in2, std::string inout, std::string out1, std::string out2)
  {
    for (size_t i=0; i<6; i++)
    {
      boost::shared_ptr<WorkspaceTester> ws(new WorkspaceTester());
      AnalysisDataService::Instance().addOrReplace("ws" + Strings::toString(i), ws);
    }
    StubbedWorkspaceAlgorithm alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace1", in1);
    alg.setPropertyValue("InputWorkspace2", in2);
    alg.setPropertyValue("InOutWorkspace", inout);
    alg.setPropertyValue("OutputWorkspace1", out1);
    alg.setPropertyValue("OutputWorkspace2", out2);
    // This throws or hangs if the code is wrong
    alg.execute();
  }


  //------------------------------------------------------------------------
  void test_lockingWorkspaces()
  {
    // Input and output are different
    do_test_locking("ws0", "", "", "ws1", "");
    // Repeated output workspaces
    do_test_locking("ws0", "", "", "ws1", "ws1");
    // Different output workspaces
    do_test_locking("ws0", "", "", "ws1", "ws2");
    // Input and output are same
    do_test_locking("ws0", "", "", "ws0", "");
    // Two input workspaces
    do_test_locking("ws0", "ws0", "", "ws5", "");
    // Also in-out workspace
    do_test_locking("ws0", "ws0", "ws0", "ws0", "");
    // All the same
    do_test_locking("ws0", "ws0", "ws0", "ws0", "ws0");
  }

  /** Have a workspace property that does NOT lock the workspace.
   * The failure mode of this test is HANGING. */
  void test_workspace_notLocking()
  {
    boost::shared_ptr<WorkspaceTester> ws1(new WorkspaceTester());
    AnalysisDataService::Instance().addOrReplace("ws1", ws1);

    {
      // Get a write lock.
      WriteLock _lock(*ws1);
      // The algorithm would hang waiting for the write-lock to release if the property were locking.
      StubbedWorkspaceAlgorithm2 alg;
      alg.initialize();
      alg.setPropertyValue("NonLockingInputWorkspace", "ws1");
      alg.execute();
      TS_ASSERT( alg.isExecuted() );
    }
    {
      // Acquire a scoped read-lock on ws1.
      ReadLock _lock(*ws1);
      // The algo would lock up when trying to WRITE-lock the workspace again
      StubbedWorkspaceAlgorithm2 alg;
      alg.initialize();
      alg.setPropertyValue("NonLockingOutputWorkspace", "ws1");
      alg.execute();
      TS_ASSERT( alg.isExecuted() );
    }

  }

  //------------------------------------------------------------------------
  /** Make a workspace group with:
   *
   * @param group1 :: name of the group. Do nothing if blank.
   * @param contents1 :: comma-sep names of fake workspaces in the group
   *        Make no group if blank, just 1 workspace
   */
  void makeWorkspaceGroup(std::string group1, std::string contents1)
  {
    if (contents1.empty())
    {
      if (group1.empty()) return;
      boost::shared_ptr<WorkspaceTester> ws(new WorkspaceTester());
      AnalysisDataService::Instance().addOrReplace(group1,ws);
      return;
    }

    std::vector<std::string> names;
    boost::split( names, contents1, boost::algorithm::detail::is_any_ofF<char>(","));
    if (names.size() >= 1)
    {
      WorkspaceGroup_sptr wsGroup = WorkspaceGroup_sptr(new WorkspaceGroup());
      AnalysisDataService::Instance().addOrReplace(group1, wsGroup);
      std::vector<std::string>::iterator it = names.begin();
      for (; it != names.end(); it++)
      {
        boost::shared_ptr<WorkspaceTester> ws(new WorkspaceTester());
        ws->init(10,10,10);
        AnalysisDataService::Instance().addOrReplace(*it,ws);
        wsGroup->add(*it);
      }
    }
  }

  //------------------------------------------------------------------------
  WorkspaceGroup_sptr do_test_groups(
      std::string group1, std::string contents1,
      std::string group2, std::string contents2,
      std::string group3, std::string contents3,
      bool expectFail = false,
      int expectedNumber = 3
      )
  {
    makeWorkspaceGroup(group1, contents1);
    makeWorkspaceGroup(group2, contents2);
    makeWorkspaceGroup(group3, contents3);

    StubbedWorkspaceAlgorithm alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace1", group1);
    alg.setPropertyValue("InputWorkspace2", group2);
    alg.setPropertyValue("InOutWorkspace", group3);
    alg.setPropertyValue("Number", "234");
    alg.setPropertyValue("OutputWorkspace1", "D");
    alg.setPropertyValue("OutputWorkspace2", "E");
    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    if (expectFail)
    {
      TS_ASSERT( !alg.isExecuted() );
      return WorkspaceGroup_sptr();
    } 
    TS_ASSERT( alg.isExecuted() )
    Workspace_sptr out1 = AnalysisDataService::Instance().retrieve("D");
    WorkspaceGroup_sptr group = boost::dynamic_pointer_cast<WorkspaceGroup>(out1);

    TS_ASSERT_EQUALS( group->name(), "D" )
    TS_ASSERT_EQUALS( group->getNumberOfEntries(), expectedNumber )
    if (group->getNumberOfEntries() < 1) return group;
    ws1 = boost::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(0));
    if (group->getNumberOfEntries() < 2) return group;
    ws2 = boost::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(1));
    if (group->getNumberOfEntries() < 3) return group;
    ws3 = boost::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(2));
    return group;
  }

  void test_processGroups_failures()
  {
    // Fails due to unequal sizes.
    do_test_groups("A", "A_1,A_2,A_3",
        "B", "B_1,B_2,B_3,B_4",   "", "",
        true /*fails*/);
  }

  /// All groups are the same size
  void test_processGroups_allSameSize()
  {
      Mantid::API::AnalysisDataService::Instance().clear();
    WorkspaceGroup_sptr group = do_test_groups("A", "A_1,A_2,A_3",
        "B", "B_1,B_2,B_3",   "C", "C_1,C_2,C_3");

    TS_ASSERT_EQUALS( ws1->name(), "D_1" );
    TS_ASSERT_EQUALS( ws1->getTitle(), "A_1+B_1+C_1" );
    TS_ASSERT_EQUALS( ws1->readY(0)[0], 234 );
    TS_ASSERT_EQUALS( ws2->name(), "D_2" );
    TS_ASSERT_EQUALS( ws2->getTitle(), "A_2+B_2+C_2" );
    TS_ASSERT_EQUALS( ws3->name(), "D_3" );
    TS_ASSERT_EQUALS( ws3->getTitle(), "A_3+B_3+C_3" );
  }

  /// All groups are the same size, but they don't all match the rigid naming
  void test_processGroups_allSameSize_namesNotSimilar()
  {
    WorkspaceGroup_sptr group = do_test_groups("A", "A_1,A_2,A_3",
        "B", "B_1,B_2,B_3",   "C", "alice,bob,charlie");

    TS_ASSERT_EQUALS( ws1->name(), "A_1_B_1_alice_D" );
    TS_ASSERT_EQUALS( ws1->getTitle(), "A_1+B_1+alice" );
    TS_ASSERT_EQUALS( ws1->readY(0)[0], 234 );
    TS_ASSERT_EQUALS( ws2->name(), "A_2_B_2_bob_D" );
    TS_ASSERT_EQUALS( ws2->getTitle(), "A_2+B_2+bob" );
    TS_ASSERT_EQUALS( ws3->name(), "A_3_B_3_charlie_D" );
    TS_ASSERT_EQUALS( ws3->getTitle(), "A_3+B_3+charlie" );
  }

  /// One input is a group, rest are singles
  void test_processGroups_onlyOneGroup()
  {
    WorkspaceGroup_sptr group = do_test_groups("A", "A_1,A_2,A_3",
        "B", "",   "C", "");

    TS_ASSERT_EQUALS( ws1->name(), "D_1" );
    TS_ASSERT_EQUALS( ws1->getTitle(), "A_1+B+C" );
    TS_ASSERT_EQUALS( ws1->readY(0)[0], 234 );
    TS_ASSERT_EQUALS( ws2->name(), "D_2" );
    TS_ASSERT_EQUALS( ws2->getTitle(), "A_2+B+C" );
    TS_ASSERT_EQUALS( ws3->name(), "D_3" );
    TS_ASSERT_EQUALS( ws3->getTitle(), "A_3+B+C" );
  }

  /// One optional WorkspaceProperty is not specified
  void test_processGroups_optionalInput()
  {
    WorkspaceGroup_sptr group = do_test_groups("A", "A_1,A_2,A_3",
        "B", "",   "", "");

    TS_ASSERT_EQUALS( ws1->name(), "D_1" );
    TS_ASSERT_EQUALS( ws1->getTitle(), "A_1+B+" );
    TS_ASSERT_EQUALS( ws1->readY(0)[0], 234 );
    TS_ASSERT_EQUALS( ws2->name(), "D_2" );
    TS_ASSERT_EQUALS( ws2->getTitle(), "A_2+B+" );
    TS_ASSERT_EQUALS( ws3->name(), "D_3" );
    TS_ASSERT_EQUALS( ws3->getTitle(), "A_3+B+" );
  }

  /// One optional WorkspaceProperty is not specified
  void test_processGroups_twoGroups_and_optionalInput()
  {
    WorkspaceGroup_sptr group = do_test_groups("A", "A_1,A_2,A_3",
        "", "", "C", "C_1,C_2,C_3");

    TS_ASSERT_EQUALS( ws1->name(), "D_1" );
    TS_ASSERT_EQUALS( ws1->getTitle(), "A_1++C_1" );
    TS_ASSERT_EQUALS( ws1->readY(0)[0], 234 );
    TS_ASSERT_EQUALS( ws2->name(), "D_2" );
    TS_ASSERT_EQUALS( ws2->getTitle(), "A_2++C_2" );
    TS_ASSERT_EQUALS( ws3->name(), "D_3" );
    TS_ASSERT_EQUALS( ws3->getTitle(), "A_3++C_3" );
  }

  /// One input is a group with only one member (not possible via GUI)
  void test_processGroups_onlyOneGroup_withOnlyOneMember()
  {
    WorkspaceGroup_sptr group = do_test_groups("A", "A_1",
        "B", "",   "C", "", false, 1);

    TS_ASSERT_EQUALS( ws1->name(), "D_1" );
    TS_ASSERT_EQUALS( ws1->getTitle(), "A_1+B+C" );
    TS_ASSERT_EQUALS( ws1->readY(0)[0], 234 );
  }

  /// Two inputs are groups with one member (each)
  void test_processGroups_twoGroup_withOnlyOneMember()
  {
    WorkspaceGroup_sptr group = do_test_groups("A", "A_1",
        "B", "B_1",   "C", "", false, 1);

    TS_ASSERT_EQUALS( ws1->name(), "D_1" );
    TS_ASSERT_EQUALS( ws1->getTitle(), "A_1+B_1+C" );
    TS_ASSERT_EQUALS( ws1->readY(0)[0], 234 );
  }

  void test_processGroups_failOnGroupMemberErrorMessage()
  {
    makeWorkspaceGroup("A", "A_1,A_2,A_3");

    FailingAlgorithm alg;
    alg.initialize();
    alg.setRethrows(true);
    alg.setLogging(false);
    alg.setPropertyValue("InputWorkspace", "A");
    alg.setPropertyValue("WsNameToFail", "A_2");

    try
    {
      alg.execute();
      TS_FAIL("Exception wasn't thrown");
    }
    catch(std::runtime_error& e)
    {
      std::string msg(e.what());

      TSM_ASSERT("Error message should contain original error",
                 msg.find(FailingAlgorithm::FAIL_MSG) != std::string::npos);
    }
  }

private:
  IAlgorithm_sptr runFromString(const std::string & input)
  {
    IAlgorithm_sptr testAlg;
    TS_ASSERT_THROWS_NOTHING(testAlg = Algorithm::fromString(input) );
    TS_ASSERT(testAlg);
    if(!testAlg) TS_FAIL("Failed to create algorithm, cannot continue test.");
    return testAlg;
  }

  ToyAlgorithm alg;
  ToyAlgorithmTwo algv2;
  ToyAlgorithmThree algv3;

  MatrixWorkspace_sptr ws1;
  MatrixWorkspace_sptr ws2;
  MatrixWorkspace_sptr ws3;
};

 

#endif /*ALGORITHMTEST_H_*/
