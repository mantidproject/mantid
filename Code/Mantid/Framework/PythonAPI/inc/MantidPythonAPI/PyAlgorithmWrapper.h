#ifndef MANTIDAPI_PYTHONAPI_PYALGORITHMWRAPPER_H_
#define MANTIDAPI_PYTHONAPI_PYALGORITHMWRAPPER_H_

#include <MantidAPI/CloneableAlgorithm.h>
#include <MantidAPI/WorkspaceProperty.h>
#include <MantidAPI/MatrixWorkspace.h>
#include <MantidAPI/ITableWorkspace.h>

#include <MantidAPI/FileProperty.h>
#include <MantidKernel/NullValidator.h>

#include <MantidPythonAPI/PythonInterfaceFunctions.h>

#include <Poco/Void.h>

namespace Mantid  
{


namespace PythonAPI
{
  //---------------------------------------
  //Forward declarations
  //---------------------------------------

  /** 
    A wrapper around API::CloneableAlgorithm that allows inheritance from Python

    @author Martyn Gigg, Tessella Support Services plc
    @date 20/12/2009

    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class PyAlgorithmBase : public Mantid::API::CloneableAlgorithm
{

public:
  ///Constructor
  PyAlgorithmBase();

  /**
   * Return a reference to the logger object
   * @returns A reference to this algorithms logger object
   */
  Kernel::Logger & getLogger()
  {
    return g_log;
  }

  /**
   * Expose createSubAlgorithm to python so that a python algorithm can
   * create a sub-algorithm that will not log itself.
   * @param algo_name :: Name of the algorithm to create
   */
  boost::shared_ptr<API::IAlgorithm>  _createSubAlgorithm(const std::string algo_name)
  {
    return this->Algorithm::createSubAlgorithm(algo_name);
  }

  /**
   * Declare a property, templated on the value along with a validator
   * @param prop_name :: The name of the property
   * @param default_value :: The default value
   * @param validator :: A validator for this property
   * @param description :: A string describing the property
   * @param direction :: The direction
   */
  template<typename TYPE>
  void _declareProperty(const std::string & prop_name, TYPE default_value, 
			Kernel::IValidator<TYPE> & validator, 
			const std::string & description, const unsigned int direction)
  {
    this->IAlgorithm::declareProperty(prop_name, default_value, validator.clone(), description, direction);
  }
 
  /**
   * Declare a property, templated on the value
   * @param prop_name :: The name of the property
   * @param default_value :: The default value
   * @param description :: A string describing the property
   * @param direction :: The direction
   */
  template<typename TYPE>
  void _declareProperty(const std::string & prop_name, TYPE default_value, 
			const std::string & description, const unsigned int direction)
  {
    this->IAlgorithm::declareProperty(prop_name, default_value, description, direction);
  }

  /**
   * Declare a list property, templated on the list type
   * @param prop_name :: The name of the property
   * @param values :: A python list of values
   * @param validator :: A validator for the parameter
   * @param doc :: A string describing the property
   * @param direction :: The direction
   */
  template<typename TYPE>
  void _declareListProperty(const std::string & prop_name, boost::python::list values, 
    Kernel::IValidator<TYPE> & validator,const std::string &doc, const unsigned int direction)
  {
    // FIXME validator is not passed in - but I'm not sure this would ever reasonably be called.
    (void)validator;
    //Extract the values from the python list into a std vector
    this->IAlgorithm::declareProperty(prop_name, Conversions::toStdVector<TYPE>(values), doc, direction);
  }

  /**
   * Declare a list property, templated on the list type
   * @param prop_name :: The name of the property
   * @param values :: A python list of values
   * @param validator :: A validator for the parameter
   * @param doc :: A string describing the property
   * @param direction :: The direction
   */
  template<typename TYPE>
  void _declareListProperty(const std::string & prop_name, boost::python::list values,
    Kernel::IValidator<std::vector<TYPE> > & validator,const std::string &doc, const unsigned int direction)
  {
    (void)validator;
    //Extract the values from the python list into a std vector
    this->IAlgorithm::declareProperty(prop_name, Conversions::toStdVector<TYPE>(values), validator.clone(), doc, direction);
  }

  /**
   * Declare a list property, templated on the list type
   * @param prop_name :: The name of the property
   * @param values :: A python list of values
   * @param doc :: A string describing the property
   * @param direction :: The direction
   */
  template<typename TYPE>
  void _declareListProperty(const std::string & prop_name, boost::python::list values, const std::string &doc,
			    const unsigned int direction)
  {
    //Extract the values from the python list into a std vector
    this->IAlgorithm::declareProperty(prop_name, Conversions::toStdVector<TYPE>(values), doc, direction);
  }

