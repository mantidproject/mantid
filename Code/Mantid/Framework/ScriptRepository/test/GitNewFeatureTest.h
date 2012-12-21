#ifndef GITNEWFEATURE_H_
#define GITNEWFEATURE_H_

#include <cxxtest/TestSuite.h>
#include "MantidScriptRepository/GitScriptRepository.h"
#include <Poco/Path.h>
using namespace std; 
using Mantid::API::GitScriptRepository; 
using Mantid::API::ScriptRepoException;

/**
   These tests requires connection to the internet.
 */
class GitNewFeature : public CxxTest::TestSuite{

 public: 
  static GitNewFeature * createSuite(){return new GitNewFeature(); }
  static void destroySuite (GitNewFeature * suite){delete suite; }
  GitScriptRepository * repo; 
  void setUp(){

  }
  
  void tearDown(){
  }

  void test_clone_git_transport(){
    // WHEN THIS TEST PASS, we will be able to have upload directly. 
    const char * ssh_transport = "git@github.com:mantidproject/scripts.git";
    std::string local_rep = Poco::Path::temporary().append("/sshgitrepo"); 
    repo = NULL;
    TS_ASSERT_THROWS_NOTHING(repo = new GitScriptRepository(ssh_transport,
	                        local_rep));
    Poco::File f(local_rep);  
    f.remove(true); 
  
  delete repo; 
  }
};

#endif
