#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp> 

#include "Poco/Path.h"
#include "Poco/File.h"
#include "Poco/FileStream.h"
#include "Poco/URI.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/StreamCopier.h"
#include "Poco/Net/HTTPCookie.h"
#include "Poco/Net/HTMLForm.h"
#include "Poco/Net/FilePartSource.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/NameValueCollection.h"
#include "Poco/NullStream.h"
#include "Poco/Exception.h"

#include "MantidScriptRepository/GitScriptRepository.h"
#include "MantidAPI/ScriptRepositoryFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/FacilityInfo.h"
#include "git2.h"
#include "assert.h"
#include <locale.h>
#ifdef _WIN32
#define snprintf _snprintf
#endif

using std::vector;
using std::string;

using Poco::Path; 
using Poco::FileStream; 
using Poco::FileInputStream;
using Poco::URI;
using Poco::Net::HTTPClientSession;
using Poco::Net::HTTPRequest;
using Poco::Net::HTTPCookie;
using Poco::Net::HTMLForm;
using Poco::Net::FilePartSource;
using Poco::Net::HTTPMessage;
using Poco::Net::HTTPResponse;
using Poco::Net::NameValueCollection;
using Poco::Exception;

using Poco::StreamCopier;

using Mantid::Kernel::ConfigServiceImpl; 
using Mantid::Kernel::ConfigService;
using Mantid::Kernel::Logger;


namespace Mantid
{
namespace API
{
  DECLARE_SCRIPTREPOSITORY(GitScriptRepository)
  
  GitScriptRepository::GitScriptRepository(const std::string local_rep, 
                                         const std::string remote) :
  g_log(Logger::get("GitScriptRepository"))
  {    
    g_log.debug() << "GitScriptRepository constructor: local_rep " 
                  << local_rep << "; remote = " << remote << "\n"; 
    update_called = false;  
    std::string loc, rem; 
    if (local_rep.empty() || remote.empty()){
      ConfigServiceImpl & config = ConfigService::Instance();
      loc = config.getString("ScriptLocalRepository"); 
      rem = config.getString("ScriptRepository");     
    }else{
      local_repository = local_rep; 
      remote_url = remote; 
    }
    
    if (local_rep.empty())
      local_repository = loc; 
    else
      local_repository = local_rep; 
    
    if (remote.empty())
      remote_url = rem; 
    else
      remote_url = remote;
    
    if (remote_url.empty()){
      g_log.error() << "Failed to find the specification for the remote url and local repository\n"; 
      
      throw ScriptRepoException(
                                "We failed to get correct definition of the path for the local repository",
                                "Attempt to construct GitScriptRepository with invalid inputs");                              
    }
    
    repo = NULL; 
    
    if (local_repository.empty())
      return;
    
    //parsing the local_repository
    Poco::Path local(local_repository); 
    std::string aux_local_rep;
    if (local.isRelative()){
      aux_local_rep = std::string(Poco::Path::home()).append(local_repository);
      local_repository = aux_local_rep;
    }
     g_log.debug() << "GitScriptRepository::Constructor Configuring repository for local " << local_repository  
                  << "\n"; 
    // it will try to initialize the git_repository
    // It is possible to have a non valid git_repository
    // when there is no local repository. But, this means, 
    // that it will accept only the update method.
    
    int err = git_repository_open(&repo, local_repository.c_str());
    if (err){
      const git_error *git_err = giterr_last();
      g_log.warning() << "ScriptRepository not installed in this machine."<< std::endl;
      g_log.debug() << "Invalid path detected in the GitScriptRepository constructor '" << local_repository 
                      << "'.\n Git Error: "
                      <<git_err->message                  
                      << "\n";
      // repo should be NULL in this case
      assert(!repo);
    } 

  }


  GitScriptRepository::~GitScriptRepository() throw() {

    if (repo)
      git_repository_free(repo);
    repo = NULL; 

     
  }



  ScriptRepoException GitScriptRepository::invalidRepository(){
    char information [200]; 
    snprintf(information, 200, "The configuration of your repository is not valid.\n Hint: Check if the folder '%s' exists. If not, execute the method update first.",local_repository.c_str()); 

    g_log.error() << "Failure: " << information << "\n"; 
    return ScriptRepoException(information);
  }


  ScriptRepoException GitScriptRepository::gitException(const std::string info, const std::string file, int line){
    const git_error *err = giterr_last();
    std::string desc; 
    switch(err->klass){
    case GITERR_OS:
      desc = "Internet failure. It may be the internet connection or proxy setting problem\n";      
    break;
    case GITERR_REPOSITORY:
      desc = "The definition of the working directory path is invalid. Check the value of ScriptLocalRepository at the properties file.\n";
      break;
    default:
      break;
    }
    desc.append(err->message);

    g_log.error() << "Failure: "<< desc  << "\n"<< "Info " << info << " -> Error Code ("<< err->klass << ")\n)" ; 
    return ScriptRepoException(info, desc, 
                               file, line);
  }


