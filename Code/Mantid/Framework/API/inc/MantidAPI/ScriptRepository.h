#ifndef MANTID_API_SCRIPTREPOSITORY_H_
#define MANTID_API_SCRIPTREPOSITORY_H_


#include <string>
#include <vector>

#include "MantidKernel/DateAndTime.h"
#include "MantidAPI/DllConfig.h"
#include <boost/shared_ptr.hpp>

namespace Mantid
{
namespace API{


  ///Information about the files inside the repository. 
  struct ScriptInfo
  {
    /// Identification of the author of the script.
    std::string author; 
    /// Description of the purpose of the script
    std::string description; 
    /// Time of the last update of this file (remotelly)
    Kernel::DateAndTime pub_date;
  };

  /** Represent the possible states for a given file: 
        - REMOTE_ONLY: it exists only at the remote repository. 
        - LOCAL_ONLY: it exists only at the local folder. 
        - REMOTE_CHANGED: it has been changed remotely, may be updated. 
        - LOCAL_CHANGED: it has been modified locally, may be published.
        - BOTH_CHANGED: modified locally and remotelly. 
        - BOTH_UNCHANGED: the local file is a copy of the remotely one.

      For folders, the reason are slightly different: 
        - REMOTE_ONLY: the folder exists only remotely. 
        - LOCAL_ONLY: the folder exists only locally. 
        - BOTH_UNCHANGED: if no file inside the folder has the XXX_CHANGED status. 
        - BOTH_CHANGED: if inside the folder it has one file with BOTH_CHANGED status
        or one REMOTE_CHANGED with another LOCAL_CHANGED.
        - REMOTE_CHANGED: at least one marked with REMOTE_CHANGED and others as 
        REMOTE_CHANGED or BOTH_UNCHANGED. 
        - LOCAL_CHANGED: analogous to REMOTE_CHANGED, but locally. 
   **/
  enum SCRIPTSTATUS{BOTH_UNCHANGED = 0,
                    REMOTE_ONLY = (1u << 0),
                    LOCAL_ONLY = (1u << 1), 
                    REMOTE_CHANGED = (1u << 2),
                    LOCAL_CHANGED = (1u << 3), 
                    BOTH_CHANGED = (REMOTE_CHANGED | LOCAL_CHANGED),
                    }; 

  /**
  The ScriptRepository class is intended to be used mainly by the users, who
  will be willing to share and download scripts for their analysis. As so, 
  the exceptions that may occurr while operating must provide information that 
  are usefull for them to understand what is happening, but, the Mantid Team 
  must still be informed what happened in more techinical detail in order 
  to be able to deal with eventually bugs. 
  
  To provide this functionality, the ScriptRepoException will be used. 
  As a normal std::exception (the default base one used through all the 
  Mantid Project), it allows Mantid to work as normally. 

  But, it extends the usage of the exception, by allowing more information 
  to be added. Below, some examples on how to trigger exceptions. 

  \code
  
  // throw "Unknown Exception"
  throw ScriptRepoException();
  
  // After system changing errno number, for example, EACCES
  // You could give the user the reason way he can not download the file.
  throw ScriptRepoException(EACCES, "You can allowed to download scripts. Please, contact the administrator"); 

  // For more serious exception, you could provide the location where it 
  // were triggered.
  throw ScriptRepoException(errno, "Critical Failure", __FILE__, __LINE__)
  \endcode

  The default ScriptRepoException::what method will be used to show the user
  message, while it is up to whom is using the ScriptRepository to decide on 
  using or not the techinical information through ScriptRepoException::systemError, 
  ScriptRepoException::filePath. 


  */
  class MANTID_API_DLL ScriptRepoException : public std::exception
  {
  public:
    ///default constructor
  ScriptRepoException(const std::string info = std::string("Unknown Exception")):
      _system_error(""),
      _user_info(info),
      _file_path(""){
    };
 
    ScriptRepoException( int err_, const std::string info = std::string(),
                         const std::string file = std::string(),
                         int line = -1);

    ScriptRepoException(const std::string info ,
                        const std::string system,                        
                        const std::string file = std::string(),
                        int line = -1);


    /// Destructor
    ~ScriptRepoException() throw() {}
    
    /// Returns the message string.
    const char* what() const throw(); 
    /*
    {
      return _user_info.c_str();
      }*/
    std::string systemError(); 

    std::string filePath(); 
  private:
    /// The message returned by what()
    std::string _system_error;
    std::string _user_info; 
    std::string _file_path; 
  };

