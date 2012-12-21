#ifndef _MANTIDSCRIPTREPOSITORY_GITSCRIPTREPOSITORY_H_
#define _MANTIDSCRIPTREPOSITORY_GITSCRIPTREPOSITORY_H_

#include "MantidAPI/ScriptRepository.h"
#include <vector>
using Mantid::API::ScriptRepository; 
class git_repository ; 
namespace Mantid
{
  namespace Kernel{
    class Logger;
  }
namespace API{


  class GitScriptRepository: public ScriptRepository
  {    
  public:
    /**
       The main information that GitScriptRepository needs to be able 
       to operate are where the local repository is (or will be), and
       the url for the remote repository (that is required, in case, 
       the local repository does not exists and it is necessary to 
       clone it. 

       Usually these values are available at the Mantid properties files, 
       so, it is possible to construct the GitScriptRepository without
       parameters. 

       But, for flexibility reasons, (for example, testing with other
       repositories), a more general constructor is provided. 
       
       In case a string is passed to the constructor different from the 
       default one, it will have precedence, but it will not override what
       is defined by the Mantid properties files. These values will be valid
       only for that instance. 

       @todo Define the mantid properties (Suggestion: ScriptRepository, 
       ScriptLocalRepository)

       @code
       // get ScriptRepository and ScriptLocalRepository values from Mantid Config Service
       GitScriptRepository sharing(); 
       // apply given values
       GitScriptRepository sharing("/tmp/gitrep", 
                         "git://github.com/mantidproject/scripts");      
       @endcode
     */
   
    GitScriptRepository(const std::string local_repository = std::string(), 
                        const std::string remote_url = std::string())
      throw (ScriptRepoException&); 
    
    virtual ~GitScriptRepository() throw();

    ScriptInfo fileInfo(const std::string path) throw (ScriptRepoException&);
    
    std::vector<std::string> listFiles() throw (ScriptRepoException&); 
    
    void download(const std::string file_path) throw (ScriptRepoException&);
    SCRIPTSTATUS fileStatus(const std::string file_path) throw (ScriptRepoException&);
    
    void upload(const std::string file_path, const std::string comment,
                const std::string author, 
                const std::string description = std::string()) throw (ScriptRepoException&);
    void update(void) throw (ScriptRepoException&);


    std::string localRepository() const {return local_repository;  }

    /// Define a file inside the repository
    struct file_entry{
      /// path related to git
      std::string path; 
      /// file status
      SCRIPTSTATUS status; 
      /// show if it is a directory or not
      bool directory;
    };

    /** Auxiliar struct that will be used as payload, to allo 
        auxiliar_list_files_cb to list all files and folders inside
        a repository.
    */
    struct repo_iteration{
      std::string last_directory;
      std::vector<file_entry > * repository_list;
    };
  protected:


    /// get all the files from a repository
    std::vector<struct file_entry> repository_list; 
    
    int recurseDirectory( std::vector<struct file_entry> & repository_list,
                         unsigned int & index);

    /// Used to throw when a local repository is mal-formed.
    ScriptRepoException invalidRepository();

    ScriptRepoException gitException(const std::string info = std::string(),
                                     const std::string file = std::string(),
                                     int line = -1); 


    /**
       Transform the file path in a path related to the local repository.
       Set the flag file_is_local to true if the file already exists inside
       the local machine.
       
       For example: 
       
       @code 
       // consider the local repository at /opt/scripts/
       bool flag; 
       convertPath("/opt/scripts/README.md", flag) // returns: README.md
       convertPath("README.md", flag) // returns: README.md
       // consider the local repository at c:\MantidInstall\scripts
       convertPath("c:\MantidInstall\scripts\README.md", flag)// returns README.md
       @endcode
     */
    std::string convertPath(const std::string path, bool & file_is_local); 
    
    void cloneRepository(void) throw (ScriptRepoException&); 
    void fetchOrigin(void) throw (ScriptRepoException&); 
    enum FILEINFOSUPPORT{READMEFILE, PYTHONFILE};
    std::string processInfo(const std::string path, FILEINFOSUPPORT filetype); 
    

  private: 
    /// Path of the local repository.
    std::string local_repository; 
    /// URL for the remote repository, usually: git://github.com/mantidproject/scripts.git
    std::string remote_url;
    /// Pointer to the liggit2 repository struct
    git_repository * repo; 
    /// reference to the logger class
    Mantid::Kernel::Logger& g_log;
    
    /// flag to indicate that the ::update method was called at least once in the life of
    /// this object.
    bool update_called; 
  };
  
} // namespace API
} // namespace Mantid


#endif // _MANTIDSCRIPTREPOSITORY_GITSCRIPTREPOSITORY_H_