  /**Auxiliar call back function used to list all files inside a 
     repository.
   
     This function will be called automatically from the git_status_foreach_ext and will pass the following arguments: 
   
     @file : path of the file 
     @status: git_status_t of the file
     @payload: pointer to the struct repo_iteration;

     The conversion from git_status_t and SCRIPTSTATUS is as follows: 
     - REMOTE_ONLY: GIT_STATUS_WT_DELETED or GIT_STATUS_INDEX_NEW
     - LOCAL_ONLY: GIT_STATUS_WT_NEW
     - REMOTE_CHANGED: GIT_STATUS_INDEX_MODIFIED
     - LOCAL_CHANGD: GIT_STATUS_WT_MODIFIED
     - BOTH_CHANGED: GIT_STATUS_WT_MODIFIED && GIT_STATUS_INDEX_MODIFIED
     - BOTH_UNCHANGED: GIT_STATUS_CURRENT

     The call back will call only for the files, the directories must be 
     extracted from the files themselves. 
  */
  static int auxiliar_list_files_cb(const char * file, 
                                    unsigned int status, 
                                    void * payload)
  {
    GitScriptRepository::repo_iteration & repo_iteration = *(static_cast< GitScriptRepository::repo_iteration* >(payload));  

    std::string curr_directory; 
    
    // get the path of the current directory
    using boost::algorithm::split;
    using boost::algorithm::is_any_of;
    using boost::algorithm::join;           
    vector<std::string> strs;
    std::string str = file;
    //split the full path
    // /home/user/mantid/myfile
    // will be splited in : home user mantid myfile
    split(strs, str, boost::is_any_of("/\\"));
    
    if (strs.size()<= 1){
      curr_directory = "";
    }else{
      //remote the current file name
      strs.pop_back(); 
      curr_directory = join(strs,"/"); 
    }
    

    //check if the directory changed 
    std::string ancestor_dir; 
    vector<std::string> ancestor_parts; 
    if (repo_iteration.last_directory.find(curr_directory) != 0){

      // look for the common ancestor: 
      unsigned int index; 
      for (index = 0; index<strs.size(); index++){
        ancestor_parts.push_back(strs[index]);
        ancestor_dir = join(ancestor_parts,"/");
        if (repo_iteration.last_directory.find(ancestor_dir) == 0)
          continue;
        else
          break; 
      }
      //std::cout << "auxiliar: file " << file << " directory : " << curr_directory  << " index = " << index<< std::endl;       
      // now, add the directories
      do{
        GitScriptRepository::file_entry directory; 
        directory.path = ancestor_dir; 
        directory.directory = true; 
        directory.status = REMOTE_ONLY;
        // std::cout << "insert directory " << ancestor_dir << std::endl; 
        repo_iteration.repository_list->push_back(directory);
        index++; 
        if (index >= strs.size())
          break;
        ancestor_parts.push_back(strs[index]); 
        ancestor_dir = join(ancestor_parts,"/"); 
      }while(true); 
    }
    // update the directory
    repo_iteration.last_directory = curr_directory;

    // create the entry for the file: 
    GitScriptRepository::file_entry curfile; 
    curfile.status = BOTH_UNCHANGED; 
    curfile.path = file;
    if ((status & GIT_STATUS_WT_DELETED) ||
        (status & GIT_STATUS_INDEX_NEW) || 
        (status & GIT_STATUS_INDEX_DELETED))
      curfile.status = REMOTE_ONLY; 
    else if (status &  GIT_STATUS_WT_NEW)
      curfile.status = LOCAL_ONLY;
    else if (status == (GIT_STATUS_INDEX_MODIFIED| GIT_STATUS_WT_MODIFIED))
      curfile.status = BOTH_CHANGED;
    else if  (status & GIT_STATUS_INDEX_MODIFIED)
      curfile.status = REMOTE_CHANGED; 
    else if (status & GIT_STATUS_WT_MODIFIED)
      curfile.status = LOCAL_CHANGED;
    else if ( ! status) // GIT_STATUS_CURRENT
      curfile.status = BOTH_UNCHANGED;
    else
      curfile.status = BOTH_UNCHANGED;

    curfile.directory = false; 

    //insert at list
    repo_iteration.repository_list->push_back(curfile); 

    return 0; // so the foreach continues to loop  
  }

  /** For description of this method, please consult ScriptRepository::listFiles. 
    
      It will fill up the repository_list variable in order to provide information about the status of the file as well.

      In order to list all the values from the repository, it uses the 
      method git_status_foreach_ext wich Gather file status information and run callbacks as requested.

      The only limitation of the git_status_foreach_ext is that it ignores
      empty folders, which means, that empty folders will be hidden when listing the files. 

      git_status_foreach_ext calls a callback function to process each 
      entry. This will be done through the auxiliar_list_files_cb .

      @attention Empty folders will not be listed!

  */
  vector<std::string> GitScriptRepository::listFiles() {
    // this method requires a valid local repository.
    if (!repo)
      throw invalidRepository();

    // first of all, clear the repository_list values
    repository_list.clear(); 
    //configure the repo_iteration
    GitScriptRepository::repo_iteration repo_iteration;
    repo_iteration.last_directory = ""; 
    repo_iteration.repository_list = &repository_list; 
  

    // configure the options for the git_status_foreach
    git_status_options opts;   
    memset(&opts,0,sizeof(opts)); 
    opts.version = GIT_STATUS_OPTIONS_VERSION;
  
    // wich files to include
    opts.flags = (GIT_STATUS_OPT_INCLUDE_UNTRACKED|
                  GIT_STATUS_OPT_INCLUDE_UNMODIFIED|
                  GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS|
                  GIT_STATUS_OPT_DISABLE_PATHSPEC_MATCH);
    opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
    opts.version =  GIT_STATUS_OPTIONS_VERSION;
    int err = git_status_foreach_ext(repo, &opts, auxiliar_list_files_cb,
                                     &repo_iteration); 
    if (err){
      throw gitException("",__FILE__,__LINE__); 
    }

    // set the status of the directories
    // dealing with directories
    Mantid::API::SCRIPTSTATUS acc_state = Mantid::API::BOTH_UNCHANGED;
    std::string last_directory = "";
    for (size_t i = repository_list.size() - 1 ; i> 0;
         i--){
      struct file_entry & curr_entry = repository_list[i]; 
      std::string & curr_path  = curr_entry.path; 
      size_t pos = curr_path.rfind("/"); 
      std::string parent_dir ="";       
      if (pos != string::npos){
        parent_dir  = std::string(curr_path.begin(), curr_path.begin() + pos);         
      }
      

      if (curr_entry.directory){        
        curr_entry.status = acc_state; 
        last_directory = curr_path; 
      }

      // this file is child of the last directory?
      if (!last_directory.empty()){
        if (curr_path.find(last_directory) == string::npos){
          // no, it is not child
          // restart the status 
          acc_state = Mantid::API::BOTH_UNCHANGED;          
        }          
      }
      //FIXME: when you download just one file, it says that you still can download the folder.
      // update status
      switch ( acc_state | curr_entry.status){
        case BOTH_UNCHANGED: 
          acc_state = BOTH_UNCHANGED;
          break;
        case REMOTE_ONLY:
        case LOCAL_ONLY:
        case LOCAL_CHANGED:
        case REMOTE_CHANGED:
          acc_state =  curr_entry.status; 
          break;
        default: 
          acc_state = BOTH_CHANGED; 
          break;
      }
    }
  
    vector<std::string> file_names(repository_list.size());

    for (unsigned int i=0; i< repository_list.size(); i++)
      file_names[i] = repository_list[i].path;
    return file_names; 
  } 

  

