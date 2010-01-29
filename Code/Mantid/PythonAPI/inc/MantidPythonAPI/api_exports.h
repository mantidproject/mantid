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
#include <boost/python.hpp>
#include <iostream>

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
   *Some helpful function pointers typedefs
   */
  typedef Mantid::API::IAlgorithm*(FrameworkManagerProxy::*createAlg_overload1)(const std::string&);
  typedef Mantid::API::IAlgorithm*(FrameworkManagerProxy::*createAlg_overload2)(const std::string&, const int&);
  typedef Mantid::API::IAlgorithm*(FrameworkManagerProxy::*createAlg_overload3)(const std::string&, const std::string&);
  typedef Mantid::API::IAlgorithm*(FrameworkManagerProxy::*createAlg_overload4)(const std::string&, const std::string&, const int&);

  //@endcond
}
}

#endif //MANTIDPYTHONAPI_API_EXPORTS_H_
