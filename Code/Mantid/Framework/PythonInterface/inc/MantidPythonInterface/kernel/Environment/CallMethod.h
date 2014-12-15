#ifndef MANTID_PYTHONINTERFACE_CALLMETHOD_H_
#define MANTID_PYTHONINTERFACE_CALLMETHOD_H_
/** 
    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>    
*/
#include "MantidPythonInterface/kernel/Environment/ErrorHandling.h"
#include "MantidPythonInterface/kernel/Environment/Threading.h"
#include "MantidPythonInterface/kernel/Environment/WrapperHelpers.h"

#include <boost/python/class.hpp>
#include <boost/python/call_method.hpp>

namespace Mantid { namespace PythonInterface {
  namespace Environment
  {

    /// Defines start of CallMethod function
    /// @param obj A Python object to perform the call
    /// @param name A string containing the method name
    #define PRE_CALL(obj,name)\
      GlobalInterpreterLock gil;\
      if(Environment::typeHasAttribute(self, funcName))\
      {\
        try\
        {\

    /// Defines the end of CallMethod that returns a default value
    #define POST_CALL_DEFAULT() \
        }\
        catch(boost::python::error_already_set&)\
        {\
          throwRuntimeError();\
        }\
      }\
      return defaultValue;

    /// Defines the end of CallMethod that throws an exception if the method doesn't exist
    #define POST_CALL_EXCEPT() \
        }\
        catch(boost::python::error_already_set&)\
        {\
          throwRuntimeError();\
        }\
      }\
      else\
      {\
        std::ostringstream os;\
        os << self->ob_type->tp_name << " has no function named '" << funcName << "'\n"\
           << "Check the function exists and that its first argument is self.";\
        throw std::runtime_error(os.str());\
      }\
    return ResultType(); // required to avoid compiler warning

