#ifndef MANTIDPYTHONAPI_API_EXPORTS_H_
#define MANTIDPYTHONAPI_API_EXPORTS_H_

/**
 * Define some small wrapper and proxy classes for various Mantid C++ classes
 * so that we can override their behaviour in Python
 */
#include <MantidPythonAPI/FrameworkManagerProxy.h>
#include <MantidPythonAPI/PythonInterfaceFunctions.h>
#include <MantidAPI/MatrixWorkspace.h>
#include <MantidAPI/AnalysisDataService.h>
#include <MantidAPI/IAlgorithm.h>
#include <MantidAPI/FileFinder.h>
#include <boost/python.hpp>
#include <iostream>

#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/TextAxis.h"

namespace Mantid
{
  
namespace PythonAPI
{
  //@cond
  /**
   * A wrapper for the PythonAPI::FrameworkProxy
   */
  class FrameworkProxyCallback : public FrameworkManagerProxy
  {
  public:
    /// Constructor
    FrameworkProxyCallback(PyObject* self) : FrameworkManagerProxy(), m_self(self) 
    {
      Py_INCREF(m_self);
    }

    /// Destructor
    ~FrameworkProxyCallback()
    {
      Py_DECREF(m_self);
    }

    
    void workspaceRemoved(const std::string & name)
    {
      PyCall_OneArg<void, std::string>::dispatch(m_self,"_workspaceRemoved" ,name);
    }

    static void default_workspaceRemoved(FrameworkManagerProxy& self, const std::string & name)
    {
      self.FrameworkManagerProxy::workspaceRemoved(name);
    }

    void workspaceReplaced(const std::string & name)
    {
      PyCall_OneArg<void,std::string>::dispatch(m_self,"_workspaceReplaced",name);
    }

    static void default_workspaceReplaced(FrameworkManagerProxy& self, const std::string & name)
    {
      self.FrameworkManagerProxy::workspaceReplaced(name);
    }

    void workspaceAdded(const std::string & name)
    {
      PyCall_OneArg<void, std::string>::dispatch(m_self,"_workspaceAdded",name);
    }

    static void default_workspaceAdded(FrameworkManagerProxy& self, const std::string & name)
    {
      self.FrameworkManagerProxy::workspaceAdded(name);
    }

    void workspaceStoreCleared()
    {
      PyCall_NoArg<void>::dispatch(m_self,"_workspaceStoreCleared");
    }

    static void default_workspaceStoreCleared(FrameworkManagerProxy& self)
    {
      self.FrameworkManagerProxy::workspaceStoreCleared();
    }

    void algorithmFactoryUpdated()
    {
      PyCall_NoArg<void>::dispatch(m_self,"_algorithmFactoryUpdated");
    }

    static void default_algorithmFactoryUpdated(FrameworkManagerProxy& self)
    {
      self.FrameworkManagerProxy::algorithmFactoryUpdated();
    }

  private:
    /// Store the PyObject that the callbacks refer to
    PyObject* m_self;
  };

  /**
   * FileFinder singleton wrapper
   *
   * This class is the type that is actually instantiated within Python.
   */
  struct FileFinderWrapper
  {
  public:
    /**
     * Call the corresponding FileFinder method
     * @param input The input for the function
     * @returns A string containing either the full path or nothing if the file could not be found
     */
    static std::string getFullPath(const std::string & input)
    {
      return API::FileFinder::Instance().getFullPath(input);
    }

    /**
     * Call the corresponding FileFinder method
     * @param input The input for the function
     * @returns A list of runs that have been found
     */
    static std::vector<std::string> findRuns(const std::string & input)
    {
      return API::FileFinder::Instance().findRuns(input);
    }

  };

  /**
  * Creates a NumericAxis
  * @param number of elements in the axis
  * @return pointer to the axis object
  */
  Mantid::API::Axis* createNumericAxis(int length)
  {
    return new Mantid::API::NumericAxis(length);
  }
  /**
  * Creates a SpectraAxis
  * @param number of elements in the axis
  * @return pointer to the axis object
  */
  Mantid::API::Axis* createSpectraAxis(int length)
  {
    return new Mantid::API::SpectraAxis(length);
  }
  /**
  * Creates a TextAxis
  * @param number of elements in the axis
  * @return pointer to the axis object
  */
  Mantid::API::Axis* createTextAxis(int length)
  {
    return new Mantid::API::TextAxis(length);
  }

  //@endcond


} // namespace PythonAPI
} // namespace Mantid

#endif //MANTIDPYTHONAPI_API_EXPORTS_H_
