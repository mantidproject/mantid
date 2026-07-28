// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX-License-Identifier: GPL-3.0+
#pragma once

#include <string>
#include <vector>

#include "MantidAPI/DllConfig.h"
#include "MantidKernel/DateAndTime.h"

namespace Mantid {
namespace API {

/// Information about the files inside the repository.
struct ScriptInfo {
  /// Identification of the author of the script.
  std::string author;
  /// Time of the last (remote) update of this file.
  Types::Core::DateAndTime pub_date;
  /// Whether the file is marked for auto-update.
  bool auto_update;
  /// Flag to indicate whether the entry is a directory.
  bool directory;
};

/** Represent the possible states for a given file:
      - REMOTE_ONLY: it exists only in the remote repository.
      - LOCAL_ONLY: it exists only in the local folder.
      - REMOTE_CHANGED: it has been changed remotely and may be updated.
      - LOCAL_CHANGED: it has been modified locally.
      - BOTH_CHANGED: modified both locally and remotely.
      - BOTH_UNCHANGED: the local file is a copy of the remote one.

    For folders, the meanings are slightly different:
      - REMOTE_ONLY: the folder exists only remotely.
      - LOCAL_ONLY: the folder exists only locally.
      - BOTH_UNCHANGED: no file inside the folder has a XXX_CHANGED status.
      - BOTH_CHANGED: the folder contains a file marked BOTH_CHANGED, or one
        file marked REMOTE_CHANGED together with another marked LOCAL_CHANGED.
      - REMOTE_CHANGED: at least one file is marked REMOTE_CHANGED and the rest
        are REMOTE_CHANGED or BOTH_UNCHANGED.
      - LOCAL_CHANGED: analogous to REMOTE_CHANGED, but for local changes.
 **/
enum SCRIPTSTATUS {
  BOTH_UNCHANGED = 0,
  REMOTE_ONLY = (1u << 0),
  LOCAL_ONLY = (1u << 1),
  REMOTE_CHANGED = (1u << 2),
  LOCAL_CHANGED = (1u << 3),
  BOTH_CHANGED = (REMOTE_CHANGED | LOCAL_CHANGED)
};

/**
The ScriptRepository class is intended to be used mainly by users, who
will want to share and download scripts for their analysis. As such,
the exceptions raised while operating must provide information that is
useful for them to understand what is happening, while the Mantid Team
must still be informed of what happened in more technical detail in order
to be able to deal with any resulting bugs.

To provide this functionality, ScriptRepoException is used. As a normal
std::exception (the default base used throughout the Mantid Project),
it allows Mantid to work as usual.

It also extends the usage of the exception by allowing more information
to be added. Below are some examples of how to throw exceptions.

\code

// Throw "Unknown Exception"
throw ScriptRepoException();

// After the system sets an errno value, for example EACCES,
// you could tell the user why they cannot download the file.
throw ScriptRepoException(EACCES, "You are not allowed to download scripts. "
"Please contact the administrator");

// For a more serious exception, you could provide the location where it
// was triggered.
throw ScriptRepoException(errno, "Critical Failure", __FILE__, __LINE__)
\endcode

The default ScriptRepoException::what method is used to show the user
message, while it is up to whoever is using the ScriptRepository to decide
whether to use the technical information available through
ScriptRepoException::systemError and ScriptRepoException::filePath.


*/
class MANTID_API_DLL ScriptRepoException : public std::exception {

public:
  /// default constructor
  ScriptRepoException(const std::string &info = std::string("Unknown Exception"))
      : m_systemError(""), m_userInfo(info), m_filepath("") {};

  ScriptRepoException(const std::string &info, const std::string &system, const std::string &file = std::string(),
                      int line = -1);

  /// Returns the message string.
  const char *what() const noexcept override;