  int GitScriptRepository::recurseDirectory( vector<struct file_entry> & repository_list,
                                             unsigned int & index){

    int acc = BOTH_UNCHANGED; 
    for (unsigned int i = index+1; i< repository_list.size();
         i++){
      if (i >= repository_list.size())
        return acc; 
      GitScriptRepository::file_entry * curr = &repository_list[i];
      if (curr->directory){        
        curr->status = (Mantid::API::SCRIPTSTATUS)recurseDirectory(repository_list, i);        
      }else{
        int acc_or_currrent = (acc | curr->status); 
        switch ( acc_or_currrent){
        case BOTH_UNCHANGED: 
          acc = BOTH_UNCHANGED;
          break;
        case REMOTE_ONLY:
        case LOCAL_ONLY:
        case LOCAL_CHANGED:
        case REMOTE_CHANGED:
          acc =  acc_or_currrent; 
          break;
        default: 
          acc = BOTH_CHANGED; 
          break;
        } 
      }
    }
    return acc; 
  }

  /**
     One performance issue was considered for the fileStatus. We could have chosen to check for each file,
     it's current state. But, as long this method may be called many times for every single file (specially, 
     when connected to GUI ), we decided to have in memory the status of the files being filled through
     the listFiles method. This means that, at least before calling the fileStatus, it is necessary to 
     call first the listFiles method. 
   
     @pre For a correct answer, ensure that the method ::listFiles was executed. 
   
     For detailed information, please look at ScriptRepository::fileStatus.

   
  */
  SCRIPTSTATUS GitScriptRepository::fileStatus(const std::string file_path) {
    if (!repo)
      throw invalidRepository();
    // change the path to a path related to the repository.
    bool file_is_local; 
    std::string file_path_adjusted = convertPath(file_path, file_is_local); 
    //g_log.debug() << "Check status of file " << file_path_adjusted << "\n"; 

    for( std::vector<struct file_entry>::iterator it = repository_list.begin(); 
         it != repository_list.end();
         it++){
      if( it->path == file_path_adjusted){
        //g_log.debug() << "File " << file_path_adjusted << " status = " << it->status;
        return it->status;       
      }
    }

    char info[200]; 
    snprintf(info, 200, 
             "The File %s was not found inside the repository. Hint: Check spelling, and list the files again",
             file_path_adjusted.c_str()); 
    g_log.warning() << info << "\n";        
    throw ScriptRepoException(info, "Exception at GitScriptRepository::fileStatus");
  }


  std::string GitScriptRepository::convertPath(const std::string path, bool & file_is_local){
    if (!repo)
      throw invalidRepository();
    const char * git_rep =  git_repository_path (repo);
    // git path always returns '.git/',
    //confirm: 
    size_t size = strlen(git_rep); 
    assert(git_rep[size-5] == '.');assert(git_rep[size-4] == 'g'); 
    assert(git_rep[size-3] == 'i');assert(git_rep[size-2] == 't'); 
    assert(git_rep[size-1] == '/');

    std::string repo_path = std::string(git_rep, git_rep + size - 5); 
    //g_log.debug() << "Repo_path: " << repo_path <<"\n"; 

    // first of all, check if the file is local: 
    // the given path may be absolute, relative to the git local repository or to your home
    vector<std::string> lookAfter; 
    lookAfter.push_back(Path::current()); 
    lookAfter.push_back(Path::home()); 
    lookAfter.push_back(repo_path); 
  
    Path pathFound; 
    // try to find the given path at one of the paths at lookAfter.
    file_is_local = Path::find(lookAfter.begin(), lookAfter.end(), path, pathFound);
    // get the absolute path: 
    std::string absolute_path;
    if (file_is_local)
      absolute_path = pathFound.absolute().toString(); 
    else
      absolute_path = path;
    // this is necessary because in windows, the absolute path is given with \ slash.
    boost::replace_all(absolute_path,"\\","/");     
    
    
    //check it the path is inside the repository: 
    size_t pos = absolute_path.find(repo_path); 

    if (pos == std::string::npos){
      // the given file is not inside the local repository. It can not be converted. 
      return path; 
    }else{
      // the path is inside the local repository
      // remove the repo_path from te absolute path 
      return std::string(absolute_path.begin() + pos + repo_path.size(), absolute_path.end());
    }

  
    return path; 
  }


