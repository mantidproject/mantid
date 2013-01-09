#ifndef GITSCRIPTREPOSITORYTEST_H_
#define GITSCRIPTREPOSITORYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidScriptRepository/GitScriptRepository.h"
#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/FileStream.h>
#include <algorithm>
using Poco::File; 
using Poco::FileOutputStream;
using Mantid::API::GitScriptRepository; 
using Mantid::API::ScriptRepoException;
using namespace std; 
/** These tests do no depend on the internet connection*/
/**ctest -j8 -R GitScriptRepositoryTest_GitScript  --verbose*/




class GitScriptRepositoryTest : public CxxTest::TestSuite{
  GitScriptRepository * repo; 
 public: 
  static GitScriptRepositoryTest * createSuite(){return new GitScriptRepositoryTest(); }
  static void destroySuite (GitScriptRepositoryTest * suite){delete suite; }

  void setUp(){
    TS_ASSERT_THROWS_NOTHING(repo = new GitScriptRepository()); 
  }
  
  void tearDown(){
    delete repo; 
  }


  void create_file(std::string path, std::string content){

    // It must list the local files as well as remote files
    std::string repo_path = repo->localRepository(); 
    if (path.find('/') != string::npos){
      // create the sub directories
      std::string new_dir = repo_path;     
      new_dir.append("/"); 
      new_dir.append(path);
      // remote the file part
      size_t found; 
      found = new_dir.rfind('/');       
      new_dir.replace(found, new_dir.size()-found, ""); 

      File f_dir(new_dir); 
      f_dir.createDirectories();
    }
    
    std::string new_path = repo_path; 
    new_path.append("/"); 
    new_path.append(path); 
    { // create file

      File f_file(new_path); 
      f_file.createFile(); 

    }
    
    FileOutputStream out(new_path, std::ios::out); 
    out << content; 
    out.close(); 

  }

void delete_file(std::string path){
  std::string new_path = repo->localRepository();
  new_path.append("/"); 
  new_path.append(path); 
  File f(new_path);

  f.remove(true); 

}
 


  
  void test_List_All_Remote_Files_Inside_Repository(){
    // it must show the files and folders!

    vector<string> files = repo->listFiles(); 
    
    
    const string files_inside_repository [] = {
      (const char*)"TofConv/README.txt",
      (const char *)"reflectometry",
      (const char *)"development/diffraction",
      (const char *)"largescalestructures/offspec/Larmor_Detector_Map_File_2.xml",
      (const char *)"inelastic/user/javier_250mev.py",
      (const char *)"inelastic",
      (const char *)"inelastic/user",
    };

    //print 
    if (false){
      for (vector<string>::iterator it = files.begin();
           it != files.end(); it++)
        cout << "f: " << *it << endl;     
    }
    
    for (int i = 0; i<7; i++)
    {
      TSM_ASSERT(files_inside_repository[i],
                 find(files.begin(),files.end(),files_inside_repository[i]) != files.end()); 
    }

    // comment the return in order to print the values

   
  }

  void test_List_All_Local_Files_Inside_Repository(){
    // It must list the local files as well as remote files
    const char * content = "#!/bin/bash\n# -*- coding: utf-8 -*-\n'''\nusage: ...\n'''\n"; 
    create_file("newfolder/newfile.py",content); 

    // it must show the files and folders!
    vector<string> files = repo->listFiles(); 
    TS_ASSERT(find(files.begin(),files.end(),"newfolder/newfile.py") != files.end()); 
    TS_ASSERT(find(files.begin(),files.end(),"newfolder") != files.end()); 

    delete_file("newfolder"); 

  }


  void test_Should_Not_Show_pyc_files(){
    const string filename = (const char *)"myfile.pyc";
    create_file(filename,"binaryfile\n"); 
    vector<string> files = repo->listFiles(); 
    // should not be list
    TS_ASSERT(find(files.begin(),files.end(),filename) == files.end());
    delete_file(filename);                   
  }

  void test_download_file(){
    std::string file_to_download = "TofConv/TofConverter.py"; 
    std::string repo_file = repo->localRepository(); 
    repo_file.append("/"); 
    std::cout << "Trying to download file " << file_to_download<<endl; 
    TS_ASSERT_THROWS_NOTHING(repo->download(file_to_download)); 

    Poco::File f(std::string(repo_file).append(file_to_download)); 
    TSM_ASSERT(f.path(), f.exists()); 

    delete_file("TofConv"); 
  }


