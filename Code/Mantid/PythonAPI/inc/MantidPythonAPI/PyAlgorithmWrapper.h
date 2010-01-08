#ifndef MANTIDAPI_PYTHONAPI_PYALGORITHMWRAPPER_H_
#define MANTIDAPI_PYTHONAPI_PYALGORITHMWRAPPER_H_

#include <MantidAPI/Algorithm.h>
#include <MantidAPI/WorkspaceProperty.h>
#include <MantidAPI/MatrixWorkspace.h>
#include <MantidAPI/ITableWorkspace.h>

#include <MantidKernel/FileProperty.h>

#include <boost/python.hpp>

namespace Mantid  
{

//---------------------------------------
//Forward declarations
//---------------------------------------

namespace PythonAPI
{
/** 
    A wrapper around API::Algorithm that allows inheritance from Python

    @author Martyn Gigg, Tessella Support Services plc
    @date 20/12/2009

    Copyright &copy; 2008 STFC Rutherford Appleton Laboratories

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>    
*/
struct PyAlgorithmWrapper : Mantid::API::Algorithm, boost::python::wrapper<API::Algorithm>
{
public:
  /// Declare a property
  template<typename TYPE>
  void _declareProperty(const std::string & prop_name, TYPE default_value, const std::string & description, 
			const unsigned int direction)
  {
    Algorithm::declareProperty(prop_name, default_value, description, direction);
  }

  ///Declare a matrix workspace
  void _declareMatrixWorkspace(const std::string & prop_name, const std::string & default_wsname, const std::string & description,
			       const unsigned int direction)
  {
    Algorithm::declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>(prop_name, default_wsname, direction), description);
  }

  ///Declare a table workspace
  void _declareTableWorkspace(const std::string & prop_name, const std::string & default_wsname, const std::string & description,
			      const unsigned int direction)
  {
    Algorithm::declareProperty(new API::WorkspaceProperty<API::ITableWorkspace>(prop_name, default_wsname, direction), description);
  }

  /// File property
  void _declareFileProperty(const std::string & prop_name, const std::string & default_value, const unsigned int type, 
			    boost::python::list exts, const std::string & description, 
			    const unsigned int direction)
  {
    Algorithm::declareProperty(new Kernel::FileProperty(prop_name, default_value, type, convertToStdVector<std::string>(exts), direction), description);
  }
  
  ///getProperty function that gives back the correct type
  template<typename TYPE>
  TYPE _getProperty(const std::string & prop_name)
  {
    TYPE retval = Algorithm::getProperty(prop_name);
    return retval;
  }
  
  /// Set a matrix workspace property
  void _setMatrixWorkspaceProperty(const std::string & prop_name, boost::python::object pyobject)
  {
    API::MatrixWorkspace_sptr workspace = boost::python::extract<API::MatrixWorkspace_sptr>(pyobject);
    Algorithm::setProperty(prop_name,workspace);
  }

  /// Set a table workspace property
  void _setTableWorkspaceProperty(const std::string & prop_name, boost::python::object pyobject)
  {
    API::ITableWorkspace_sptr workspace = boost::python::extract<API::ITableWorkspace_sptr>(pyobject);
    Algorithm::setProperty(prop_name,workspace);
  }

private:
  /// Overridden algorithm init method
  virtual void init()
  {
    dispatch_python_call_noarg("PyInit");
  }
  /// Overridden algorithm exec method
  virtual void exec()
  {
    dispatch_python_call_noarg("PyExec");
  }

  ///Overridden name method
  virtual const std::string name() const 
  {
    return dispatch_python_call_noarg<std::string>("name");
  }
  ///Overridden version method
  virtual const int version() const 
  {
    return dispatch_python_call_noarg<int>("version");
  }
  /// Overridden category method
  virtual const std::string category() const 
  {
    return dispatch_python_call_noarg<std::string>("category");
  }

  /// Templated function call up to python for a no argument function
  template<typename TYPE>
  TYPE dispatch_python_call_noarg(const std::string & pyfunction_name) const
  {
    if( boost::python::override dispatcher = this->get_override(pyfunction_name.c_str()) )
    {
      PyGILState_STATE gstate = PyGILState_Ensure();
      TYPE retval = dispatcher();
      PyGILState_Release(gstate);
      return retval;
    }
    return TYPE();
  }
  ///Specialized dispatcher for void
  void dispatch_python_call_noarg(const std::string & pyfunction_name)
  {
    if( boost::python::override dispatcher = this->get_override(pyfunction_name.c_str()) )
    {
      PyGILState_STATE gstate = PyGILState_Ensure();
      dispatcher();
      PyGILState_Release(gstate);
    }
  }
  
  /// Convert a Boost Python list to a std::vector of the requested type
  template<typename TYPE>
  std::vector<TYPE> convertToStdVector(const boost::python::list & pylist)
  {
    int length = boost::python::extract<int>(pylist.attr("__len__")());
    std::vector<TYPE> seq_std(length, TYPE());
    if( length == 0 )
    {
      return seq_std;
    }

    for( int i = 0; i < length; ++i )
    {
      boost::python::extract<TYPE> cppobj(pylist[i]);
      if( cppobj.check() )
      {
	seq_std[i] = cppobj();
      }
    }
    return seq_std;
  }

};

}
}

#endif //MANTIDAPI_PYTHONAPI_PYALGORITHMWRAPPER_H_
