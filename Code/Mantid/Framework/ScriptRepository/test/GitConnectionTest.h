#ifndef GITCONNECTIONTEST_H_
#define GITCONNECTIONTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidScriptRepository/GitScriptRepository.h"
#include <Poco/File.h>
#include <Poco/FileStream.h>
#include <iostream>
using namespace std; 
using Poco::File; 
using Mantid::API::GitScriptRepository; 
using Mantid::API::ScriptRepoException;

/**
   These tests requires connection to the internet.
 */
class GitConnectionTest : public CxxTest::TestSuite{

 public: 
  static GitConnectionTest * createSuite(){return new GitConnectionTest(); }
  static void destroySuite (GitConnectionTest * suite){delete suite; }
  GitScriptRepository * repo; 
  void setUp(){

  }
  
  void tearDown(){
    cout << "tear down" << endl; 
    delete repo; 
  }

  void test_UpdateNewRepositoryMustCloneRepository(){
    const char * newpath = "/tmp/newrep"; 
    TS_ASSERT_THROWS_NOTHING(repo = new GitScriptRepository(newpath));
    if (repo){
      TS_ASSERT_THROWS_NOTHING(repo->update());
      // check that the path was created
      Poco::File p(newpath);      
      TS_ASSERT(p.exists()); 
      p.remove(true); 
    }                             
  }


  void test_NormalUpdateOperationShouldCloneOrUpdateRepository(){
    TS_ASSERT_THROWS_NOTHING(repo = new GitScriptRepository());
    TS_ASSERT(repo); 
    TS_ASSERT_THROWS_NOTHING(repo->update());
  }


  void test_UploadingNewFiles(){
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