  //----------------------------------------------------------------------
  /** 
  Abstract Class to manage the interaction between the users and the Script folder (mantid subproject). 
  
  Inside the mantid repository (https://github.com/mantidproject) there is also a subproject called 
  scripts (https://github.com/mantidproject/scripts), created to allow users to share their scripts, 
  as well as to allow Mantid Team to distribute to the Mantid community scripts for analysis and 
  also to enhance the quality of the scripts used for the sake of data analysis. 

  The ScriptSharing class aims to provide a simple way to interact with that repository in order to 
  promote its usage. In order to enhance the usage, it is necessary:
  
    - List all scripts available at the repository
    - Download selected scripts. 
    - Check for updates
    - Allow to publish users scripts/folders. 

  ScriptSharing may show all the files inside the script repository through ScriptSharing::listFiles.
  Only the names of the files may not be sufficient, to help deciding if the file would be usefull or 
  not, so, the author, the description and when the file was last changed can be accessed through 
  ScriptSharing::fileInfo. 

  After looking at a file, you may be interested in downloading it through ScriptSharing::download. 

  When working with the repository, the files may be local only, if the user created a file inside 
  his folder, it may not be downloaded, it can be locally modified, or changed remotely and 
  not up-to-date, or even, being modified locally and remotely. Use ScriptSharing::fileStatus in 
  order to get these informations of any file. 

  The user may decide to upload one of his file or folder, to share it. This is possible through
  the ScriptSharing::upload. The same command may be used to publish also some updates made to an 
  already shared file.

  For files that are not up-to-date, the user may be interested in what changes have been done,
  before downloading it, this is possible through ScriptSharing::history. 

  Finally, the ScriptSharing have to check periodically the remote repository 
  (https://github.com/mantidproject/scripts), it is the responsibility of external tools to ensure
  it is done periodically, through ScriptSharing::update. For the sake of simplicity, this method
  is used also to create the local repository in case it does not exists. 

  @note Exceptions will be triggered through ScriptRepoException in order user 
        understandable information as well as techinical details. 


  @section script-description-sec Scripts Description

  The description of the files and scripts will obey an agreement for the 
  following type of files: 
 
   - @ref pyscript-sec
   - @ref folders-sec
   - @ref readme-sec 
   

  @subsection pyscript-sec Python Scripts

  If the script is as python file, then the description will be the module __doc__ attribute. For
  example, the following code: 

  @code{.py}
  import mantid
  print mantid.__doc__
  @endcode
  
  Produces: 
  
  @verbatim
Mantid
======

http://www.mantidproject.org

The Mantid project pro (...)  
  @endverbatim


 @subsection folders-sec Folders
 
 If the script is a folder, it will try to find a file that starts with name README, 
 and will show it. For example, if the mantid repository path would be passed, 
 it would show the content of its README.md file:

@verbatim
Mantid
======

The Mantid project provides (...)

@endverbatim

 In this case, author should be any name found inside the README file that starts with 
 the following line: 'Author:'. 


@subsection readme-sec README files

They will work as was expected for folders @ref folders-sec. 


  @todo Show the known implementations

  @todo The ScriptSharing will use MantidProperties to know where to set the
   local repository as well as the remote repository. (ConfigService)


   @todo Create the abstract history method.
  
  @author Gesner Passos, ISIS, RAL
  @date 11/12/2012
  
  Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
  
  This file is part of Mantid.
  
  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.
  
  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
  
  File change history is stored at: <https://github.com/mantidproject/mantid>.
  Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class MANTID_API_DLL ScriptRepository
  {
  public:
   
    /// Virtual destructor (always needed for abstract classes)
    virtual ~ScriptRepository() throw(){};
    
    
    

    /** The constructor of ScriptSharing. 
        
        Ideally, it requires the definition of the remote repository
        and the local repository. But these values will be available inside
        the mantid properties service (MantidKernel::ConfigService). 
     */
    // ScriptSharing();
    
    /**
       Return the information about the script through the ScriptRepoException 
       struct. 

       It may throw exception if the file is not presented locally, or even remotelly. 
       
       For directories, it will try to find a README file inside the directory 
       wich will be used as the description, otherwise, it will return an empty 
       ScriptInfo. 

       The description of the scripts will follow the agreement @ref pyscript-sec.



       @param path Script path related to the repository, or to operate system. 
       @return ScriptInfo Information about the script. 
       @exception ScriptRepoException Mainly for scripts not found. 

       @code
         ScriptSharing spt; 
         ScriptInfo info = spt.fileInfo("README.md"); 
         // info.description : returns the file description.
       @endcode       
     */
    virtual ScriptInfo fileInfo(const std::string path) throw (ScriptRepoException&)  = 0;
    