  /// Returns the error description with technical details on the origin and
  /// cause.
  const std::string &systemError() const { return m_systemError; };
  /// Returns the file and position where the error was caused.
  const std::string &filePath() const { return m_filepath; };

private:
  /// Technical description of the error, returned by systemError().
  std::string m_systemError;
  /// User-facing message, returned by what().
  std::string m_userInfo;
  /// Path to the file where the error originated, returned by filePath().
  std::string m_filepath;
};

//----------------------------------------------------------------------
/**
Abstract class to manage the interaction between users and the scripts folder
(a mantid subproject).

Inside the mantid organisation (https://github.com/mantidproject) there is also
a subproject called scripts (https://github.com/mantidproject/scripts), created
to allow users to share their scripts, to allow the Mantid Team to distribute
analysis scripts to the Mantid community, and to help improve the quality of
the scripts used for data analysis.

The ScriptRepository interface aims to provide a simple way to interact with
that repository in order to promote its usage. To do so, it needs to:

  - List all scripts available in the repository
  - Download selected scripts
  - Check for updates

@note The repository is read-only from Mantid: scripts may be downloaded and
kept up to date, but they can not be published or deleted through this
interface.

ScriptRepository can list all the files inside the script repository through
ScriptRepository::listFiles. The file names alone may not be enough to decide
whether a file is useful, so the author, the description and the time the file
was last changed can be accessed through ScriptRepository::ScriptInfo.

The file list could become confusing if a large number of automatically
generated files were shown. To avoid this, you can edit the file patterns that
should be ignored when listing files; this is done through
ScriptRepository::setIgnorePatterns, and the current patterns can be checked
through ScriptRepository::ignorePatterns.

After looking at a file, you may want to download it through
ScriptRepository::download.

When working with the repository, a file may be local only (if the user created
it inside their folder and it has not been uploaded), locally modified, changed
remotely and out of date, or even modified both locally and remotely. Use
ScriptRepository::fileStatus to get this information for any file.

Finally, the ScriptRepository has to check the remote repository
(https://github.com/mantidproject/scripts) periodically, but it does so
indirectly through a mantid web service. It is the responsibility of external
tools to ensure this is done periodically, through
ScriptRepository::check4Update. For simplicity, this method is also used to
create the local repository if it does not exist.

Before using the ScriptRepository, it must be installed inside a local folder
(ScriptRepository::install). If ScriptRepository is not pointing at a valid
local repository, the method ScriptRepository::isValid will return false and no
method will be available except install. As good practice, it is worth ensuring
that the connection between the local object and the mantid web service is
available, through ScriptRepository::connect.

@note Exceptions are raised through ScriptRepoException to provide
      user-understandable information as well as technical details.


@note Mantid::API::ScriptRepositoryImpl implements this class.


@author Gesner Passos, ISIS, RAL
@date 11/12/2012
*/

/** @page ScriptRepositoryDescription The Description of the ScriptRepository
Files

@section script-description-sec Scripts, Folders and Files Description

The description of the files and scripts follows a convention for the
following types of file:

 - @ref pyscript-sec
 - @ref folders-sec
 - @ref readme-sec


@subsection pyscript-sec Python Scripts

If the script is a python file, then the description is the module __doc__
attribute. If this is not available, it will try to get the first group of
comments at the header of the file. For example, the following code:

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

If the entry is a folder, it will look for an __init__.py file to check whether
the folder is a python module. If it is, it will parse the __init__.py as in
section @ref pyscript-sec. Otherwise, it will look for a file whose name starts
with README and show it. For example, if the mantid repository path were passed,
it would show the content of its README.md file:

@verbatim
Mantid
======

The Mantid project provides (...)

@endverbatim

In this case, the author is taken from any line inside the README file that
starts with 'Author:'.


@subsection readme-sec README files

These work as described for folders in @ref folders-sec.

*/

class MANTID_API_DLL ScriptRepository {
public:
  /// Virtual destructor (always needed for abstract classes)
  virtual ~ScriptRepository() = default;
  /**
     Return information about the script through the Mantid::API::ScriptInfo
     struct.

     It may throw an exception if the file is present neither locally nor
     remotely.

     @param path : script path relative to the repository, or to the operating
     system.
     @return Mantid::API::ScriptInfo : Information about the script.

     @exception ScriptRepoException Mainly for scripts not found.

     @code
       ScriptSharing spt;
       ScriptInfo info = spt.info("README.md");
       // info.author : returns the file author.
     @endcode
   */
  virtual ScriptInfo info(const std::string &path) = 0;

  /** Provide the description of the file at the given path.
   *
   *  @param path: script path relative to the repository, or to the operating
   *system.
   *  @return the description of the file or folder.
   */
  virtual const std::string &description(const std::string &path) = 0;