  /**
     For details about download, please look at ScriptRepository::download.

     For performance reason, and in order to allow a user to download many files, this method
     does not look at the remote site to download your file. In reallity, the file is already 
     on your local machine when you perform an ::update operation. So, what download does, is 
     just to peak the last file for the origin tree, decompress it and put it inside your folder. 

     This implies that this method is not sufficient alone. You should perform the 
     GitScriptRepository::update first. This is not done in your behalf, in order to allow you 
     to download many files, without being blocked on many queries to the remote site that 
     probably would not have changed in the miliseconds between one query and another. 

     But, it is your responsiblity to ensure that the update method was called at least once.
     If it has not beeing called in the life of this object, a warning message will be given you
     to remember you may be downloading an old version.

     @pre: Before downloading, please be sure you have executed the GitScriptRepository::update

     @note: The download it self does not depend on the internet connection


     Examples: 

     @code
     GitScriptRepository p; 
     p.update(); // update the informatio about the remote repository
     p.download("README.md"); // download this file
     p.download("TofConv"); // download the folder and all files inside this folder
     p.download("autoreduce/"); // download this file (may create the folder )  
     @endcode

  
     To download the file, it uses the git_checkout_index, documentation available at: 
     http://libgit2.github.com/libgit2/#HEAD/group/checkout/git_checkout_index

  */
  void GitScriptRepository::download(const std::string file_path) {
    if (! repo)
      throw invalidRepository();

    if (!update_called){
      g_log.warning() << "The information about the remote repository may be out-to-date.\n"
                      << "Maybe you should update that information through ::update() method\n";
    }

  
    // change the path to a path related to the repository.
    bool file_is_local;
    std::string file_path_adjusted = convertPath(file_path, file_is_local); 
    g_log.debug() << "Request to download file : " << file_path_adjusted << "\n"; 
    // refresh the list of files inside the repository. It should do at least once.
    // but, usually, the information will not change after.
    if (repository_list.size() == 0)
      listFiles(); 

    bool file_inside_repository = false; 
    struct file_entry * curr_entry = NULL; 
    for( std::vector<struct file_entry>::iterator it = repository_list.begin(); 
         it != repository_list.end();
         it++){
      if( it->path == file_path_adjusted){
        file_inside_repository = true;
        curr_entry = &(*it); 
        break; 
      }
    }
    // it is impossible to download/update files that are not inside the repository
    if (!file_inside_repository){
      char info[200]; 
      snprintf(info,200, "The file %s is not inside the repository. You cannot download it or update it.\nHint: Check mispelling", file_path.c_str());
      g_log.warning() << "File not inside the repository: " << info << "\n"; 
      throw ScriptRepoException(info); 
    }
    {
    // file found
    const char * git_rep =  git_repository_path (repo);
    // git path always returns '.git/',
    //confirm: 
    size_t size = strlen(git_rep); 
    std::string repo_path = std::string(git_rep, git_rep + size - 5); 
    
    std::string abs_path = std::string(repo_path).append(file_path_adjusted); 
    
    Poco::File file2down (abs_path); 
    
    if (file2down.exists()){
      //make backup if necessary
      SCRIPTSTATUS st = curr_entry->status; 
      if (st == LOCAL_CHANGED || st == BOTH_CHANGED){
        g_log.debug() << "perform a backup of this file\n";
        Poco::File f (std::string(repo_path).append(file_path_adjusted)); 
        // copy file
        std::string bck = std::string(f.path()).append("_bck"); 
        g_log.warning() << "This script has local changes that would be override from the remote one"
                        << " so, a back up copy will be created on your behalf, at: " 
                        << bck << "\n";
        f.copyTo(bck);
      }                          
    }else{
      if (curr_entry->directory){
        //  file2down.createDirectories();
      }else{      
      // touch file
      size_t pos = file_path_adjusted.rfind("/"); 
      if (pos != string::npos){
        std::string dir_parent = std::string(repo_path).append(std::string(file_path_adjusted.begin(), 
                                                                           file_path_adjusted.begin() + pos));
        Poco::File dir_ (dir_parent); 
        dir_.createDirectories();             
      }
      file2down.createFile();       
      }
    }

  }
    // if the file can be downloaded
    // the download used the gti_checkout_index method.  
    git_checkout_opts opts; 
    int err = 0; 
    memset(&opts,0,sizeof(opts));
    opts.version = GIT_CHECKOUT_OPTS_VERSION;
    opts.checkout_strategy = GIT_CHECKOUT_FORCE; 
    opts.paths.strings = new char*[1];
    // add file name
    opts.paths.strings[0] = strdup(file_path_adjusted.c_str()); 
    opts.paths.count = 1; 
    g_log.debug() << "entering git checkout index " << opts.paths.strings[0]<< "\n" ;
    //err = git_checkout_index(repo, NULL, &opts); 
    err = git_checkout_head(repo, &opts); 
      g_log.debug() << " git checkout index\n" ;
    // release memory FIXME
    delete [] opts.paths.strings[0]; 
    free(opts.paths.strings); 
    opts.paths.strings = NULL;
  
    if (err)
      throw gitException("Failure to download."); /// @todo provide a better explanation.
    g_log.debug() << "download ok! \n"; 
  }


