#ifndef GITNEWFEATURE_H_
#define GITNEWFEATURE_H_

#include <cxxtest/TestSuite.h>
#include "MantidScriptRepository/GitScriptRepository.h"
#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/TemporaryFile.h>
using namespace std; 
using Mantid::API::GitScriptRepository; 
using Mantid::API::ScriptRepoException;

/**
   These tests requires connection to the internet.
 */
class GitNewFeatureTest : public CxxTest::TestSuite{

 public: 
  static GitNewFeatureTest * createSuite(){return new GitNewFeatureTest(); }
  static void destroySuite (GitNewFeatureTest * suite){delete suite; }
  GitScriptRepository * repo; 
  void setUp(){

  }
  
  void tearDown(){
  }

  void test_clone_git_transport(){
    // WHEN THIS TEST PASS, we will be able to have upload directly. 
    const char * ssh_transport = "git@github.com:mantidproject/scripts.git";    
    std::string local_rep = Poco::TemporaryFile::tempName(); 
    repo = NULL;
    TS_ASSERT_THROWS_NOTHING(repo = new GitScriptRepository(ssh_transport,
	                        local_rep));
    TS_ASSERT_THROWS_NOTHING(repo->update()); 

    Poco::File f(local_rep);  
    if (f.exists())
      f.remove(true); 
  
  delete repo; 
  }
};

#endif
