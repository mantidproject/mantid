#ifndef ANALYSISDATASERVICETEST_H_
#define ANALYSISDATASERVICETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace
{
  class MockWorkspace : public Workspace
  {
    virtual const std::string id() const { return "MockWorkspace"; }
    virtual size_t getMemorySize() const { return 1; }
  };
  typedef boost::shared_ptr<MockWorkspace> MockWorkspace_sptr;
}

class AnalysisDataServiceTest : public CxxTest::TestSuite
{
public:

  void test_IsValid_Returns_An_Empty_String_For_A_Valid_Name_When_All_CharsAre_Allowed()
  {
    AnalysisDataServiceImpl & ads = AnalysisDataService::Instance();
    TS_ASSERT_EQUALS(ads.isValid("CamelCase"), "");
    TS_ASSERT_EQUALS(ads.isValid("_Has_Underscore"), "");
    TS_ASSERT_EQUALS(ads.isValid("alllowercase"), "");
    TS_ASSERT_EQUALS(ads.isValid("ALLUPPERCASE"), "");
  }

  void test_IsValid_Returns_An_Error_String_For_A_Invalid_Name()
  {
    AnalysisDataServiceImpl & ads = AnalysisDataService::Instance();
    const std::string illegalChars = " +-/*\\%<>&|^~=!@()[]{},:.`$'\"?";
    ads.setIllegalCharacterList(illegalChars);
    const size_t nchars(illegalChars.size());
    for( size_t i = 0; i < nchars; ++i )
    {
      std::ostringstream name;
      name << "NotAllowed" << illegalChars[i];
      std::ostringstream expectedError;
      expectedError << "Invalid object name '" << name.str()  << "'. Names cannot contain any of the following characters: " << illegalChars;
      TS_ASSERT_EQUALS(ads.isValid(name.str()), expectedError.str());
    }
    // Clean up
    ads.setIllegalCharacterList("");
  }

