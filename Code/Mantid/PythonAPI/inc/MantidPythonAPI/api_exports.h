#ifndef MANTIDPYTHONAPI_API_EXPORTS_H_
#define MANTIDPYTHONAPI_API_EXPORTS_H_

/**
 * Define some small wrapper and proxy classes for various Mantid C++ classes
 * so that we can override their behaviour in Python
 */
#include <MantidPythonAPI/FrameworkManagerProxy.h>
#include <MantidPythonAPI/PythonInterfaceFunctions.h>
#include <MantidAPI/MatrixWorkspace.h>
#include <MantidAPI/WorkspaceFactory.h>
#include <MantidAPI/WorkspaceOpOverloads.h>
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

  /**
   *  A proxy struct for implementing workspace algebra operator overloads
   */
  struct MatrixWorkspaceProxy
  {
    typedef API::MatrixWorkspace wraptype;
    typedef boost::shared_ptr<wraptype> wraptype_ptr;
    
    //Binary operation for 2 workspaces
    static wraptype_ptr performBinaryOp(const wraptype_ptr lhs, const wraptype_ptr rhs, char op, bool inplace)
    {
      wraptype_ptr result;
      std::string name(lhs->getName());
      if( inplace )
      {
        switch( op )
        {
        case 'p':
          lhs += rhs;
          break;
        case 'm':
          lhs -= rhs;
          break;
        case 't':
          lhs *= rhs;
          break;
        case 'd':
          lhs /= rhs;
        }
        result = lhs;
      }
      else
      {
        switch( op )
        {
        case 'p':
          result = lhs + rhs;
          break;
        case 'm':
          result = lhs - rhs;
          break;
        case 't':
          result = lhs * rhs;
          break;
        case 'd':
          result = lhs / rhs;
        }
        name += std::string("_") + op + std::string("_") + rhs->getName();
      }
      Mantid::API::AnalysisDataService::Instance().addOrReplace(name, result);
      return result;
    }
    //Plus workspace
    static wraptype_ptr plus(const wraptype_ptr lhs, const wraptype_ptr rhs)
    {
      return performBinaryOp(lhs, rhs, 'p', false);
    }
    /// Inplace Plus workspace
    static wraptype_ptr inplace_plus(const wraptype_ptr lhs, const wraptype_ptr rhs)
    {
      return performBinaryOp(lhs, rhs, 'p', true);
    }
    /// Minus workspace
    static wraptype_ptr minus(const wraptype_ptr lhs, const wraptype_ptr rhs)
    {
      return performBinaryOp(lhs, rhs, 'm', false);
    }
    /// Inplace Minus workspace
    static wraptype_ptr inplace_minus(const wraptype_ptr lhs, const wraptype_ptr rhs)
    {
      return performBinaryOp(lhs, rhs, 'm', true);
    }
    /// Multiply workspace
    static wraptype_ptr times(const wraptype_ptr lhs, const wraptype_ptr rhs)
    {
      return performBinaryOp(lhs, rhs, 't', false);
    }
    /// Inplace Multiply workspace
    static wraptype_ptr inplace_times(const wraptype_ptr lhs, const wraptype_ptr rhs)
    {
      return performBinaryOp(lhs, rhs, 't', true);
    }
    /// Divide workspace
    static wraptype_ptr divide(const wraptype_ptr lhs, const wraptype_ptr rhs)
    {
      return performBinaryOp(lhs, rhs, 'd', false);
    }
    /// Divide workspace
    static wraptype_ptr inplace_divide(const wraptype_ptr lhs, const wraptype_ptr rhs)
    {
      return performBinaryOp(lhs, rhs, 'd', true);
    }

    /// Binary operation for a workspace and a double
    static wraptype_ptr performBinaryOp(const wraptype_ptr lhs, double rhs, char op, bool inplace)
    {
      wraptype_ptr result;
      std::string name(lhs->getName());
      if( inplace )
      {
        switch( op )
        {
        case 'p':
          lhs += rhs;
          break;
        case 'm':
          lhs -= rhs;
          break;
        case 't':
          lhs *= rhs;
          break;
        case 'd':
          lhs /= rhs;
          break;
        }
        result = lhs;
      }
      else
      {
        switch( op )
        {
        case 'p':
          result = lhs + rhs;
          break;
        case 'm':
          result = lhs - rhs;
          break;
        case 't':
          result = lhs * rhs;
          break;
        case 'd':
          result = lhs / rhs;
        }
        std::ostringstream os;
        os << rhs;
        name += std::string("_") + op + std::string("_") + os.str();
      }
      Mantid::API::AnalysisDataService::Instance().addOrReplace(name, result);
      return result;
    }
    /// Binary operation for a double and a workspace
    static wraptype_ptr performBinaryOp(double lhs, const wraptype_ptr rhs, char op)
    {
      wraptype_ptr result;
      std::ostringstream os;
      os << lhs;
      std::string name(os.str());
      switch( op )
      {
        case 'p':
          result = rhs + lhs;
          break;
        case 'm':
          result = lhs - rhs;
          break;
        case 't':
          result = lhs * rhs;
          break;
        case 'd':
          result = lhs / rhs;
      }
      name += std::string("_") + op + std::string("_") + rhs->getName();
      Mantid::API::AnalysisDataService::Instance().addOrReplace(name, result);
      return result;
    }
    /// Plus
    static wraptype_ptr plus(const wraptype_ptr lhs, double rhs)
    {
      return performBinaryOp(lhs, rhs, 'p', false);
    }
    /// Inplace Plus
    static wraptype_ptr inplace_plus(const wraptype_ptr lhs, double rhs)
    {
      return performBinaryOp(lhs, rhs, 'p', true);
    }
    /// Reverse Plus
    static wraptype_ptr rplus(const wraptype_ptr rhs, double lhs)
    {
      return performBinaryOp(lhs, rhs, 'p');
    }
    /// Minus
    static wraptype_ptr minus(const wraptype_ptr lhs, double rhs)
    {
      return performBinaryOp(lhs, rhs, 'm', false);
    }
    /// Inplace Minus
    static wraptype_ptr inplace_minus(const wraptype_ptr lhs, double rhs)
    {
      return performBinaryOp(lhs, rhs, 'm', true);
    }
    /// Reverse Minus
    static wraptype_ptr rminus(const wraptype_ptr rhs, double lhs)
    {
      return performBinaryOp(lhs, rhs, 'm');
    }
    /// Multiply
    static wraptype_ptr times(const wraptype_ptr lhs, double rhs)
    {
      return performBinaryOp(lhs, rhs, 't', false);
    }
    /// Inplace Multiply
    static wraptype_ptr inplace_times(const wraptype_ptr lhs, double rhs)
    {
      return performBinaryOp(lhs, rhs, 't', true);
    }
    /// Multiply
    static wraptype_ptr rtimes(const wraptype_ptr rhs, double lhs)
    {
      return performBinaryOp(lhs, rhs, 't');
    }
    /// Divide
    static wraptype_ptr divide(const wraptype_ptr lhs, double rhs)
    {
      return performBinaryOp(lhs, rhs, 'd', false);
    }
    /// Reverse Divide
    static wraptype_ptr rdivide(const wraptype_ptr rhs, double lhs)
    {
      return performBinaryOp(lhs, rhs, 'd');
    }
    /// Inplace Divide
    static wraptype_ptr inplace_divide(const wraptype_ptr lhs, double rhs)
    {
      return performBinaryOp(lhs, rhs, 'd', true);
    }

  };

  // Overload get and set members for isDistribution
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(MatrixWorkspace_isDistribution_overloads_1, isDistribution, 0, 0)
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(MatrixWorkspace_isDistribution_overloads_2, isDistribution, 1, 1)
  //@endcond

  /**
   * A proxy struct for the WorkspaceFactory
   */
  struct WorkspaceFactoryProxy
  {
    /**
     * Create a MatrixWorkspace object that is initialized to the required size
     */
    static API::MatrixWorkspace_sptr createMatrixWorkspace(int nvectors, int xlength, int ylength)
    {
      return API::WorkspaceFactory::Instance().create("Workspace2D", nvectors, xlength, ylength);
    }

    /**
     * Create a table workspace object
     */
    static API::ITableWorkspace_sptr createTableWorkspace()
    {
      return API::WorkspaceFactory::Instance().createTable();
    }

  };

}
}

#endif //MANTIDPYTHONAPI_API_EXPORTS_H_
