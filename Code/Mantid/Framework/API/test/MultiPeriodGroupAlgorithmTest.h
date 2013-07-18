#ifndef MANTID_API_MultiPeriodGroupAlgorithmTEST_H_
#define MANTID_API_MultiPeriodGroupAlgorithmTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/MultiPeriodGroupAlgorithm.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidTestHelpers/FakeObjects.h"
#include "MantidKernel/MandatoryValidator.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

// ------------------------------------------------------------------
// Working, concrete MultiPeriodGroupAlgorithm. With single input array property.
class TestAlgorithmA : public MultiPeriodGroupAlgorithm
{
public:
  TestAlgorithmA(){}
  virtual const std::string name() const {return "TestAlgorithmA";}
  virtual int version() const {return 1;}
  virtual void init() 
  {
    declareProperty(new ArrayProperty<std::string>("MyInputWorkspaces"));
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "");
    declareProperty("PropertyA", 1, boost::make_shared<Kernel::MandatoryValidator<int> >()); // I'm only adding this property to cause errors if it's not passed to spawned algorithms.
  }
  virtual void exec()
  {
    setProperty("OutputWorkspace", Workspace_sptr(new WorkspaceTester));
  }
  virtual std::string fetchInputPropertyName() const
  {
    return "MyInputWorkspaces";
  }
  virtual bool useCustomInputPropertyName() const
  {
    return true;
  }
  virtual ~TestAlgorithmA()
  {
  }
};
DECLARE_ALGORITHM( TestAlgorithmA)
// ------------------------------------------------------------------
// End class dec

// ------------------------------------------------------------------
// Working, concrete MultiPeriodGroupAlgorithm. With proper named group input properties.
class TestAlgorithmB : public MultiPeriodGroupAlgorithm
{
public:
  TestAlgorithmB(){}
  virtual const std::string name() const {return "TestAlgorithmB";}
  virtual int version() const {return 1;}
  virtual void init() 
  {
    declareProperty(new WorkspaceProperty<>("PropertyA", "ws1", Direction::Input));
    declareProperty(new WorkspaceProperty<>("PropertyB", "ws2", Direction::Input));
    declareProperty(new WorkspaceProperty<>("PropertyC", "ws3", Direction::Input));
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "");
    declareProperty("PropertyX", 1, boost::make_shared<Kernel::MandatoryValidator<int> >()); // I'm only adding this property to cause errors if it's not passed to spawned algorithms.
  }
  virtual void exec()
  {
    MatrixWorkspace_sptr a = getProperty("PropertyA");
    MatrixWorkspace_sptr b = getProperty("PropertyB");
    MatrixWorkspace_sptr c = getProperty("PropertyC");
    int x = getProperty("PropertyX");
    UNUSED_ARG(a);
    UNUSED_ARG(b);
    UNUSED_ARG(c);
    UNUSED_ARG(x);

    setProperty("OutputWorkspace", Workspace_sptr(new WorkspaceTester));
  }
  virtual std::string fetchInputPropertyName() const
  {
    return "";
  }
  virtual bool useCustomInputPropertyName() const
  {
    return false;
  }
  virtual ~TestAlgorithmB()
  {
  }
};
DECLARE_ALGORITHM( TestAlgorithmB)
// ------------------------------------------------------------------
// End class dec


class MultiPeriodGroupAlgorithmTest : public CxxTest::TestSuite
{
private:

  // Helper method to add multiperiod logs to make a workspacegroup look like a real multiperiod workspace group.
  void add_periods_logs(WorkspaceGroup_sptr ws)
  {
    int nperiods = static_cast<int>(ws->size());
    for(size_t i = 0; i < ws->size(); ++i)
    { 
      MatrixWorkspace_sptr currentWS = boost::dynamic_pointer_cast<MatrixWorkspace>(ws->getItem(i));
      PropertyWithValue<int>* nperiodsProp = new PropertyWithValue<int>("nperiods", nperiods);
      currentWS->mutableRun().addLogData(nperiodsProp);
      PropertyWithValue<int>* currentPeriodsProp = new PropertyWithValue<int>("current_period", static_cast<int>(i+1));
      currentWS->mutableRun().addLogData(currentPeriodsProp);
    }
  }
    /// Helper to fabricate a workspace group consisting of equal sized matrixworkspaces.
  WorkspaceGroup_sptr create_good_multiperiod_workspace_group(const std::string name)
  {
    MatrixWorkspace_sptr a = MatrixWorkspace_sptr(new WorkspaceTester);
    MatrixWorkspace_sptr b = MatrixWorkspace_sptr(new WorkspaceTester);
    //a->setName(name + "_1");
    //b->setName(name + "_2");
    WorkspaceGroup_sptr group = boost::make_shared<WorkspaceGroup>();
    //group->setName(name);
    group->addWorkspace(a);
    group->addWorkspace(b);
    add_periods_logs(group);
//    AnalysisDataService::Instance().addOrReplace(a->name(), a);
//    AnalysisDataService::Instance().addOrReplace(b->name(), b);
    AnalysisDataService::Instance().addOrReplace(name+"_1", a);
    AnalysisDataService::Instance().addOrReplace(name+"_2", b);
    AnalysisDataService::Instance().addOrReplace(name, group);
    return group;
  }

public:

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MultiPeriodGroupAlgorithmTest *createSuite() { return new MultiPeriodGroupAlgorithmTest(); }
  static void destroySuite( MultiPeriodGroupAlgorithmTest *suite ) { delete suite; }