  void test_Retrieve_Checks_For_Exact_Match_Then_Lower_Upper_And_Sentence_Case()
  {
    addToADS("z");
    addToADS("Z");
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().retrieve("z"));
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().retrieve("Z"));

    removeFromADS("z");// Remove lower case
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().retrieve("z")); // Will find upper case
    removeFromADS("z");// Remove lower case
    TS_ASSERT_THROWS(AnalysisDataService::Instance().retrieve("z"), Exception::NotFoundError);
  }

  void test_Add_With_Name_That_Has_No_Special_Chars_Is_Accpeted()
  {
    const std::string name = "MySpace";
    TS_ASSERT_THROWS_NOTHING(addToADS(name));
    TS_ASSERT(isInADS(name));
    TS_ASSERT_THROWS_NOTHING(removeFromADS(name));
  }

  void test_Adding_A_Second_Item_Of_Same_Name_Throws_Runtime_Error()
  {
    const std::string name = "SameName";
    TS_ASSERT_THROWS_NOTHING(addToADS(name));
    // Adding again will throw
    TS_ASSERT_THROWS(addToADS(name), std::runtime_error);
    TS_ASSERT_THROWS_NOTHING(removeFromADS(name));
  }

  void test_Add_With_Name_Containing_Special_Chars_Throws_Invalid_Argument()
  {
    this->doAddingOnInvalidNameTests(false/*Don't use replace*/);
  }

  void test_AddOrReplace_With_Name_Containing_Special_Chars_Throws_Invalid_Argument()
  {
    this->doAddingOnInvalidNameTests(true/*Use replace*/);
  }

  void test_Add_Then_Changing_Illegal_Char_List_Only_Affects_Future_Additions()
  {
    // The ADS shouldcurrently accept anything
    const std::string illegalChar(".");
    std::string name = "ContainsIllegal" + illegalChar;
    TS_ASSERT_THROWS_NOTHING(addToADS(name));
    // Ban period characters
    AnalysisDataService::Instance().setIllegalCharacterList(illegalChar);
    // Check we still have the original one
    TS_ASSERT_EQUALS(isInADS(name), true);
    std::string banned = "Also.Contains.Illegal";
    // This should not be allowed now.
    TS_ASSERT_THROWS(addToADS(banned), std::invalid_argument);
    AnalysisDataService::Instance().remove(name);
    // Clear up
    AnalysisDataService::Instance().setIllegalCharacterList("");
  }

  void test_AddOrReplace_Does_Not_Throw_When_Adding_Object_That_Has_A_Name_That_Already_Exists()
  {
    const std::string name("MySpaceAddOrReplace");
    TS_ASSERT_THROWS_NOTHING(addOrReplaceToADS(name));
    TS_ASSERT_THROWS(addToADS(name),std::runtime_error);
    TS_ASSERT_THROWS_NOTHING(addOrReplaceToADS(name));
    TS_ASSERT_THROWS_NOTHING(removeFromADS(name));
  }

  void testRemove()
  {
    const std::string name("MySpace");
    addToADS(name);
    TS_ASSERT_THROWS_NOTHING(removeFromADS(name));
    TS_ASSERT_THROWS(AnalysisDataService::Instance().retrieve(name),std::runtime_error);
    // Remove should not throw but give a warning in the log file, changed by LCC 05/2008
    TS_ASSERT_THROWS_NOTHING(removeFromADS("ttttt"));
  }

  void testRetrieve()
  {
    const std::string name("MySpace");
    Workspace_sptr work = addToADS(name);
    Workspace_sptr workBack;
    TS_ASSERT_THROWS_NOTHING(workBack = AnalysisDataService::Instance().retrieve(name));
    TS_ASSERT_EQUALS(work, workBack);
    //clean up the ADS for other tests
    removeFromADS(name);
  }

  void testRetrieveWS()
  {
    const std::string name("MySpace");
    Workspace_sptr work = addToADS(name);
    MockWorkspace_sptr workBack;
    TS_ASSERT_THROWS_NOTHING(workBack = AnalysisDataService::Instance().retrieveWS<MockWorkspace>(name));
    TS_ASSERT_EQUALS(work, workBack);
    //clean up the ADS for other tests
    removeFromADS(name);
  }

  void test_Rename()
  {
    const std::string oldName = "Old";
    const std::string newName = "New";
    Workspace_sptr work = addToADS(oldName);
    TS_ASSERT_THROWS_NOTHING( AnalysisDataService::Instance().rename(oldName, newName) );
    auto workBack = AnalysisDataService::Instance().retrieve(newName);
    TS_ASSERT_EQUALS(work, workBack);
    TS_ASSERT( ! AnalysisDataService::Instance().doesExist( oldName ) );
    TS_ASSERT( AnalysisDataService::Instance().doesExist( newName ) );
    //clean up the ADS for other tests
    AnalysisDataService::Instance().clear();
  }

  void test_Rename_Overwrites_Existing_WS()
  {
    AnalysisDataService::Instance().clear();
    const std::string oldName = "Old";
    const std::string newName = "New";
    Workspace_sptr work1 = addToADS(oldName);
    Workspace_sptr work2 = addToADS(newName);
    TS_ASSERT_THROWS_NOTHING( AnalysisDataService::Instance().rename(oldName, newName) );
    auto workBack = AnalysisDataService::Instance().retrieve(newName);
    TS_ASSERT_EQUALS(work1, workBack);
    TS_ASSERT( ! AnalysisDataService::Instance().doesExist( oldName ) );
    TS_ASSERT( AnalysisDataService::Instance().doesExist( newName ) );
    TS_ASSERT_EQUALS( AnalysisDataService::Instance().size(), 1 );

    //clean up the ADS for other tests
    AnalysisDataService::Instance().clear();
  }

  void test_add_workspace_group()
  {
    auto& ADS = AnalysisDataService::Instance();
    // create a group
    WorkspaceGroup_sptr group( new WorkspaceGroup );
    // create anonymous workspaces
    MockWorkspace_sptr ws1 = MockWorkspace_sptr(new MockWorkspace);
    MockWorkspace_sptr ws2 = MockWorkspace_sptr(new MockWorkspace);
    // add them to the group
    group->addWorkspace( ws1 );
    group->addWorkspace( ws2 );
    // ADS must be empty
    TS_ASSERT_EQUALS( ADS.size(), 0 );
    ADS.add( "Group", group );
    // there must be 3 workspaces in the ADS
    TS_ASSERT_EQUALS( ADS.size(), 3 );
    TS_ASSERT( ADS.doesExist( "Group" ) );
    TS_ASSERT( ADS.doesExist( "Group_1" ) );
    TS_ASSERT( ADS.doesExist( "Group_2" ) );
    //clean up the ADS for other tests
    AnalysisDataService::Instance().clear();
  }

  void test_add_workspace_group_keeps_existing_workspaces()
  {
    auto& ADS = AnalysisDataService::Instance();
    // populate the ADS
    Workspace_sptr work1 = addToADS("work1");
    Workspace_sptr work2 = addToADS("work2");
    // create a group
    WorkspaceGroup_sptr group( new WorkspaceGroup );
    // create anonymous workspace
    MockWorkspace_sptr ws1 = MockWorkspace_sptr(new MockWorkspace);
    // add one anonymous ...
    group->addWorkspace( ws1 );
    // and one existing workspace
    group->addWorkspace( work2 );
    // ADS must have 2 workspaces
    TS_ASSERT_EQUALS( ADS.size(), 2 );
    ADS.add( "Group", group );
    // there must be 4 workspaces in the ADS
    TS_ASSERT_EQUALS( ADS.size(), 4 );
    TS_ASSERT(   ADS.doesExist( "Group" ) );
    TS_ASSERT(   ADS.doesExist( "Group_1" ) );
    TS_ASSERT( ! ADS.doesExist( "Group_2" ) );
    TS_ASSERT(   ADS.doesExist( "work1" ) );
    TS_ASSERT(   ADS.doesExist( "work2" ) );

    auto names = group->getNames();
    TS_ASSERT_EQUALS( names.size(), 2 );
    TS_ASSERT_EQUALS( names[0], "Group_1" );
    TS_ASSERT_EQUALS( names[1], "work2" );

    //clean up the ADS for other tests
    AnalysisDataService::Instance().clear();
  }

  void test_addOrReplace_workspace_group_replaces_existing_workspaces()
  {
    auto& ADS = AnalysisDataService::Instance();
    // populate the ADS
    Workspace_sptr work1 = addToADS("work1");
    Workspace_sptr work2 = addToADS("Group_2");
    // create a group
    WorkspaceGroup_sptr group( new WorkspaceGroup );
    // create anonymous workspace
    MockWorkspace_sptr ws1 = MockWorkspace_sptr(new MockWorkspace);
    MockWorkspace_sptr ws2 = MockWorkspace_sptr(new MockWorkspace);
    // add them
    group->addWorkspace( ws1 );
    group->addWorkspace( ws2 );
    // ADS must have 2 workspaces
    TS_ASSERT_EQUALS( ADS.size(), 2 );
    ADS.addOrReplace( "Group", group );
    // there must be 4 workspaces in the ADS
    TS_ASSERT_EQUALS( ADS.size(), 4 );
    TS_ASSERT(   ADS.doesExist( "Group" ) );
    TS_ASSERT(   ADS.doesExist( "Group_1" ) );
    TS_ASSERT(   ADS.doesExist( "Group_2" ) );
    TS_ASSERT(   ADS.doesExist( "work1" ) );
    TS_ASSERT( ! ADS.doesExist( "work2" ) );

    auto names = group->getNames();
    TS_ASSERT_EQUALS( names.size(), 2 );
    TS_ASSERT_EQUALS( names[0], "Group_1" );
    TS_ASSERT_EQUALS( names[1], "Group_2" );

    //clean up the ADS for other tests
    AnalysisDataService::Instance().clear();
  }

  void test_add_workspace_group_throws_if_adding_existing_names()
  {
    auto& ADS = AnalysisDataService::Instance();
    // populate the ADS
    Workspace_sptr work1 = addToADS("work1");
    Workspace_sptr work2 = addToADS("Group_2");
    // create a group
    WorkspaceGroup_sptr group( new WorkspaceGroup );
    // create anonymous workspace
    MockWorkspace_sptr ws1 = MockWorkspace_sptr(new MockWorkspace);
    MockWorkspace_sptr ws2 = MockWorkspace_sptr(new MockWorkspace);
    // add them
    group->addWorkspace( ws1 );
    group->addWorkspace( ws2 );
    // ADS must have 2 workspaces
    TS_ASSERT_EQUALS( ADS.size(), 2 );
    TS_ASSERT_THROWS( ADS.add( "Group", group ), std::runtime_error );
    // there must be 4 workspaces in the ADS
    TS_ASSERT_EQUALS( ADS.size(), 4 );
    TS_ASSERT(   ADS.doesExist( "Group" ) );
    TS_ASSERT(   ADS.doesExist( "Group_1" ) );
    TS_ASSERT(   ADS.doesExist( "Group_2" ) );
    TS_ASSERT(   ADS.doesExist( "work1" ) );
    TS_ASSERT( ! ADS.doesExist( "work2" ) );

    auto names = group->getNames();
    TS_ASSERT_EQUALS( names.size(), 2 );
    TS_ASSERT_EQUALS( names[0], "Group_1" );
    TS_ASSERT_EQUALS( names[1], "Group_2" );

    //clean up the ADS for other tests
    AnalysisDataService::Instance().clear();
  }

