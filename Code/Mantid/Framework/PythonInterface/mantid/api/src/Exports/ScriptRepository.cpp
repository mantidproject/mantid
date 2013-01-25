#include "MantidAPI/ScriptRepository.h"
#include "MantidPythonInterface/kernel/PythonObjectInstantiator.h"
#include "MantidPythonInterface/kernel/SharedPtrToPythonMacro.h"

//#include "MantidPythonInterface/api/PythonAlgorithm/AlgorithmWrapper.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/tuple.hpp>
#include <boost/python/docstring_options.hpp>

// Python frameobject. This is under the boost includes so that boost will have done the
// include of Python.h which it ensures is done correctly
#include <frameobject.h>

using Mantid::API::ScriptRepository; 
using namespace Mantid::API;
using namespace boost::python;
using boost::python::tuple;
namespace
{
  ///@cond

  //------------------------------------------------------------------------------------------------------
  /**
   * A Python friendly version that returns the registered functions as a list.
   * @param self :: Enables it to be called as a member function on the FunctionFactory class
   */
  PyObject * getListFiles(ScriptRepository & self)
  {
    std::vector<std::string> files = self.listFiles();

    PyObject *registered = PyList_New(0);
    for (auto file = files.begin(); file != files.end(); ++file)
    {
      PyObject *value = PyString_FromString(file->c_str());
      if (PyList_Append(registered, value))
        throw std::runtime_error("Failed to insert value into PyList");
    }

    return registered;
    }

  PyObject * getName(ScriptRepository & self){
    UNUSED_ARG(self);
    PyObject * value = PyString_FromString("GitMyScriptRepository"); 
    return value;
  }

  tuple getInfo(ScriptRepository & self, const std::string path){
    ScriptInfo info = self.fileInfo(path);
    return  make_tuple<std::string>(info.author, info.description);
  }

  PyObject * getStatus(ScriptRepository & self, const std::string path){
    SCRIPTSTATUS st = self.fileStatus(path); 
    PyObject * value; 
    switch(st){
    case BOTH_UNCHANGED:
      value = PyString_FromString("BOTH_UNCHANGED"); 
      break;
    case REMOTE_ONLY:
      value = PyString_FromString("REMOTE_ONLY");
      break;
    case LOCAL_ONLY:
      value = PyString_FromString("LOCAL_ONLY"); 
      break; 
    case REMOTE_CHANGED:
      value = PyString_FromString("REMOTE_CHANGED");
      break;
    case LOCAL_CHANGED:
      value = PyString_FromString("LOCAL_CHANGED");
      break;
    case BOTH_CHANGED:
      value = PyString_FromString("BOTH_CHANGED");
    break;
    default:
      value = PyString_FromString("BOTH_UNCHANGED"); 
      break;
    }
    return value;
  }

  ///@endcond
}

void export_ScriptRepository()
{

  REGISTER_SHARED_PTR_TO_PYTHON(ScriptRepository);
  //  class_<ScriptRepository, boost::noncopyable>("ScriptRepository", "Base-class for ScriptRepository", no_init);

  // reset the option to 
 docstring_options local_docstring_options(true, true, false);

const char * repo_desc = 
"Manage the interaction between the users and the Script folder (mantid subproject). \n\
\n\
Inside the mantid repository (https://github.com/mantidproject) there is also a subproject called \n\
scripts (https://github.com/mantidproject/scripts), created to allow users to share their scripts, \n\
as well as to allow Mantid Team to distribute to the Mantid community scripts for analysis and \n\
also to enhance the quality of the scripts used for the sake of data analysis. \n\
\n\
The ScriptSharing class aims to provide a simple way to interact with that repository in order to \n\
promote its usage. In order to enhance the usage, it is necessary:\n\
  \n\
 - List all scripts available at the repository\n\
 - Download selected scripts. \n\
 - Check for updates\n\
 - Allow to publish users scripts/folders. \n";

  ///@todo beter description
  class_<ScriptRepository,boost::noncopyable>("ScriptRepository",  repo_desc, no_init)
    .def("name", &getName, "Return the name of the ScriptRepository")
    .def("listFiles",&getListFiles,"Return the list of the files inside the Repository")
    .def("fileInfo",&getInfo,"Return description of the file inside the repository")
    .def("download",&ScriptRepository::download,"Download file or folder ")
    .def("fileStatus",&getStatus,"Return the status")
    .def("upload",&ScriptRepository::upload,"Publish your script")
    .def("update",&ScriptRepository::update,"Check if there is update remotely");
    
}
