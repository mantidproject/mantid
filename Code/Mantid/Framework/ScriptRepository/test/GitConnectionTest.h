#ifndef GITCONNECTIONTEST_H_
#define GITCONNECTIONTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidScriptRepository/GitScriptRepository.h"
#include <Poco/File.h>
#include <Poco/FileStream.h>
#include <Poco/TemporaryFile.h>
#include <Poco/Path.h>
#include <iostream>
using namespace std; 
using Poco::File; 
using Mantid::API::GitScriptRepository; 
using Mantid::API::ScriptRepoException;

/**
   These tests requires connection to the internet.
make -j4 GitScriptRepositoryTest
ctest -j8 -R GitScriptRepositoryTest_GitConnection  --verbose
 */
class GitConnectionTest : public CxxTest::TestSuite{

 public: 
  static GitConnectionTest * createSuite(){return new GitConnectionTest(); }
  static void destroySuite (GitConnectionTest * suite){delete suite; }
  GitScriptRepository * repo; 
  void setUp(){
    cout << "setup\n"; 
  }
  
  void tearDown(){
    cout << "tear down\n" ; 
    delete repo; 
  }

  void test_UpdateNewRepositoryMustCloneRepository(){
    cout << "test Update new repository must clone repository\n";     
    std::string file_path_name = Poco::TemporaryFile::tempName(); 
    //const string newpath = Poco::Path::temp().append("newrep"); 
    // testing with a very small repository to be fast... 
    TS_ASSERT_THROWS_NOTHING(repo = new GitScriptRepository(file_path_name, "git://github.com/gesnerpassos/GPWorks.git")); 
    if (repo){
      TS_ASSERT_THROWS_NOTHING(repo->update());
      // check that the path was created
      try{
        Poco::File p(file_path_name);
        TSM_ASSERT(file_path_name, p.exists()); 
        Poco::TemporaryFile::registerForDeletion(file_path_name); 
      }
      catch(Poco::Exception & ex){
        std::cerr << "Poco Exception: " << ex.className() << ": " << ex.message() << "\n"; 
      }
    }
    else{
      TSM_ASSERT("Repo not created!",false); 
    }
  }


  void test_NormalUpdateOperationShouldCloneOrUpdateRepository(){
    TS_ASSERT_THROWS_NOTHING(repo = new GitScriptRepository());
    TS_ASSERT(repo); 
    TS_ASSERT_THROWS_NOTHING(repo->update());
  }


  void tes_UploadingNewFiles(){
    TS_ASSERT_THROWS_NOTHING(repo = new GitScriptRepository()); 
    std::string file_path = repo->localRepository().append("/mynewfile.py"); 
    Poco::FileStream newfile(file_path, std::ios::out | std::ios::app);
    newfile << "Receive new Information\n" ; 
    newfile.close(); 
    TS_ASSERT_THROWS_NOTHING(repo->upload(file_path, "No comment", "gesner", "No description")); 
    cout << "Uploading new files done" << endl; 
    Poco::File p(file_path); 
    TS_ASSERT(p.exists()); 
    p.remove(true); 

  }

  void test_CloneThrowsExceptionForInvalidRepositoryPath(){
    TS_ASSERT_THROWS_NOTHING(repo = new GitScriptRepository("/tmp/nothing", "git://github.com/mantidproject/WRONGscripts.git")); 
    TS_ASSERT_THROWS(repo->update(), ScriptRepoException);

  }


};

#endif