  /**
   * Declare a MatrixWorkspace property
   * @param prop_name :: The name of the property
   * @param default_wsname :: A default name to use for the workspace name
   * @param description :: A string describing the property
   * @param direction :: The direction
   */
  void _declareMatrixWorkspace(const std::string & prop_name, const std::string & default_wsname, 
			       const std::string & description, const unsigned int direction)
  {
    this->Algorithm::declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>(prop_name, default_wsname, direction), description);
  }

  /**
   * Declare a MatrixWorkspace property with a validator
   * @param prop_name :: The name of the property
   * @param default_wsname :: A default name to use for the workspace name
   * @param validator :: A pointer to a validator object
   * @param description :: A string describing the property
   * @param direction :: The direction
   */
  void _declareMatrixWorkspace(const std::string & prop_name, const std::string & default_wsname,
			       Kernel::IValidator<API::MatrixWorkspace_sptr> & validator,
			       const std::string & description, const unsigned int direction)
  {
    this->Algorithm::declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>(prop_name, default_wsname, direction, validator.clone()), description);
  }

  /**
   * Declare a TableWorkspace property
   * @param prop_name :: The name of the property
   * @param default_wsname :: A default name to use for the workspace name
   * @param description :: A string describing the property
   * @param direction :: The direction
   */
  void _declareTableWorkspace(const std::string & prop_name, const std::string & default_wsname, 
			      const std::string & description, const unsigned int direction)
  {
    this->Algorithm::declareProperty(new API::WorkspaceProperty<API::ITableWorkspace>(prop_name, default_wsname, direction), description);
  }

  /**
   * Declare a FileProperty
   * @param prop_name :: The name of the property
   * @param default_value :: A default value for the filename
   * @param type :: The load/save type for the property, FileAction.{Save,OptionalSave,Load,OptionalLoad}
   * @param exts :: A Python list giving the extensions
   * @param description :: A string describing the property
   * @param direction :: The direction
   */
  void _declareFileProperty(const std::string & prop_name, const std::string & default_value, const unsigned int type, 
			    boost::python::list exts, const std::string & description, const unsigned int direction)
  {
    this->Algorithm::declareProperty(new API::FileProperty(prop_name, default_value, type, Conversions::toStdVector<std::string>(exts), direction), description);
  }
  
  /**
   * Retrieve a property
   * @param prop_name :: The name of the property
   * @returns The value of the property
   */
  template<typename TYPE>
  TYPE _getProperty(const std::string & prop_name)
  {
    TYPE retval = getProperty(prop_name);
    return retval;
  }

  /**
   * Retrieve a list property
   * @param prop_name :: The name of the property
   * @returns The values of the property as a python list
   */
  template<typename TYPE>
  std::vector<TYPE> _getListProperty(const std::string & prop_name)
  {
    std::vector<TYPE> retval = getProperty(prop_name);
    return retval;
  }
  
  /**
   * Special function to set MatrixWorkspace
   * @param prop_name :: The name of the property
   * @param workspace :: A pointer to the workspace
   */
  void _setMatrixWorkspaceProperty(const std::string & prop_name, API::MatrixWorkspace_sptr workspace)
  {
    this->IAlgorithm::setProperty(prop_name,workspace);
  }

  /**
   * Special function to set TableWorkspace
   * @param prop_name :: The name of the property
   * @param workspace :: A pointer to the workspace
   */
  void _setTableWorkspaceProperty(const std::string & prop_name, API::ITableWorkspace_sptr workspace)
  {
    this->IAlgorithm::setProperty(prop_name,workspace);
  }

};

// Declare the return handler for the callback functions
DECLARE_DEFAULTRETURN(PyAlgorithmBase*, NULL)

/**
 * A callback structure that can route calls into Python
 */
class PyAlgorithmCallback : public PyAlgorithmBase
{

public:
  ///Constructor
  PyAlgorithmCallback(PyObject *self);

  ///Destructor
  virtual ~PyAlgorithmCallback();

  /// Called when a delete is performed inside a shared pointer
  virtual void kill()
  {
    PythonLocker gil;
    if( FrameworkManagerProxy::requireGIL() )
    {
      gil.lock();
    }
    // !-----
    // This order is very important as the decref causes Python to call the destructor on this object
    // and the destructor checks the value of the killed flag
    // !-----
    m_ref_killed = true;
    Py_DECREF(m_self);
  }

  ///Overridden clone method
  virtual CloneableAlgorithm * clone();
  ///Overridden name method
  const std::string name() const; 
  ///Overridden version method
  int version() const ;
  /// Overridden category method
  const std::string category() const;

private:
  /// Overridden algorithm init method
  virtual void init();
  /// Overridden algorithm exec method
  virtual void exec();

  /// A pointer referring to the Python object 
  PyObject *m_self;
  /// A boolean flagging whether Py_DECREF has been called yet.
  /// A shared pointer can't use 'delete' here so it uses kill() instead. If that is the
  /// case then the destructor shouldn't decref the reference count as well
  bool m_ref_killed;
};


}
}

#endif //MANTIDAPI_PYTHONAPI_PYALGORITHMWRAPPER_H_