    /// Defines the end of CallMethod that throws an exception but doesn't return anything
    /// to be used for void return types
    #define POST_CALL_EXCEPT_VOID() \
        }\
        catch(boost::python::error_already_set&)\
        {\
          throwRuntimeError();\
        }\
      }\
      else\
      {\
        std::ostringstream os;\
        os << self->ob_type->tp_name << " has no function named '" << funcName << "'\n"\
           << "Check the function exists and that its first argument is self.";\
        throw std::runtime_error(os.str());\
      }\


    /** @name No argument Python calls */
    //@{
    /**
     * Perform a call to a python function that takes no arguments and returns a value
     */
    template<typename ResultType>
    struct DLLExport CallMethod0
    {
      /**
       * Dispatch a call to the method on the given object. If the method does not exist
       * then return the defaultValue
       * @param self :: The object containing the method definition
       * @param funcName :: The method name
       * @param defaultValue :: A default value if the method does not exist
       * @return The value of the function or the default value if it does not exist
       */
      static ResultType dispatchWithDefaultReturn(PyObject *self, const char * funcName, const ResultType & defaultValue)
      {
        PRE_CALL(self, funcName);
        return boost::python::call_method<ResultType>(self,funcName);
        POST_CALL_DEFAULT();
      }

      /**
       * Dispatch a call to the method on the given object. If the method does not exist
       * then raise a std::runtime_error exception
       * @param self :: The object containing the method definition
       * @param funcName :: The method name
       * @return The value of the function or the default value if it does not exist
       */
      static ResultType dispatchWithException(PyObject *self, const char * funcName)
      {
        PRE_CALL(self,funcName);
        return boost::python::call_method<ResultType>(self, funcName);
        POST_CALL_EXCEPT();
      }
    };

    ///Specialization for void return type
    template<>
    struct DLLExport CallMethod0<void>
    {
      /**
       * Dispatch a call to the method on the given object. If the method does not exist
       * then raise a runtime_error
       * @param self :: The object containing the method definition
       * @param funcName :: The method name
       * @return The value of the function or the default value if it does not exist
       */
      static void dispatchWithException(PyObject *self, const char * funcName)
      {
        PRE_CALL(self, funcName);
        boost::python::call_method<void>(self,funcName);
        POST_CALL_EXCEPT_VOID();
      }
    };
    //@}

    /** @name Single argument Python calls */
    //@{
    /**
     * Perform a call to a python function that takes no arguments and returns a value
     */
    template<typename ResultType, typename Arg1>
    struct DLLExport CallMethod1
    {
      /**
       * Dispatch a call to the method on the given object. If the method does not exist
       * then return the defaultValue
       * @param self :: The object containing the method definition
       * @param funcName :: The method name
       * @param defaultValue :: A default value if the method does not exist
       * @param arg1 :: The value of the first argument
       * @return The value of the function or the default value if it does not exist
       */
      static ResultType dispatchWithDefaultReturn(PyObject *self, const char * funcName, const ResultType & defaultValue,
                                                  const Arg1 & arg1)
      {
        PRE_CALL(self, funcName);
        return boost::python::call_method<ResultType,Arg1>(self,funcName, arg1);
        POST_CALL_DEFAULT();
      }

      /**
       * Dispatch a call to the method on the given object. If the method does not exist
       * then raise a std::runtime_error exception
       * @param self :: The object containing the method definition
       * @param funcName :: The method name
       * @param arg1 :: The value of the first argument
       * @return The value of the function or the default value if it does not exist
       */
      static ResultType dispatchWithException(PyObject *self, const char * funcName, const Arg1 & arg1)
      {
        PRE_CALL(self,funcName);
        return boost::python::call_method<ResultType,Arg1>(self, funcName, arg1);
        POST_CALL_EXCEPT();
      }
    };

    ///Specialization for void return type
    template<typename Arg1>
    struct DLLExport CallMethod1<void,Arg1>
    {
      /**
       * Dispatch a call to the method on the given object. If the method does not exist
       * then raise a runtime_error
       * @param self :: The object containing the method definition
       * @param funcName :: The method name
       * @param arg1 :: The value of the first argument
       * @return The value of the function or the default value if it does not exist
       */
      static void dispatchWithException(PyObject *self, const char * funcName, const Arg1 & arg1)
      {
        PRE_CALL(self, funcName);
        boost::python::call_method<void,Arg1>(self,funcName,arg1);
        POST_CALL_EXCEPT_VOID();
      }
    };
    //@}


    /** @name Two argument Python calls */
    //@{
    template<typename ResultType,typename Arg1,typename Arg2>
    struct DLLExport CallMethod2
    {
      /**
       * Dispatch a call to the method on the given object. If the method does not exist
       * then return the defaultValue
       * @param self :: The object containing the method definition
       * @param funcName :: The method name
       * @param defaultValue :: A default value if the method does not exist
       * @param arg1 :: The value of the first argument
       * @param arg2 :: The value of the second argument
       * @return The value of the function or the default value if it does not exist
       */
      static ResultType dispatchWithDefaultReturn(PyObject *self, const char * funcName, const ResultType & defaultValue,
                                                  const Arg1 & arg1, const Arg2 & arg2)
      {
        PRE_CALL(self, funcName);
        return boost::python::call_method<ResultType,Arg1,Arg2>(self,funcName,arg1,arg2);
        POST_CALL_DEFAULT();
      }

      /**
       * Dispatch a call to the method on the given object. If the method does not exist
       * then raise a std::runtime_error exception
       * @param self :: The object containing the method definition
       * @param funcName :: The method name
       * @param arg1 :: The value of the first argument
       * @param arg2 :: The value of the second argument
       * @return The value of the function or the default value if it does not exist
       */
      static ResultType dispatchWithException(PyObject *self, const char * funcName,
                                              const Arg1 & arg1, const Arg2 & arg2)
      {
        PRE_CALL(self, funcName);
        return boost::python::call_method<ResultType,Arg1,Arg2>(self, funcName,arg1,arg2);
        POST_CALL_EXCEPT(); 
      }
    };

    ///Specialization for void return type
    template<typename Arg1,typename Arg2>
    struct DLLExport CallMethod2<void,Arg1,Arg2>
    {
      /**
       * Dispatch a call to the method on the given object. If the method does not exist
       * then raise a runtime_error
       * @param self :: The object containing the method definition
       * @param funcName :: The method name
       * @param arg1 :: The value of the first argument
       * @param arg2 :: The value of the second argument
       * @return The value of the function or the default value if it does not exist
       */
      static void dispatchWithException(PyObject *self, const char * funcName,
                                        const Arg1 & arg1, const Arg2 & arg2)
      {
        PRE_CALL(self, funcName);
        boost::python::call_method<void,Arg1,Arg2>(self,funcName,arg1,arg2);
        POST_CALL_EXCEPT_VOID();
      }
    };
    //@}
    
    // Tidy up
    #undef PRE_CALL
    #undef POST_CALL_DEFAULT
  }
}}

#endif //MANTID_PYTHONINTERFACE_CALLMETHOD_H_