  /** Update must create the local repository if it does not exists: (clone)
      And must fetch the origin, in order to get the current information about the remote repository

      @attention this method requires internet connection.

      @note Detailed information of this method is available at ScriptRepository::update.
    
    
  */
  void GitScriptRepository::update(void) 
  {
    g_log.debug() << "GitScriptRepository::update ... begin\n" ; 
    // if the repository was not initialized, check if we can initialize (meaning that it was created)
    if (!repo){
      // it will try to initialize the git_repository
      // It is possible to have a non valid git_repository
      // when there is no local repository. But, this means, 
      // that it will accept only the update method.
      repo = NULL; 
      int err = git_repository_open(&repo, local_repository.c_str());
      if (err){
        const git_error *git_err = giterr_last();
        g_log.debug() << "Script Repository Update open error: " << git_err->message << ". Code(" << git_err->klass << ")." << std::endl; 
        // repo should be NULL in this case
        assert(!repo);

        // if it does not exists... we have to create
        g_log.debug() << "GitScriptRepository::update call clone" << std::endl;   
        cloneRepository(); 
        update_called = true; // the clone, do also update the origin information
        return;
      }   
    }


    // if we are here, we have a valid repo
    assert(repo); 
    g_log.debug() << "GitScriptRepository::update call fetchOrigin\n";  
    // try to update the information
    fetchOrigin(); 
    g_log.debug() << "GitScriptRepository::update done\n";  
    // it did not throw exception, so 
    update_called = true;  
    return; 
  }


  /**
     The fetching of the origin was inspired on the example on liggit2 (network/fetch.c)
     The documentation is available at: 

     http://libgit2.github.com/libgit2/ex/HEAD/fetch.html#git_remote_update_tips-8

  */
  void GitScriptRepository::fetchOrigin(void) {
    g_log.debug() << "GitScriptRepository::fetchOrigin begin\n" ; 
    // ensures this means that repo exists
    git_remote * remote = NULL;
    const git_transfer_progress * stats;
  
    int counter; 
    const char * load_failure = "Hint: Check internet connection";
    const char * update_failure = "Internal error updating index";

    if(git_remote_load(&remote, repo, "origin"))
      throw gitException(load_failure, __FILE__, __LINE__); 
    g_log.debug() << "GitScriptRepository::fetchOrigin configure remote\n";    
    stats = git_remote_stats(remote); 
    g_log.debug() << "GitScriptRepository::fetchOrigin get stats\n";  
    //open a connection
    if (git_remote_connect(remote, GIT_DIRECTION_FETCH)){
      git_remote_free(remote);
      throw gitException(load_failure,__FILE__,__LINE__);
    } 
    g_log.debug() << "GitScriptRepository::fetchOrigin open connection\n";  
  
    // does the download through a call back function
    // trying to avoid the call back, is it possible?
    if (git_remote_download(remote, NULL, &counter)){
      g_log.warning() << "GitScriptRepository::fetchOrigin download failure\n";  
      git_remote_disconnect(remote); 
      git_remote_free(remote);
      throw gitException(update_failure,__FILE__,__LINE__); 
    }

    g_log.debug() << "Repository Download done: Fetch " << stats->indexed_objects
                  << "/" << stats->total_objects
                  << " in " << stats->received_bytes
                  << " bytes. \n";
  
    if (git_remote_update_tips(remote)){
      g_log.warning() << "GitScriptRepository::fetchOrigin Git Update tips failed\n" ; 
      git_remote_disconnect(remote); 
      git_remote_free(remote); 
      throw gitException(update_failure,__FILE__,__LINE__);
    }

    git_remote_disconnect(remote); 
    git_remote_free(remote);
  }

  
typedef struct progress_data {
	git_transfer_progress fetch_progress;
	size_t completed_steps;
	size_t total_steps;
	const char *path;
  int up_to;
  Mantid::Kernel::Logger * log; 
} progress_data;

static void print_progress(progress_data *pd)
{
	int network_percent = (100*pd->fetch_progress.received_objects) / pd->fetch_progress.total_objects;
	int index_percent = (100*pd->fetch_progress.indexed_objects) / pd->fetch_progress.total_objects;
  int percent = (network_percent + index_percent )/ 2; 
  if (percent > pd->up_to){
    pd->up_to = (percent + 5); 
    pd->log->notice() << "Progress: " << index_percent << "%\n"; 
  }
}

static void fetch_progress(const git_transfer_progress *stats, void *payload)
{
	progress_data *pd = (progress_data*)payload;
	pd->fetch_progress = *stats;
	print_progress(pd);
}
static void checkout_progress(const char *path, size_t cur, size_t tot, void *payload)
{
	progress_data *pd = (progress_data*)payload;
	pd->completed_steps = cur;
	pd->total_steps = tot;
	pd->path = path;
	print_progress(pd);
}

static int cred_acquire(git_cred **out,
		const char * /*url*/,
		unsigned int /*allowed_types*/,
		void * /*payload*/)
{
	char username[128] = {0};
	char password[128] = {0};

	printf("Username: ");
	scanf("%s", username);

	/* Yup. Right there on your terminal. Careful where you copy/paste output. */
	printf("Password: ");
	scanf("%s", password);

	return git_cred_userpass_plaintext_new(out, username, password);
}


