#ifndef SCRIPTREPOSITORYIMPLTEST_H_
#define SCRIPTREPOSITORYIMPL_H_

#include <cxxtest/TestSuite.h>
#include "MantidScriptRepository/ScriptRepositoryImpl.h"
#include <Poco/File.h>
#include <Poco/FileStream.h>
#include <Poco/TemporaryFile.h>
#include <algorithm>

using namespace std; 
/** These tests do no depend on the internet connection*/
/**ctest -j8 -R ScriptRepositoryTestImpl_  --verbose*/
using Mantid::API::ScriptRepositoryImpl;

class ScriptRepositoryImplLocal : public ScriptRepositoryImpl{
 public:
 ScriptRepositoryImplLocal(std::string a="", std::string b=""):
  ScriptRepositoryImpl(a,b){}
};

const std::string repositoryjson = "{\n"
    "\"TofConv\":\n"
    "{\n"
     " \"pub_date\": \"2012-02-13 10:00:50\",\n"
     " \"description\": \"the description\",\n"
     " \"directory\": true \n"
    "},\n"
    "\"TofConv/README.txt\":\n"
    "{\n"
 	  " \"pub_date\": \"2012-02-13 10:02:50\",\n"
	  " \"description\": \"tofconv description\",\n"
	  " \"directory\": false \n"
    "},\n"
    "\"TofConv/TofConverter.py\":\n"
    "{\n"
	  "  \"pub_date\": \"2012-02-10 10:00:50\",\n"
	  "  \"description\": \"tofconverter description\",\n"
	  "  \"directory\": false	\n"
    "},\n"
    "\"reflectometry\":\n"
    "{\n"
    "	\"pub_date\": \"2012-01-13 10:00:50\",\n"
	  "  \"directory\": true	\n"
    "},\n"
    "\"reflectometry/Quick.py\":\n"
    "{\n"
  	"  \"pub_date\": \"2012-02-13 10:00:00\",\n"
    "  \"description\": \"quick description\",\n"
	  "\"directory\": false	\n"
    "}\n"
  "}\n";


const std::string webserverurl = "http://localhost";

class ScriptRepositoryTestImpl : public CxxTest::TestSuite{
  ScriptRepositoryImplLocal * repo;
  std::string local_rep;
 public: 
  static ScriptRepositoryTestImpl * createSuite(){return new ScriptRepositoryTestImpl(); }
  static void destroySuite (ScriptRepositoryTestImpl * suite){delete suite; }

  // ensure that all tests will be perfomed in a fresh repository
  void setUp(){    
    using Poco::TemporaryFile; 
    TemporaryFile temp_f; 
    local_rep = temp_f.path(); 
    TS_ASSERT_THROWS_NOTHING(repo = new ScriptRepositoryImplLocal(local_rep, webserverurl)); 
  }
  
  // ensure that the local files are free from the test created.
  void tearDown(){
    delete repo; 
    try{
      Poco::File f(local_rep); 
      f.remove(true);
    }catch(Poco::Exception & ex){
      TS_WARN(ex.displayText());
    }
  }


  /**
     Testing the installation of the Repository Service:
     
     The normal test, it should be able to create the new folder and put inside the 
     repository.json and local.json files. 
   */
  void test_normal_installation_procedure(){
    // before installing the repository, ScriptRepositoryImpl will be always invalid
    TSM_ASSERT("Why valid?",!repo->isValid()); 
    // the installation should throw nothing
    TSM_ASSERT_THROWS_NOTHING("Installation should not throw",repo->install(local_rep)); 
    // the repository must be valid
    TSM_ASSERT("Now should be valid!",repo->isValid());
    // checking that repository.json and local.json exists
    {
      Poco::File l(std::string(local_rep).append("/.repository.json")); 
      TSM_ASSERT("Failed to create repository.json", l.exists()); 
      Poco::File r(std::string(local_rep).append("/.local.json")); 
      TSM_ASSERT("Failed to create local.json", r.exists()); 
    }

    // after the installation, all the others instances of ScriptRepositoryImpl should be valid, 
    // by geting the information from the ScriptRepository settings.
    ScriptRepositoryImplLocal * other = new ScriptRepositoryImplLocal(); 
    TSM_ASSERT("All the others should recognize that this is a valid repository",other->isValid()); 
    delete other; 

  }

  /** 
      Installation should not install on a non-empty directory. 
   */
  void test_installation_do_not_install_on_non_empty_directory(){
    // fill the local_rep path with files
    Poco::File d(local_rep); 
    d.createDirectories(); 
    Poco::FileStream f(std::string(local_rep).append("/myfile")); 
    f << "nothing"; 
    f.close(); 
    // now, local_rep is not empty!

    // before installing the repository, ScriptRepositoryImpl will be always invalid
    TSM_ASSERT("Why valid?",!repo->isValid()); 
    // the installation should throw, directory is not empty    
    TS_ASSERT_THROWS(repo->install(local_rep), Mantid::API::ScriptRepoException); 
  }


  /**
     List Files must list all the files at central repository
   */
  void test_listFiles_must_list_all_files_at_central_repository(){
    const char * test_entries []= {"TofConv",
                                 "TofConv/README.txt",
                                 "TofConv/TofConverter.py",
                                 "reflectometry",
                                 "reflectometry/Quick.py"};
    
    TS_ASSERT_THROWS_NOTHING(repo->install(local_rep)); 
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());
    // check that all the files at the central repository are inside
    for (int i = 0; i< 5; i++)
      TSM_ASSERT_THROWS_NOTHING(test_entries[i],repo->info(test_entries[i])); 
    
    }

  void test_listFiles_must_list_all_local_files(){
    // will create the folder
    TS_ASSERT_THROWS_NOTHING(repo->install(local_rep)); 
    
    // creating a file to test the list Files
    std::string local_file = std::string(local_rep).append("/myfile");
    Poco::FileStream f(local_file);    
    f << "nothing"; 
    f.close(); 

    // 
    std::vector<std::string > files;
    TS_ASSERT_THROWS_NOTHING(files = repo->listFiles()); 
    
    for (std::vector<std::string>::iterator it = files.begin(); 
         it != files.end();
         it++)
      std::cout << "Files listed: " << *it << std::endl; 
    
    // checking that the local_file was listed in listFiles.
    TSM_ASSERT_THROWS_NOTHING(local_file, repo->info("myfile"));  
    // MUST ACCEPT AN ABSOLUTE PATH AS WELL
    TSM_ASSERT_THROWS_NOTHING(local_file, repo->info(local_file));
  }


  void test_info_correctly_parses_the_repository_json(){
    using Mantid::API::ScriptInfo;
    TS_ASSERT_THROWS_NOTHING(repo->install(local_rep)); 
    TS_ASSERT_THROWS_NOTHING(repo->listFiles()); 
    ScriptInfo information = repo->info("TofConv/TofConverter.py"); 
    TS_ASSERT(information.description == "tofconverter description");
    TS_ASSERT(information.author.empty()); 
    TSM_ASSERT("check time", information.pub_date == DateAndTime("2012-02-10 10:00:50")); 
    TS_ASSERT(!information.auto_update);
  }

  void test_download_new_files_from_repository(){
    TS_ASSERT_THROWS_NOTHING(repo->install(local_rep)); 
    TS_ASSERT_THROWS_NOTHING(repo->listFiles()); 
    TS_ASSERT_THROWS_NOTHING(repo->download("TofConv/README.txt")); 
    
  }



};

#endif