  void test_download_directory(){
    // directory should be downloaded recursively
    std::string dir_to_download = "TofConv"; 
    std::string repo_file = repo->localRepository(); 
    repo_file.append("/"); 

    TS_ASSERT_THROWS_NOTHING(repo->download(dir_to_download)); 

    const std::string to_test [] = {
      "TofConv/README.txt",
      "TofConv/TofConverter.py",
      "TofConv/TofConverter",
      "TofConv/TofConverter/converter.ui",
    };
    for (int i=0; i<4; i++){
      File f(std::string(repo_file).append(to_test[i]));
      TSM_ASSERT(f.path(), f.exists()); 
             
    }
    
    delete_file("TofConv"); 


  }


  void test_must_be_able_to_show_file_is_updated(){

    using Mantid::API::BOTH_UNCHANGED; 
    using Mantid::API::LOCAL_CHANGED;
    using Mantid::API::REMOTE_CHANGED;
    std::string file_name = "TofConv/README.txt";

    TS_ASSERT_THROWS_NOTHING(repo->download(file_name));
    Poco::File f(std::string(repo->localRepository()).append("/").append(file_name));
    TS_ASSERT(f.exists()); 
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());
    
    
    // test update    
    TSM_ASSERT("Must show file is updated given relative path",repo->fileStatus(file_name)==BOTH_UNCHANGED);

    std::string abs_path = std::string(repo->localRepository()).append("/").append(file_name);
    
    TSM_ASSERT( abs_path.c_str(),
               repo->fileStatus(abs_path)==BOTH_UNCHANGED);
    
    // Change the file
    {      
      FileOutputStream out(abs_path,std::ios::out|std::ios::app); 
      out << "something new\n";
      out.close(); 
    }
    
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());
    
    // file not updated anymore
    std::cout << "check status " << repo->fileStatus(file_name)  << "lcoal changed " << LOCAL_CHANGED<< endl;
    TS_ASSERT(repo->fileStatus(file_name) & LOCAL_CHANGED); 
    TSM_ASSERT("Must identify that file is changed locally",
               repo->fileStatus(file_name) == LOCAL_CHANGED);

    
    // throws exception for invalid path
    TSM_ASSERT_THROWS("Throw exception for invalid path",
                      repo->fileStatus("/tmp/thisisaninvalidpath"),
                      ScriptRepoException);


    // test directory inside the repository
    // changed if at least one has being modified
    ///@todo test directories status
    /// It should test that the status of the directories changes 
    /// according to the status of the files inside.

    ///@todo test remote file changed.

    delete_file("TofConv"); 
  }

  /*
  void tes_shall_display_the_file_commits(){
    std::string file_name = "TofConv/TofConverter.py"; 
    TS_ASSERT_THROWS_NOTHING(repo->download(file_name));

    //   repo->history(); 
  }
  */

  void test_shall_giveback_file_info(){
    
    using Mantid::API::ScriptInfo;
    std::string file_name = "TofConv/TofConverter.py"; 
    TS_ASSERT_THROWS_NOTHING(repo->listFiles()); 


    {  
      // get info from remote only file
      ScriptInfo info = repo->fileInfo(file_name); 
      TS_ASSERT(info.description.empty()); 
      // ensure it removes directories that it has created
      Poco::File f(std::string(repo->localRepository()).append("/").append("TofConv")); 
      TS_ASSERT(!f.exists()); 
    }

    {
      // get info from local files
    TS_ASSERT_THROWS_NOTHING(repo->download("reflectometry/Quick.py")); 
    ScriptInfo info = repo->fileInfo("reflectometry/Quick.py"); 
    TS_ASSERT(info.description == "ISIS reflectometry reduction script."); 
    delete_file("reflectometry"); 
    }


    {// get info from modules
      TS_ASSERT_THROWS_NOTHING(repo->download("TofConv/TofConverter")); 
      ScriptInfo info = repo->fileInfo("TofConv/TofConverter"); 
      TS_ASSERT(info.description.empty());
      delete_file("TofConv");  
    }

    {// get info from folders
      TS_ASSERT_THROWS_NOTHING(repo->download("TofConv")); 
      ScriptInfo info = repo->fileInfo("TofConv"); 
      TS_ASSERT(info.description == "This has been adopted into the main Mantid repository as a Mantid interface. TofConverter from version 2.2.");                 
      delete_file("TofConv"); 
    } 
  }
  
};

#endif