  /**
     Does the cloning, but do also ensure that automatic generated files, will not be 
     shown at the repository. Specially, .pyc files.

  */
  void GitScriptRepository::cloneRepository(void) {
    using Mantid::Kernel::FacilityInfo;
    using boost::algorithm::split;
    using boost::algorithm::is_any_of;
    using boost::algorithm::join;           

    g_log.debug() << "GitScriptRepository::cloneRepository ... begin\n" ; 
    git_repository *cloned_repo = NULL;
    git_clone_options clone_opts;  
    git_checkout_opts checkout_opts;
    int error;
    git_config * cfg;
    
  error = git_config_open_default(&cfg);
  if (error){
    throw gitException("Script Repository Proxy Configuration Failed");
    }
    const FacilityInfo defaultFacility = ConfigService::Instance().getFacility();
    std::string proxy = defaultFacility.getHTTPProxy();
    if (!proxy.empty()){
      const char * git_proxy; 
      git_config_get_string(&git_proxy, cfg, "http.proxy"); 
      if (proxy != git_proxy){
        g_log.debug() << "Script Repository Proxy configured to " << proxy << "\n"; 
        git_config_set_string(cfg,"http.proxy",proxy.c_str()); 
      }    
    }

    // Set up options
    // avoid downloading the files, letting the local folder clean (to not fill the local folder
    // with files that the user is not interested with   
    progress_data pd; 
    memset(&pd,0,sizeof(pd)); 
    pd.log = &g_log; 
    pd.up_to = 0;
    memset(&clone_opts, 0, sizeof(clone_opts)); 
    memset(&checkout_opts,0,sizeof(checkout_opts));
    clone_opts.version = GIT_CLONE_OPTIONS_VERSION; 
    checkout_opts.version = GIT_CHECKOUT_OPTS_VERSION;
	  checkout_opts.checkout_strategy =  GIT_CHECKOUT_UPDATE_ONLY| GIT_CHECKOUT_ALLOW_CONFLICTS;
	  checkout_opts.progress_cb = checkout_progress;
	  checkout_opts.progress_payload = &pd;
	  clone_opts.checkout_opts = checkout_opts;
	  clone_opts.fetch_progress_cb = &fetch_progress;
	  clone_opts.fetch_progress_payload = &pd;
	  clone_opts.cred_acquire_cb = cred_acquire;

    
    // Do cloning
    // try not to use the call back function
    g_log.notice() << "ScriptRepository installation started! (Remember it will take a couple of minutes)" << std::endl; 

    // get the path of the current directory
    vector<std::string> url_paths;
    std::string str = remote_url;
    //split the csv of url path in url paths
    // git://github.com/scripts;http://github.com/scripts
    // will be splited in : git://github.com/scripts http://github.com/scripts
    split(url_paths, str, boost::is_any_of(";"));
    error = 0; 
    for (vector<std::string>::iterator url = url_paths.begin();
         url != url_paths.end(); 
         url ++){

      // this test is done here, in order not to repeat twice the 
      // error message. So, it will print this message only if there is more 
      // than one attempt to install the repository.
      if (error)
         g_log.warning() << "Attempt to install the repository failed with the following message: "
                         << giterr_last()->message << ".\t New attempt will be done..." << std::endl;  

      g_log.debug() << "Installing Script Repository for the url: " << (*url) << std::endl;  
      // clone the repository
      error = git_clone(&cloned_repo, 
                        (*url).c_str(), 
                      local_repository.c_str(), 
                      &clone_opts
                      );     

      if (error) // if error, try the following approach
        {
          continue;
        }
      else
        break; // no error, means success
    }
    if (error){
      throw gitException("Script Repository installation failed!",
                         __FILE__, __LINE__); 
    }
    g_log.debug() << "ScriptRepository cloneRepository ... installed!" << std::endl;
    char exclude_file_path[200]; 
    snprintf(exclude_file_path,200, "%s/info/exclude",git_repository_path(cloned_repo));
    // open the file in append mode
    FileStream file(exclude_file_path, std::ios::out| std::ios::app);   
    file << "*.pyc\n";
    file.close(); 
    repo = cloned_repo; 
    g_log.notice() << "ScriptRepository Installation Success!\n";
    return;

  }


  /**
     At the time of the development of this method, libgit2 did not support ssh transport, 
     what does not allow us to do upload directly. The alternative was the creation of a
     webserver that allows the users to upload their files. You may found this webserver 
     at
   
     @todo: show the web server url 
   
     Although users may access that site to upload files, it was required that you should 
     provide also the access through the mantid itself. 

     This is what this method does. 
   
     It takes the file as input, produces a zip file, and them, submit the form. 

     @todo point where to know about the uploader server.

  */
  void GitScriptRepository::upload(const std::string file_path, 
                                   const std::string comment,
                                   const std::string author, 
                                   const std::string description)
    
