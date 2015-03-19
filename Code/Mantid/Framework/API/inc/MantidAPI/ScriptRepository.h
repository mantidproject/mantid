#ifndef MANTID_API_SCRIPTREPOSITORY_H_
#define MANTID_API_SCRIPTREPOSITORY_H_

#include <string>
#include <vector>

#include "MantidKernel/DateAndTime.h"
#include "MantidAPI/DllConfig.h"
#include <vector>

namespace Mantid {
namespace API {

/// Information about the files inside the repository.
struct ScriptInfo {
  /// Identification of the author of the script.
  std::string author;
  /// Time of the last update of this file (remotelly)
  Kernel::DateAndTime pub_date;
  /// Marked for auto update
  bool auto_update;
  /// Directory Flag to indicate if the entry is a directory.
  bool directory;
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
      - BOTH_CHANGED: if inside the folder it has one file with BOTH_CHANGED
 status
      or one REMOTE_CHANGED with another LOCAL_CHANGED.
      - REMOTE_CHANGED: at least one marked with REMOTE_CHANGED and others as
      REMOTE_CHANGED or BOTH_UNCHANGED.
      - LOCAL_CHANGED: analogous to REMOTE_CHANGED, but locally.
 **/
enum SCRIPTSTATUS {
  BOTH_UNCHANGED = 0,
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
throw ScriptRepoException(EACCES, "You can allowed to download scripts. Please,
contact the administrator");

// For more serious exception, you could provide the location where it
// were triggered.
throw ScriptRepoException(errno, "Critical Failure", __FILE__, __LINE__)
\endcode

The default ScriptRepoException::what method will be used to show the user
message, while it is up to whom is using the ScriptRepository to decide on
using or not the techinical information through
ScriptRepoException::systemError,
ScriptRepoException::filePath.


*/
class MANTID_API_DLL ScriptRepoException : public std::exception {

public:
  /// default constructor
  ScriptRepoException(
      const std::string &info = std::string("Unknown Exception"))
      : _system_error(""), _user_info(info), _file_path(""){};

  ScriptRepoException(int err_, const std::string &info = std::string(),
                      const std::string &file = std::string(), int line = -1);

  ScriptRepoException(const std::string &info, const std::string &system,
                      const std::string &file = std::string(), int line = -1);

  /// Destructor
  ~ScriptRepoException() throw() {}

  /// Returns the message string.
  const char *what() const throw();

  /// Returns the error description with technical details on the origin and
  /// cause.
  const std::string &systemError() const { return _system_error; };
  /// Returns the file and position where the error was caused.
  const std::string &filePath() const { return _file_path; };

private:
  /// The message returned by what()
  std::string _system_error;
  std::string _user_info;
  std::string _file_path;
};

//----------------------------------------------------------------------
/**
Abstract Class to manage the interaction between the users and the Script folder
(mantid subproject).

Inside the mantid repository (https://github.com/mantidproject) there is also a
subproject called
scripts (https://github.com/mantidproject/scripts), created to allow users to
share their scripts,
as well as to allow Mantid Team to distribute to the Mantid community scripts
for analysis and
also to enhance the quality of the scripts used for the sake of data analysis.

The ScriptRepository interface aims to provide a simple way to interact with
that repository in order to
promote its usage. In order to enhance the usage, it is necessary:

  - List all scripts available at the repository
  - Download selected scripts.
  - Check for updates
  - Allow to publish users scripts/folders.

ScriptRepository may show all the files inside the script repository through
ScriptRepository::listFiles.
Only the names of the files may not be sufficient, to help deciding if the file
would be usefull or
not, so, the author, the description and when the file was last changed can be
accessed through
ScriptRepository::ScriptInfo.

The list of the files could become confusing if a large amount of automatically
created files is installed.
In order to avoid this, it is possible to edit the file patterns that should be
ignored when listing files,
this is done through ScriptRepository::setIgnorePatterns, and you can check this
settings through the
ScriptRepository::ignorePatterns.

After looking at a file, you may be interested in downloading it through
ScriptRepository::download.

When working with the repository, the files may be local only, if the user
created a file inside
his folder, it may not be downloaded, it can be locally modified, or changed
remotely and
not up-to-date, or even, being modified locally and remotely. Use
ScriptRepository::fileStatus in
order to get these informations of any file.

The user may decide to upload one of his file or folder, to share it. This is
possible through
the ScriptRepository::upload. The same command may be used to publish also some
updates made to an
already shared file.

Finally, the ScriptRepository have to check periodically the remote repository
(https://github.com/mantidproject/scripts), but it will be done indirectly
through a mantid web service,
it is the responsibility of external tools to ensure
it is done periodically, through ScriptRepository::check4Update. For the sake of
simplicity, this method
is used also to create the local repository in case it does not exists.

Before using the ScriptRepository, it must be installed inside a local folder
(ScriptRepository::install).
If ScriptRepository is not created pointing to a valid local repository, the
methog ScriptRepository::isValid
will return false, and no method will be available, except, install.
As a good practice, it is good to ensure that the connection between the local
object and the
mantid web service is available, through the ScriptRepository::connect.

@note Exceptions will be triggered through ScriptRepoException in order user
      understandable information as well as techinical details.


@note Mantid::API::ScriptRepositoryImpl implements this class.


@author Gesner Passos, ISIS, RAL
@date 11/12/2012

Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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

/** @page ScriptRepositoryDescription The Description of the ScriptRepository
Files

@section script-description-sec Scripts, Folders and Files Description

The description of the files and scripts will obey an agreement for the
following type of files:

 - @ref pyscript-sec
 - @ref folders-sec
 - @ref readme-sec


@subsection pyscript-sec Python Scripts

If the script is as python file, then the description will be the module __doc__
attribute.
If this information is not availabe, them, it will try to get the first group of
comments, at the
header of the file. For
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

Another example, consider this python file:

@code{.py}
#!/usr/bin/env python

## This module is responsible to display a
## 'Hello world' greeting.

print 'Hello world'
@endcode

Will show the description as follow:

@verbatim
This module is responsible to display a
'Hello world' greeting.
@endverbatim

@subsection folders-sec Folders

If the script is a folder, it will try to find a file __init__.py, so to check
if this folder is
a python module. If it does, it will parse the __init__.py as in section @ref
pyscript-sec. Otherwise,
it will look for a file starting with name README, and will show it.
For example, if the mantid repository path would be passed,
it would show the content of its README.md file:

@verbatim
Mantid
======

The Mantid project provides (...)

@endverbatim

In this case, author should be any name found inside the README file that starts
with
the following line: 'Author:'.


@subsection readme-sec README files

They will work as was expected for folders @ref folders-sec.

*/

class MANTID_API_DLL ScriptRepository {
public:
  /// Virtual destructor (always needed for abstract classes)
  virtual ~ScriptRepository(){};

  /**
     Return the information about the script through the Mantid::API::ScriptInfo
     struct.

     It may throw exception if the file is not presented locally, or even
     remotelly.

     @param path : Script path related to the repository, or to operate system.
     @return Mantid::API::ScriptInfo : Information about the script.

     @exception ScriptRepoException Mainly for scripts not found.

     @code
       ScriptSharing spt;
       ScriptInfo info = spt.info("README.md");
       // info.author : returns the file author.
     @endcode
   */
  virtual ScriptInfo info(const std::string &path) = 0;

  /** Provide the description of the file given the path
   *
   *  @param path: Script path related to the reposititory, or the operative
   *system.
   *  @return the description of the file or folder.
   */
  virtual const std::string &description(const std::string &path) = 0;

  /// @deprecated Previous version, to be removed.
  ScriptInfo fileInfo(const std::string &path) { return info(path); }

  /**
     Return the list of files inside the repository. It provides a file-system
     like path for all the files, folders that are inside the local repository
     as
     well as remotely.

     @note The path used a normal slash to separate folders.

     Consider the following repository:

     @verbatim
     README.md
     folderA
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
     folderA
     folderA/fileB
     fileC
     NewFile
     @endverbatim

     @return List of all the files available inside the repository as a file
     system
     path relative to the local repository.

     @exception May throw Invalid Repository if the local repository was not
     generated. In this case, it is necessary to execute the
     ScriptRepository::install (at least once).
   */
  virtual std::vector<std::string> listFiles() = 0;

  /**
     Create a copy of the remote file/folder inside the local repository.
     For folder, it will copy all the files inside the folder as well.

     If one file is reported to have local changes (@see
     ScriptRepository::fileStatus)
     the download will make a copy of the remote file, but will preserve a
     backup
     of the local file. This incident will be reported through throwing an
     exception.

     For folders, the exception will also list all the files that a backup was
     created.

     @param file_path of a file or folder to be downloaded.

     @throws ScriptRepoException to indicate file is not available at
                remotely or to indicate that a conflict was found.

   */
  virtual void download(const std::string &file_path) = 0;

  /**
     Return the status of the file, according to the status defined in
     Mantid::API::SCRIPTSTATUS.

     @param file_path: for file/folder
     @return SCRIPTSTATUS : of the given file/folder
     @exception ScriptRepoException to indicate that file is not available.
   */
  virtual SCRIPTSTATUS fileStatus(const std::string &file_path) = 0;

  /** Check if the local repository exists. If there is no local repository,
    the repository was never installed, the isValid will return false, and the
    only method valid is ScriptRepository::install.
  */
  virtual bool isValid(void) = 0;

  /** Install the necessary resources at the local_path given that allows the
    ScriptRepository to operate
    locally.
    It is allowed to create hidden files that would be necessary for the
    operation of this class.

    At the end, a new folder is created (if it does not exists already), with
    the given local_path given.

    @param local_path: path where the folder (having the same name given) will
    be created.

    @exception ScriptRepoException: If the local_path may not be created
    (Permission issues).

    */
  virtual void install(const std::string &local_path) = 0;

  /** Allow the ScriptRepository to double check the connection with the web
  server.
  An optional argument is allowed webserverurl, but, it may be taken from the
  settings defined for the ScriptRepository.

  This method ensures that the network and the link is available.

  @param webserverurl : url of the mantid web server.
  @exception ScriptRepoException: Failure to connect to the web server and the
  reason why.
  */
  virtual void connect(const std::string &webserverurl = "") = 0;

  /**
     Connects to the remote repository checking for updates.

     This method, needs to know the remote URL wich must be available to the
     object before
     calling the check4update.

     It must check the state of the central repository and download all the
     files marked as
     AutoUpdate.

     @attention The responsibility of executing this method periodically, is not
     of
                the ScriptRepository it self. The others methods may not respond
                propperly, if this method is not executed.

     @note This operation requires internet connection.

     @exception ScriptRepoException notifies mainly connection failure, but,
                may eventually, notify that the local repository may not be
     created.

     @return List with all the files automatically downloaded.
  */
  virtual std::vector<std::string> check4Update(void) = 0;

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

     @param author An string that may identify who is the responsible for
    changing
            that file. It may be a nick name, or an e-mail, or even a code, but,
            it it necessary that it identifies who was responsible for changing
            the file.

     @param email An string that identifies the email of the author.


    @exception ScriptRepoException may be triggered for an attempt to publish an
               empty folder, a file not present locally, because the file, or
    one
               of the files inside the folder are marked as BOTH_CHANGED, or
    even
               failure to connect to the remote repository.


   */
  virtual void upload(const std::string &file_path, const std::string &comment,
                      const std::string &author, const std::string &email) = 0;

  /**
     Delete the file from the remote repository (it does not touch the local
    copy).
     After this, the file will not be available for anyone among the users. As
    so,
     it is required a justification of why to remove (the comment), and the
    current
     rule accept that only the owner of the file (which is considered the last
    one
     to use the file) is allowed to remove it.

     The file will be removed from the central repository (git)

     @note This operation requires internet connection.

     @param file_path for the file to be deleted. It will not accept deleting
    folders for
            security reason.

     @param comment The reson of why deleting this entry.

     @param author An string that may identify who is requesting to delete the
    file.
            It accept only the last author to remove it.

     @param email An string that identifies the email of the author.

    @exception ScriptRepoException may be triggered for an attempt to delete
    folders,
               or a non existent file, or not allowed operation, or any network
    erros.
   */
  virtual void remove(const std::string &file_path, const std::string &comment,
                      const std::string &author, const std::string &email) = 0;

  /** Define the file patterns that will not be listed in listFiles.
      This is important to force the ScriptRepository to not list hidden files,
      automatic generated files and so on. This helps to present to the user a
      clean presentation of the Repository

      For example, there are the pyc files that are automatically generated, and
      should be discarded. We could set also to ignore files that end with ~,
      temporary files in linux. The patterns will be evaluated as a csv regex
     patterns.

      To discard all pyc files, set: "*pyc".

      To discard all pyc files and hidden files and folders: "*pyc;\b\.*"

      @param patterns : csv regex patterns to be ignored when listing files.

      This settings must be preserved, and be available after trough the
     configure system.
  */
  virtual void setIgnorePatterns(const std::string &patterns) = 0;

  /** Return the ignore patters that was defined through
   * ScriptRepository::setIgnorePatterns*/
  virtual std::string ignorePatterns(void) = 0;

  /** Define the AutoUpdate option, which define if a file will be updated as
     soon as
      new versions are available at the central repository.

      This information will be kept in a property system, in order to be
     available afterwards.

      @param path : file or folder inside the local repository

      @param option: flag to set for auto-update, or not. If true, new versions
     of the path will replace the local file as soon as they are available at
     the central repository.

      @return int: number of files changed (because of the cascading of folders)

      @exception ScriptRepoException : Invalid entry.

  */
  virtual int setAutoUpdate(const std::string &path, bool option = true) = 0;
};

/// shared pointer to the function base class
typedef boost::shared_ptr<ScriptRepository> ScriptRepository_sptr;
}
}

#endif // MANTID_API_SCRIPTREPOSITORY_H_
