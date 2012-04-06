#ifndef MANTID_PYTHONINTERFACE_CALLMETHOD_H_
#define MANTID_PYTHONINTERFACE_CALLMETHOD_H_
/** 
    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
*/
#include "MantidPythonInterface/kernel/Environment/Threading.h"
#include "MantidPythonInterface/kernel/Environment/WrapperHelpers.h"

#include <boost/python/class.hpp>
#include <boost/python/call_method.hpp>


namespace Mantid { namespace PythonInterface {
  namespace Environment
  {

    /// Handle a Python error state
    DLLExport void translateErrorToException(const bool withTrace = true);

    ///
    /// A wrapper around the boost::call_method to ensure that
    /// the GIL is locked while the call is happening
    ///

    /** @name No argument Python calls */
    //@{
    /**
     * Perform a call to a python function that takes no arguments and returns a value
     */
    template<typename ResultType>
    struct DLLExport CallMethod_NoArg
    {
      /**
       * Dispatch a call to the method on the given object. If the method does not exist
       * then return the defaultValue
       * @param self The object containing the method definition
       * @param funcName The method name
       * @param defaultValue A default value if the method does not exist
       * @return
       */
      static ResultType dispatchWithDefaultReturn(PyObject *self, const char * funcName, const ResultType & defaultValue)
      {
        GlobalInterpreterLock gil;
        if(Environment::typeHasAttribute(self, funcName))
        {
          try
          {
            return boost::python::call_method<ResultType>(self, funcName);
          }
          catch(boost::python::error_already_set&)
          {
            translateErrorToException();
          }
        }
        return defaultValue;
      }

      /**
       * Dispatch a call to the method on the given object. If the method does not exist
       * then raise a std::runtime_error exception
       * @param self The object containing the method definition
       * @param funcName The method name
       * @param errorMsg An error message to pass to the generated exception
       * @return
       */
      static ResultType dispatchWithException(PyObject *self, const char * funcName, const char * errorMsg)
      {
        GlobalInterpreterLock gil;
        if(Environment::typeHasAttribute(self, funcName))
        {
          try
          {
            return boost::python::call_method<ResultType>(self, funcName);
          }
          catch(boost::python::error_already_set&)
          {
            translateErrorToException();
          }
        }
        else
        {
          throw std::runtime_error(errorMsg);
        }
        return ResultType();
      }
    };

    ///Specialization for void return type
    template<>
    struct DLLExport CallMethod_NoArg<void>
    {
      /**
       * Dispatch a call to the method on the given object. If the method does not exist
       * then do nothing
       * @param self The object containing the method definition
       * @param funcName The method name
       * @param defaultValue A default value if the method does not exist
       * @return
       */
      static void dispatchWithDefaultReturn(PyObject *self, const char * funcName)
      {
        GlobalInterpreterLock gil;
        if(Environment::typeHasAttribute(self, funcName))
        {
          try
          {
            boost::python::call_method<void>(self, funcName);
          }
          catch(boost::python::error_already_set&)
          {
            translateErrorToException();
          }
        }
      }
      /**
       * Dispatch a call to the method on the given object. If the method does not exist
       * then raise a runtime_error
       * @param self The object containing the method definition
       * @param funcName The method name
       * @param errorMsg An error message if the method does not exist
       * @return
       */
      static void dispatchWithException(PyObject *self, const char * funcName, const char * errorMsg)
      {
        GlobalInterpreterLock gil;
        if(Environment::typeHasAttribute(self, funcName))
        {
          try
          {
            boost::python::call_method<void>(self, funcName);
          }
          catch(boost::python::error_already_set&)
          {
            translateErrorToException();
          }
        }
        else
        {
          throw std::runtime_error(errorMsg);
        }
      }
    };
    //@}

  }
}}

#endif //MANTID_PYTHONINTERFACE_CALLMETHOD_H_