  {
    using Poco::FileOutputStream; 
    ///  @todo: deal with the description
    UNUSED_ARG(description);    
    /// @todo: create the zip file
    bool file_is_local; 
    std::string file_path_adjusted = convertPath(file_path, file_is_local); 
    if (!file_is_local)
      throw ScriptRepoException("You can not upload an unexisting file!"); 

    try{
      const char * failure_message = "Failed connect to server\n";
      std::string url; 
      ConfigServiceImpl & config = ConfigService::Instance();
      url = config.getString("UploaderWebServer");
      if (url.empty())
        throw ScriptRepoException("Failed to get information about the Uploader Server.Please, check you have a UploaderWebServer entry in your properties file"); 

      URI uri(url); 
      std::string path(uri.getPathAndQuery()); 
      if (path.empty()) path = "/";    

      // creating the connection:
      HTTPClientSession session(uri.getHost(), uri.getPort()); 


      HTTPResponse response; 
      {
        // send the request, to load the page
        HTTPRequest request(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1); 
      
        session.sendRequest(request); 
        std::istream & rs = session.receiveResponse(response); 
        g_log.debug() << "Get Page: Response Status = " << response.getStatus() << " " << 
          response.getReason() << "\n"; 
      
        if (response.getStatus() != Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED){
          ///@todo It is not necessary to print the page
          StreamCopier::copyStream(rs,std::cout); // print the page 
          //Poco::NullOutputStream null;
          //StreamCopier::copyStream(rs, null);
        }else{
          StreamCopier::copyStream(rs, std::cerr); //print the error
          throw ScriptRepoException(failure_message); 
        }
      }

      // so we got the connection, and requested the form
      // create the post request
      HTTPRequest post_request(HTTPRequest::HTTP_POST, path, HTTPMessage::HTTP_1_1);     

      std::vector<HTTPCookie> cookies;
      {
        // check if the connection requires cookies     
        response.getCookies(cookies); 
        NameValueCollection nvc;
        std::vector<HTTPCookie>::iterator it = cookies.begin();
        for(; it != cookies.end(); ++it){
          nvc.add((*it).getName(), (*it).getValue());
        }  
        if (cookies.size())
          post_request.setCookies(nvc); 
      }
    
      // fulfilling the form
      HTMLForm form(HTMLForm::ENCODING_MULTIPART); 
      if (cookies.size())
        // add the csrf code (DJANGO REQUIRED)
        form.add("csrfmiddlewaretoken",cookies[0].getValue()); 
    
      form.add("comment", comment); 
      form.add("author", author); 
    

      /// the form will take ownership of this memory and 
      /// will delete it when necessary
      FilePartSource * fil = new FilePartSource(file_path,
                                                "application/octet-stream");       
      form.addPart("script", fil); 
     
      //send the request     
      form.prepareSubmit(post_request);
      std::stringstream sst; 
      form.write(sst); 
      int size = (int) sst.str().size(); 
      // set the length of the request.
      post_request.setContentLength(size+2);

      // send header
      std::ostream& ostr = session.sendRequest(post_request);
      // send content
      ostr << sst.str();
    
    
      {
        std::istream& rs = session.receiveResponse(response);
        g_log.debug() << "Post Page: Response Status = " << response.getStatus() << " " << response.getReason() << std::endl;

        if (response.getStatus() != Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED){
          ///@todo It is not necessary to print the page
          StreamCopier::copyStream(rs,std::cout); // print the page 
          //Poco::NullOutputStream null;
          //StreamCopier::copyStream(rs, null);
        }else{
          StreamCopier::copyStream(rs, std::cerr); //print the error
          throw ScriptRepoException(failure_message); 
        }
      
      }
    }	catch (Exception& exc)
      {
        g_log.error() << "GitScriptRepository::upload failure : " << exc.displayText() << "\n"; 
        // convert the exception in ScriptRepoException
        throw ScriptRepoException("Failed to upload",
                                  exc.displayText(), __FILE__, __LINE__); 
      }
  
  }


  static bool  hasAlpha(std::string buf){
    std::locale loc;
    for(string::iterator it = buf.begin();
        it != buf.end();
        it++){
      if (isalpha(*it,loc)){
        return true; 
      }
    }
    return false; 
  }

  static std::string extractPythonDocString( std::istream & input, 
                                             size_t &doc_start, 
                                             size_t &doc_end){

    const char * startmarks[] = {"\"\"\"",
                                 "'''",
                                 "##",
                                 "# ",
                                 "#\n"};
    const char * endmarks[] = {"\"\"\"",
                                 "'''",
                                 "",
                                 "",
                               ""};

    // int docstring_found = 0; 
    std::string buf;
    size_t pos; 
    size_t start_mark = SIZE_MAX; 
    size_t end_mark = SIZE_MAX; 
    int end_mark_index = 0;
    while(getline(input,buf)){
      if (start_mark == SIZE_MAX){
        // looking for the start mark
      for (int i = 0; i< 5; i++){
        pos = buf.find(startmarks[i]); 
        if (pos == 0){
          std::cout << "start_mark found\n";
          end_mark_index = i;
           start_mark = input.tellg();
           if (hasAlpha(buf)){
             start_mark -= (buf.size()-strlen(startmarks[i]) + 1);
           }
        }
      }
      continue;
      }
      
      // looking for end mark
      if (end_mark_index < 2){
        pos = buf.find(endmarks[end_mark_index]); 
        if (pos != std::string::npos){
          end_mark = input.tellg(); 
          if (hasAlpha(buf))
            end_mark -= (buf.size() - pos + 1);
          else
            end_mark -= (buf.size() + 2);
          break;
        }

      }else{
        if (buf.size() == 0){
          end_mark = input.tellg(); 
          end_mark -= 2;
          break;
        }
      }
      
    }

    if (start_mark == SIZE_MAX || end_mark == SIZE_MAX){
      // no documentation
      doc_start = 0; 
      doc_end = 0; 
      input.clear(); 
      input.seekg(0,std::ios::beg); 
      return "";
    }
    
    doc_start = start_mark; 
    doc_end = end_mark; 
    size_t size = end_mark - start_mark;
    char * buffer = new char [size + 1]; 
    input.clear(); 
    input.seekg(start_mark,std::ios::beg); 
    input.read(buffer,size);
    buffer[size] = '\0'; 
    std::string ret = buffer; 
    delete [] buffer; 
    return ret; 
    //  end_mark; 
  }