  // Note that we may wish to retire this test if we support other input property types in the future.
  void test_input_property_not_string_array_throws()
  {
    // ------------------------------------------------------------------
    // Test Algorithm with input property that is not an array.
    class BrokenAlgorithm : public MultiPeriodGroupAlgorithm
    {
    public:
      virtual const std::string name() const {return "BrokenAlgorithm";}
      virtual int version() const {return 1;}
      virtual void init() 
      {
        declareProperty(new WorkspaceProperty<WorkspaceGroup>("InputWorkspaces","",Direction::Input), "");
        declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "");
      }
      virtual void exec()
      {
        setProperty("OutputWorkspace", Workspace_sptr(new WorkspaceTester));
      }
      virtual std::string fetchInputPropertyName() const
      {
        return "InputWorkspaces";
      }
      virtual bool useCustomInputPropertyName() const {return true;}
      virtual ~BrokenAlgorithm()
      {
      }
    };
    // ------------------------------------------------------------------
    // End class dec

    WorkspaceGroup_sptr testInput = create_good_multiperiod_workspace_group("test");

    BrokenAlgorithm alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspaces", testInput);
    alg.setPropertyValue("OutputWorkspace", "outWS");
    TSM_ASSERT_THROWS("Should throw because fetchInputPropertyName is returning the name of a property which doesn't exist.", alg.execute(), std::runtime_error);
  }


  void test_input_property_doesnt_exist_throws()
  {
    // ------------------------------------------------------------------
    // Test Algorithm with fetchInputPropertyName incorrectly wired-up.
    class BrokenAlgorithm : public MultiPeriodGroupAlgorithm
    {
    public:
      virtual const std::string name() const {return "BrokenAlgorithm";}
      virtual int version() const {return 1;}
      virtual void init() 
      {
        declareProperty(new ArrayProperty<std::string>("InputWorkspaces"));
        declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "Name of the output workspace" );
      }
      virtual void exec()
      {
        setProperty("OutputWorkspace", Workspace_sptr(new WorkspaceTester));
      }
      virtual std::string fetchInputPropertyName() const
      {
        return "made_up_property_name";
      }
      virtual bool useCustomInputPropertyName() const {return true;}
      virtual ~BrokenAlgorithm()
      {
      }
    };
    // ------------------------------------------------------------------
    // End class dec

    WorkspaceGroup_sptr a = create_good_multiperiod_workspace_group("a");
    WorkspaceGroup_sptr b = create_good_multiperiod_workspace_group("b");

    BrokenAlgorithm alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("InputWorkspaces", "a, b");
    alg.setPropertyValue("OutputWorkspace", "outWS");
    TSM_ASSERT_THROWS("Should throw because fetchInputPropertyName is returning the name of a property which doesn't exist.", alg.execute(), Kernel::Exception::NotFoundError);
  }

  void test_process_groups_with_array_input()
  {
    WorkspaceGroup_sptr a = create_good_multiperiod_workspace_group("a");
    WorkspaceGroup_sptr b = create_good_multiperiod_workspace_group("b");
    WorkspaceGroup_sptr c = create_good_multiperiod_workspace_group("c");

    TestAlgorithmA alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("MyInputWorkspaces", "a, b, c");
    alg.setProperty("PropertyA", 1);
    alg.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    WorkspaceGroup_sptr wsgroup = Mantid::API::AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("outWS");
    TS_ASSERT(wsgroup != NULL);
    TS_ASSERT_EQUALS(a->size(), wsgroup->size());
  }

  void xtest_process_groups_with_workspace_type_inputs()
  {
    WorkspaceGroup_sptr a = create_good_multiperiod_workspace_group("a");
    WorkspaceGroup_sptr b = create_good_multiperiod_workspace_group("b");
    WorkspaceGroup_sptr c = create_good_multiperiod_workspace_group("c");

    AnalysisDataService::Instance().addOrReplace("ws1", a);
    AnalysisDataService::Instance().addOrReplace("ws2", b);
    AnalysisDataService::Instance().addOrReplace("ws3", c);

    TestAlgorithmB alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("PropertyA", a);
    alg.setProperty("PropertyB", b);
    alg.setProperty("PropertyC", c);
    alg.setProperty("PropertyX", 1);
    alg.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    WorkspaceGroup_sptr wsgroup = Mantid::API::AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("outWS");
    TS_ASSERT(wsgroup != NULL);
    TS_ASSERT_EQUALS(a->size(), wsgroup->size());
  }


};


#endif /* MANTID_API_MultiPeriodGroupAlgorithmTEST_H_ */