private:

  /// If replace=true then usea addOrReplace
  void doAddingOnInvalidNameTests(bool replace)
  {
    const std::string illegalChars = " +-/*\\%<>&|^~=!@()[]{},:.`$'\"?";
    AnalysisDataService::Instance().setIllegalCharacterList(illegalChars);
    const size_t nchars(illegalChars.size());
    const std::string allowed("WsName");

    for( size_t i = 0; i < nchars; ++i )
    {
      //Build illegal name
      std::ostringstream name;
      name << allowed << illegalChars[i] << allowed << illegalChars[i] << allowed;
      // Add it
      std::ostringstream errorMsg;
      errorMsg << "Name containing illegal character " << illegalChars[i] << " is not allowed but ADS did not throw.";
      if( replace )
      {
        TSM_ASSERT_THROWS(errorMsg.str(), addToADS(name.str()), std::invalid_argument);
      }
      else
      {
        TSM_ASSERT_THROWS(errorMsg.str(), addOrReplaceToADS(name.str()), std::invalid_argument);
      }
      bool stored = isInADS(name.str());
      TS_ASSERT_EQUALS(stored, false);
      if( stored ) removeFromADS(name.str()); // Clear up if the test fails so that it dones't impact on others.
    }
    // Clean up
    AnalysisDataService::Instance().setIllegalCharacterList("");
  }

  /// Add a ptr to the ADS with the given name
  Workspace_sptr addToADS(const std::string & name)
  {
    MockWorkspace_sptr space = MockWorkspace_sptr(new MockWorkspace);
    AnalysisDataService::Instance().add(name, space);
    return space;
  }

  /// Add or replace the given name
  void addOrReplaceToADS(const std::string & name)
  {
    AnalysisDataService::Instance().addOrReplace(name, Workspace_sptr(new MockWorkspace));
  }

  void removeFromADS(const std::string & name)
  {
    AnalysisDataService::Instance().remove(name);
  }
  
  bool isInADS(const std::string & name)
  {
    return AnalysisDataService::Instance().doesExist(name);
  }

};

#endif /*ANALYSISDATASERVICETEST_H_*/
