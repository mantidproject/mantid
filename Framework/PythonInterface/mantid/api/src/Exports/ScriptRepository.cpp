// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/ScriptRepository.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include "MantidPythonInterface/core/PythonObjectInstantiator.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/docstring_options.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/tuple.hpp>

using namespace Mantid::API;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(ScriptRepository)

namespace {
/** @cond */

//------------------------------------------------------------------------------------------------------
/**
 * A Python friendly version that returns the registered functions as a list.
 * @param self :: Enables it to be called as a member function on the
 * FunctionFactory class
 */
PyObject *getListFiles(ScriptRepository &self) {
  std::vector<std::string> files = self.listFiles();

  PyObject *registered = PyList_New(0);
  for (const auto &file : files) {
    PyObject *value = to_python_value<const std::string &>()(file);
    if (PyList_Append(registered, value))
      throw std::runtime_error("Failed to insert value into PyList");
  }

  return registered;
}

tuple getInfo(ScriptRepository &self, const std::string &path) {
  ScriptInfo info = self.info(path);
  return boost::python::make_tuple<std::string>(info.author,
                                                info.pub_date.toSimpleString());
}

PyObject *getStatus(ScriptRepository &self, const std::string &path) {
  SCRIPTSTATUS st = self.fileStatus(path);
  PyObject *value;
  switch (st) {
  case BOTH_UNCHANGED:
    value = to_python_value<char const *&>()("BOTH_UNCHANGED");
    break;
  case REMOTE_ONLY:
    value = to_python_value<char const *&>()("REMOTE_ONLY");
    break;
  case LOCAL_ONLY:
    value = to_python_value<char const *&>()("LOCAL_ONLY");
    break;
  case REMOTE_CHANGED:
    value = to_python_value<char const *&>()("REMOTE_CHANGED");
    break;
  case LOCAL_CHANGED:
    value = to_python_value<char const *&>()("LOCAL_CHANGED");
    break;
  case BOTH_CHANGED:
    value = to_python_value<char const *&>()("BOTH_CHANGED");
    break;
  default:
    value = to_python_value<char const *&>()("BOTH_UNCHANGED");
    break;
  }
  return value;
}

PyObject *getDescription(ScriptRepository &self, const std::string &path) {
  return to_python_value<const std::string &>()(self.description(path));
}

/** @endcond */
} // namespace

void export_ScriptRepository() {

  register_ptr_to_python<boost::shared_ptr<ScriptRepository>>();

  // reset the option to
  docstring_options local_docstring_options(true, true, false);

  const char *repo_desc =
      "Manage the interaction between the users and the Script folder (mantid subproject). \n\
\n\
Inside the mantid repository (https://github.com/mantidproject) there is also a subproject \n\
called scripts (https://github.com/mantidproject/scripts), created to allow users to share \n\
their scripts, as well as to allow Mantid Team to distribute to the Mantid community \n\
scripts for analysis and also to enhance the quality of the scripts used for the sake of \n\
data analysis. \n\
\n\
The ScriptRepository aims to provide a simple way to interact with that repository in \n\
order to promote its usage. In order to enhance the usage, it is necessary:\n\
  \n\
 - List all scripts available at the repository\n\
 - Download selected scripts. \n\
 - Check for updates\n\
 - Allow to publish users scripts/folders. \n\
\n\
Basically, you will need to install the repository through ::install(local_path). \n\
After, you will be able to ::listFiles inside your repository to check the files that are \n\
available, eventually, you may want to check the information of a single entry \n\
::fileInfo(path). You may also be interested to know if there is a new version available, \n\
or if the file has been modified since last downloaded (::fileStatus(path)). You may want \n\
to download scripts through ::download(path), or check for updates through ::update(). \n\
\n\
'''NOTE:''' Upload is not implemented yet.\n";

  const char *list_files_desc =
      "Return an array with all the entries inside the repository. \n\
\n\
Folder os files, locally or remotely, all will be listed together through the listFiles. \n\
The listFiles has another function, which is related to update the internal cache about \n\
the status and information of the files. So, local changes or remote changes will only be \n\
available to fileStatus of fileInfo after listFiles.\n\
\n\
Returns:\n\
\n\
  list: entries inside the repository.\n";

  const char *file_info_desc =
      "Return general information from the entries inside ScriptRepository. \n\
\n\
The author, description and publication date are available through this method. \n\
\n\
Arguments:\n\
\n\
  - path(str): Path to the entry.\n\
\n\
Returns:\n\\n\
  - tuple: (author, last publication date)\n";

  const char *file_description_desc =
      "Return description of the entry inside ScriptRepository. \n\
\n\
Arguments:\n\
\n\
  - path: Path to the entry.\n\
\n\
Return:\n\
   - string with the description\n";

  const char *file_status_desc = "Return the status of a given entry.\n\
\n\
The following status are applied to the entries:\n\
  - REMOTE_ONLY: The file is only at the central repository\n\
  - LOCAL_ONLY: The file is only in your file system\n\
  - BOTH_UNCHAGED: The file is in your file system and remotely, and are iqual.\n\
  - REMOTE_CHANGED: A new version is available for this file.\n\
  - LOCAL_CHANGED: You have edited the file\n\
  - BOTH_CHANGED: There is a new version and you have changed as well\n\
\n\
'''NOTE:''' ScriptRepository recognizes changes locally and remotely only through \n\
listFiles method.\n\
Arguments:\n\
\n\
  - path (str): The path for the entry.\n\
\n\
Returns:\n\
\n\
  - str: Status of the entry.\n";

  const char *download_desc =
      "Download from repository into your local file system.\n\
\n\
You may give a file or folder. If the later is given, ScriptRepository will \n\
download all the files inside that folder from the remote repository to you.\n\
\n\
Arguments:\n\
\n\
   - path (str): Path for the entry do download";

  const char *update_desc = "Check for updates at the remote repository.\n\
\n\
New versions of the files may be available, and the update method will check the \n\
remote repository to see if there is anything new. It will not download new versions \n\
of the available files unless you ask to do so. You should do this often to check if \n\
there is a new script to solve your problem ;)";

  const char *install_desc =
      "Install the ScriptRepository in your local file system\n\
\n\
The installation of the ScriptRepository is very simple. You must only provide a path, \n\
existing or new folder, where the ScriptRepository will put the database it requires to \n\
run itself. The installation requires network connection, to connect to the central \n\
repository but usually takes very few moments to be installed. After installing, all the \n\
others methods will be available.\n\
Arguments:\n\
\n\
  - path (str): An existing or path to a new folder to be created, where the ScriptRepository will install itself.\
";

  ///@todo better description
  class_<ScriptRepository, boost::noncopyable>("ScriptRepository", repo_desc,
                                               no_init)
      .def("install", &ScriptRepository::install,
           (arg("self"), arg("local_path")), install_desc)
      .def("listFiles", &getListFiles, arg("self"), list_files_desc)
      .def("fileInfo", &getInfo, (arg("self"), arg("path")), file_info_desc)
      .def("description", &getDescription, (arg("self"), arg("path")),
           file_description_desc)
      .def("fileStatus", &getStatus, (arg("self"), arg("path")),
           file_status_desc)
      .def("download", &ScriptRepository::download,
           (arg("self"), arg("file_path")), download_desc)
      .def("update", &ScriptRepository::check4Update, arg("self"), update_desc);
}