  /// @deprecated Old name for info(); kept for compatibility and to be removed.
  ScriptInfo fileInfo(const std::string &path) { return info(path); }

  /**
     Return the list of files inside the repository. It provides a file-system
     like path for all the files and folders that are inside the local
     repository as well as remotely.

     @note The path uses a normal slash to separate folders.

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

     @return List of all the files available inside the repository, as a
     file-system path relative to the local repository.

     @exception May throw Invalid Repository if the local repository was not
     generated. In this case, ScriptRepository::install must be run (at least
     once).
   */
  virtual std::vector<std::string> listFiles() = 0;

  /**
     Create a copy of the remote file/folder inside the local repository.
     For a folder, it copies all the files inside the folder as well.

     If a file is reported to have local changes (@see
     ScriptRepository::fileStatus), the download will overwrite it with a copy
     of the remote file, but will keep a backup of the local file. This is
     reported by throwing an exception.

     For folders, the exception will also list all the files for which a backup
     was created.

     @param file_path of a file or folder to be downloaded.

     @throws ScriptRepoException to indicate that the file is not available
                remotely, or that a conflict was found.

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

  /** Check whether the local repository exists. If there is no local
    repository (it was never installed), isValid returns false and the only
    valid method is ScriptRepository::install.
  */
  virtual bool isValid() = 0;

  /** Install, at the given local_path, the resources that allow the
    ScriptRepository to operate locally. It may create any hidden files needed
    for the operation of this class.

    At the end, a new folder is created at local_path (if it does not already
    exist).

    @param local_path: path at which the folder will be created.

    @exception ScriptRepoException: if local_path cannot be created (for
    example, because of permission issues).

    */
  virtual void install(const std::string &local_path) = 0;

  /** Allow the ScriptRepository to double-check the connection with the web
  server. The webserverurl argument is optional; if omitted, it is taken from
  the settings defined for the ScriptRepository.

  This method ensures that the network and the link are available.

  @param webserverurl : url of the mantid web server.
  @exception ScriptRepoException: failure to connect to the web server, and the
  reason why.
  */
  virtual void connect(const std::string &webserverurl = "") = 0;

  /**
     Connect to the remote repository and check for updates.

     This method needs to know the remote URL, which must be available to the
     object before check4Update is called.

     It checks the state of the central repository and downloads all the files
     marked as AutoUpdate.

     @attention Executing this method periodically is not the responsibility of
                the ScriptRepository itself. The other methods may not respond
                properly if this method is not executed.

     @note This operation requires an internet connection.

     @exception ScriptRepoException mainly reports a connection failure, but may
                also report that the local repository could not be created.

     @return List of all the files that were automatically downloaded.
  */
  virtual std::vector<std::string> check4Update() = 0;

  /** Define the file patterns that will not be listed by listFiles.
      This lets the ScriptRepository avoid listing hidden files, automatically
      generated files and so on, giving the user a cleaner view of the
      repository.

      For example, pyc files are automatically generated and should be
      discarded. You could also ignore files ending with ~, which are temporary
      files on linux. The patterns are evaluated as csv regex patterns.

      To discard all pyc files, set: "*pyc".

      To discard all pyc files as well as hidden files and folders: "*pyc;\b\.*"

      @param patterns : csv regex patterns to be ignored when listing files.

      These settings must be preserved and remain available afterwards through
      the configuration system.
  */
  virtual void setIgnorePatterns(const std::string &patterns) = 0;

  /** Return the ignore patterns that were defined through
   * ScriptRepository::setIgnorePatterns.*/
  virtual std::string ignorePatterns() = 0;

  /** Define the AutoUpdate option, which determines whether a file is updated
      as soon as new versions are available in the central repository.

      This information is kept in a property system so that it is available
      afterwards.

      @param path : file or folder inside the local repository.

      @param option: auto-update flag. If true, new versions of the path will
     replace the local file as soon as they are available in the central
     repository.

      @return int: number of files changed (folders cascade to their contents).

      @exception ScriptRepoException : invalid entry.

  */
  virtual int setAutoUpdate(const std::string &path, bool option = true) = 0;
};

/// shared pointer to the ScriptRepository base class
using ScriptRepository_sptr = std::shared_ptr<ScriptRepository>;
} // namespace API
} // namespace Mantid