  static void insertInformation(std::ostream& out,
                                std::istream & in,
                                const  std::string info,
                                long int start_mark,
                                long int end_mark){
  
    in.clear(); 
    // read the beggining of the document 
    // and insert it at the output
    if (start_mark){
      in.seekg(0,std::ios::beg);
      char * buffer = new char [start_mark + 1];
      in.read(buffer, start_mark); 
      out << buffer; 
      delete [] buffer; 
    }else{
      // insert the comment area
      out << "\"\"\"\n"; 
    }
  
    // insert the info 
    out << info ; 

  
    // insert the rest of the document
    if (end_mark)
      in.seekg(end_mark, std::ios::beg); 
    else
      out << "\"\"\"\n"; 
  
    StreamCopier::copyStream(in, out);
  }
     
  std::string GitScriptRepository::processInfo(const std::string path, FILEINFOSUPPORT filetype){
    g_log.debug() << "GitScriptRepository::processInfo " << path << " \n"; 

    bool local;
    std::string convpath = convertPath(path, local); 
    std::string abs_path = std::string(localRepository()).append("/").append(convpath); 

    std::stringstream description;
    std::string directory = abs_path;  


    if (!local){
      /** It is necessary do download the file before (don't worry, it will not use the internet,
          it will just take the file from the local repository and decompress it. The file is already
          there. But, this must be something that is temporary, so we must restore the 
          local repository to its position before, removing every file, folder created.
        
          This part will look to see how many directories/files we will create to download this file
      */
    
      std::string part_removed;
      while(true){      
        size_t found; 
        // find the last directory
        found = directory.rfind("/"); 
        if (found == string::npos){
          // no more directory: means no directory created.
          directory = ""; 
          break; 
        }

        // remove the last file/directory
        // and get the directory parent
        part_removed = std::string(directory.begin() + found,directory.end()); 
        directory = directory.replace(found, directory.size()-found,""); 
      
        // check if the directory already exists
        Poco::File f(directory);
        if (f.exists()){
          // this means, that this directory will not be created, 
          // but, its child will be created, so, we will mark its 
          // child to be removed
          directory.append(part_removed); 
          break;
        }
      
        if ( localRepository().find(directory) != string::npos) {
          // we reached the repository root, so no directory will be created.
          directory = ""; 
          break;
        }
      
      }

      // download the file, to check its info
      download(convpath); 
    }

    //open the file
    FileInputStream  input(abs_path, std::ios::in);   
 
    switch(filetype){
    case PYTHONFILE:
      {
        size_t start, end; 
        description << extractPythonDocString(input, start, end); 
      }
      break;
    case READMEFILE:    
      StreamCopier::copyStream(input,description); // copy readme file 
      break; 
    default:
      g_log.information() << "No support for extracting information of file like" << abs_path << "\n"; 
      break;
    }
    // close the file descriptor, to allow deleting the file.
    input.close();
    if (!local){
      try{
        // remove the files and folders that were created when downloading
        if (directory.empty()){     
          Poco::File f(abs_path); 
          f.remove(); 
        }
        else{
          Poco::File f(directory); 
          f.remove(true); 
        }    
      }catch (Poco::Exception & ex){
        g_log.error() <<"ProcessInfo exception: " << ex.className() << ": " << ex.message() << "\n";


      
      }
    }

    return description.str(); 
  }

  /**
     The information about one file has 3 fields: author, description and pub_date. Look at ScriptRepository::fileInfo for details about these fields. 
   
     The main issue, is how to get the file description. As defined, is defined differently, depending on the kind of file. 
  */
  ScriptInfo GitScriptRepository::fileInfo(const std::string path) {
    using boost::algorithm::ends_with;
    if ( !repo)
      throw invalidRepository();
    g_log.debug() << "GitScriptRepository::fileInfo " << path << " \n"; 
    ScriptInfo empty_info; 
    empty_info.author = ""; 
    empty_info.description = ""; 
    empty_info.pub_date = Kernel::DateAndTime(); 

    // look after the input file inside the repository_list
    for( std::vector<struct file_entry>::iterator it = repository_list.begin(); 
         it != repository_list.end();
         it++){
      if( it->path == path){

        if (it->directory){
          // for directories, the info may be inside README or __init__.py (module)
          // look for the README file
          // look for the __init__.py file
          std::string readme_path = path;
          std::string init_path = path; 
          readme_path.append("/README");
          init_path.append("/__init__.py"); 
          for (;it!= repository_list.end();
               it++){
            // continue to look inside the list to find the files
          
            if (it->path.find(readme_path) != string::npos){
              // README found
              empty_info.description = processInfo(it->path, READMEFILE); 
              return empty_info; 
            }else if (it->path.find(init_path) != string::npos){
              // __init__.py found
              empty_info.description = processInfo(it->path, PYTHONFILE); 
              return empty_info; 
            }
          }

          // after looking at all files,         
          // no useful information from this folder found
          return empty_info;                         
        }else{
        
          // the path is a file
          if (ends_with(it->path, ".py") || ends_with(it->path,".PY")){
            empty_info.description = processInfo(it->path, PYTHONFILE);
          }else if (it->path.find("README") != string::npos){
            empty_info.description = processInfo(it->path,READMEFILE);           
          }
        
          return empty_info; 
        }

      }    
    }
    // after looking at all files, it did not find 
    // this path
    std::stringstream info; 
    info << "The required file " << path << " was not found inside the repository!"<<std::ends; 
    g_log.warning() << info.str()<<"\n"; 
    throw ScriptRepoException(info.str(), "GitScriptRepository::fileInfo"); 
  }
}// END API
}// END MANTID