    /**
       Return the list of files inside the repository. It provides a file-system 
       like path for all the files, folders that are inside the local repository as
       well as remotely.

       Consider the following repository: 
       
       @verbatim 
       README.md
       folderA/
       folderA/fileB
       fileC 
       @endverbatim
     
       And the local repository folder containing these files: 
       
       @verbatim
       README.md
       NewFile
       @endverbatim

       List files, must show all the files:
       @verbatim 
       README.md
       folderA/
       folderA/fileB
       fileC 
       NewFile
       @endverbatim

       @return List of all the files available inside the repository as a file system
       path relative to the local repository. 
       
       @exception May throw Invalid Repository if the local repository was not generated. In this case, it is necessary to execute the ScriptRepository::update (at least once). 
     */
    virtual std::vector<std::string> listFiles() throw (ScriptRepoException&) = 0;


    /**
       Create a copy of the remote file/folder inside the local repository.
       For folder, it will copy all the files inside the folder as well. 

       @attention If one file is different locally and remotelly, the download
                  will make a copy of the remote file, but will preserv a backup
                  of the local file. This will be reported through throwing an
                  exception.
       
       @param path of a file or folder to be downloaded. 

       @exception ScriptRepoException to indicate file is not available at 
                  remotely or to indicate that a confict was found.
       
     */
    virtual void download(const std::string file_path) throw (ScriptRepoException&) = 0 ;


    
    /**
       Return the status of the file, according to the status defined in 
       ::SCRIPTSTATUS. 

       @param path for file/folder
       @return SCRIPTSTATUS of the given file/folder
       @exception ScriptRepoException to indicate that file is not available.
     */
    virtual SCRIPTSTATUS fileStatus(const std::string file_path) throw (ScriptRepoException&) = 0; 

    
    /**
       Upload the local file/folder to be available at the remote repository. 
       After this, this file/folder will be available for every, being published
       as with the same license as the mantid framework itself. 
       
       The user is not allowed to upload files that are marked as BOTH_CHANGED. It
       must first download the file (wich will make a copy of the local one), than
       update the downloaded file with the user own changes. At this point, he may
       published his file. 

       @note This operation requires internet connection.

       @param file_path for the file/folder to be published. For folders, it will 
              publish all the files inside the folder. 
              (Empty folders will not be accepted). 
       
       @param comment Allows to give information of the last changes, updates on
              a given file/folder. It differs from description, in the sense, that
              it may inform just what changed from the last version, while, the 
              description must provide information about the scope of the file. 
       
       @param author An string that may identify who is the responsible for changing
              that file. It may be a nick name, or an e-mail, or even a code, but, 
              it it necessary that it identifies who was responsible for changing 
              the file. 

       @param description is an optional argument, because, as 
              explained at @ref script-description-sec it may be already inside the 
              file. But, if the description is given, it will be inserted to the local
              file (if it is a script) or to the README file inside the folder.
     

      @exception ScriptRepoException may be triggered for an attempt to publish an 
                 empty folder, a file not present locally, because the file, or one
                 of the files inside the folder are marked as BOTH_CHANGED, or even
                 failure to connect to the remote repository.

    
     */
    virtual void upload(const std::string file_path, const std::string comment,
                const std::string author, 
                const std::string description = std::string()) throw (ScriptRepoException&) = 0;



    /**
       Connects to the remote repository, and checking for updates. 
       
       If necessary, it may create the local repository, specially, when first called. 
       
       This method, needs to know the remote URL wich will be provided
       by the Mantid ConfigService. 
      
       @attention The responsibility of executing this method periodically, is not of 
                  the ScriptRepository it self. The others methods may not respond
                  propperly, if this method is not executed. 
 
       @note This operation requires internet connection.

       @exception ScriptRepoException notifies mainly connection failure, but, 
                  may eventually, notify that the local repository may not be created. 
    */
    virtual void update(void) throw (ScriptRepoException&) = 0;    
  };

///shared pointer to the function base class
typedef boost::shared_ptr<ScriptRepository> ScriptRepository_sptr;

};
  
};

#endif // MANTID_API_SCRIPTREPOSITORY_H_
