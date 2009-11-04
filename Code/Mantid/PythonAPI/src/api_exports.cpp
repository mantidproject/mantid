//
// Wrappers for classes in the Mantid::API namespace
//

#include <boost/python.hpp>
#include <string>

// API
#include <MantidAPI/MatrixWorkspace.h>
#include <MantidAPI/ITableWorkspace.h>
#include <MantidAPI/WorkspaceGroup.h>
#include <MantidAPI/WorkspaceOpOverloads.h>
#include <MantidAPI/AnalysisDataService.h>
#include <MantidAPI/IAlgorithm.h>
#include <MantidAPI/Instrument.h>
#include <MantidAPI/ParInstrument.h>
#include <MantidAPI/Sample.h>

#include <MantidPythonAPI/FrameworkManager.h>
#include <MantidPythonAPI/stl_proxies.h>

namespace Mantid
{
namespace PythonAPI
{
  using namespace Mantid::API;
  using namespace boost::python;
  //@cond
  //@{

  //---------------------------------------------------------------------------
  // Class export functions
  //---------------------------------------------------------------------------
  Mantid::API::IAlgorithm* (Mantid::PythonAPI::FrameworkManager::*createAlg_ptr1)(const std::string&) = &Mantid::PythonAPI::FrameworkManager::createAlgorithm;
  Mantid::API::IAlgorithm* (Mantid::PythonAPI::FrameworkManager::*createAlg_ptr2)(const std::string&, const int &) = 
    &Mantid::PythonAPI::FrameworkManager::createAlgorithm;
  Mantid::API::IAlgorithm* (Mantid::PythonAPI::FrameworkManager::*createAlg_ptr3)(const std::string&, const std::string&) = 
    &Mantid::PythonAPI::FrameworkManager::createAlgorithm;
  Mantid::API::IAlgorithm* (Mantid::PythonAPI::FrameworkManager::*createAlg_ptr4)(const std::string&, const std::string&, const int &) = 
    &Mantid::PythonAPI::FrameworkManager::createAlgorithm;

  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(FrameworkManager_execute_overloads, execute, 2, 3)
  Mantid::API::IAlgorithm* (Mantid::PythonAPI::FrameworkManager::*exec_ptr)(const std::string&, const std::string&, const int&) =
    &Mantid::PythonAPI::FrameworkManager::execute;

  void export_frameworkmanager()
  {
    /** 
     * Python Framework class (note that this is not the Mantid::API::FrameworkManager, there is another in PythonAPI::FrameworkManager)
     * This is the main class through which Python interacts with Mantid and with the exception of PyAlgorithm and V3D, is the only
     * one directly instantiable in Python
     */
    

    class_<Mantid::PythonAPI::FrameworkManager, boost::noncopyable>("FrameworkManager", init<>())
      .def("clear", &Mantid::PythonAPI::FrameworkManager::clear)
      .def("clearAlgorithms", &Mantid::PythonAPI::FrameworkManager::clearAlgorithms)
      .def("clearData", &Mantid::PythonAPI::FrameworkManager::clearData)
      .def("clearInstruments", &Mantid::PythonAPI::FrameworkManager::clearInstruments)
      .def("createAlgorithm", createAlg_ptr1, return_value_policy< reference_existing_object >())
      .def("createAlgorithm", createAlg_ptr2, return_value_policy< reference_existing_object >())
      .def("createAlgorithm", createAlg_ptr3, return_value_policy< reference_existing_object >())
      .def("createAlgorithm", createAlg_ptr4, return_value_policy< reference_existing_object >())
      .def("execute", exec_ptr, return_value_policy< reference_existing_object >(), FrameworkManager_execute_overloads())
      .def("getMatrixWorkspace", &Mantid::PythonAPI::FrameworkManager::getMatrixWorkspace, 
	   return_value_policy< reference_existing_object >())
      .def("getTableWorkspace", &Mantid::PythonAPI::FrameworkManager::getTableWorkspace, 
	   return_value_policy< reference_existing_object >())
      .def("getMatrixWorkspaceGroup", &Mantid::PythonAPI::FrameworkManager::getMatrixWorkspaceGroup)
      .def("deleteWorkspace", &Mantid::PythonAPI::FrameworkManager::deleteWorkspace)
      .def("getWorkspaceNames", &Mantid::PythonAPI::FrameworkManager::getWorkspaceNames)
      .def("getWorkspaceGroupNames", &Mantid::PythonAPI::FrameworkManager::getWorkspaceGroupNames)
      .def("getWorkspaceGroupEntries", &Mantid::PythonAPI::FrameworkManager::getWorkspaceGroupEntries)
      .def("createPythonSimpleAPI", &Mantid::PythonAPI::FrameworkManager::createPythonSimpleAPI)
      .def("sendLogMessage", &Mantid::PythonAPI::FrameworkManager::sendLogMessage)
      .def("workspaceExists", &Mantid::PythonAPI::FrameworkManager::workspaceExists)
      .def("addPythonAlgorithm", &Mantid::PythonAPI::FrameworkManager::addPythonAlgorithm)
      .def("executePythonAlgorithm", &Mantid::PythonAPI::FrameworkManager::executePythonAlgorithm)
     ;
  }

  void export_ialgorithm()
  {
    register_ptr_to_python<Mantid::API::IAlgorithm*>();

    class_< Mantid::API::IAlgorithm, boost::noncopyable>("IAlgorithm", no_init)
      .def("initialize", &Mantid::API::IAlgorithm::initialize)
      .def("execute", &Mantid::API::IAlgorithm::execute)
      .def("isInitialized", &Mantid::API::IAlgorithm::isInitialized)
      .def("isExecuted", &Mantid::API::IAlgorithm::isExecuted)
      .def("setPropertyValue", &Mantid::API::IAlgorithm::setPropertyValue)
      .def("getPropertyValue", &Mantid::API::IAlgorithm::getPropertyValue)
      .def("getProperties", &Mantid::API::IAlgorithm::getProperties, return_value_policy< copy_const_reference >())
      ;
  }

  void export_workspace()
  {
    class_<Mantid::API::Workspace, boost::noncopyable>("Workspace", no_init)
      .def("getTitle", &Mantid::API::Workspace::getTitle, 
	   return_value_policy< copy_const_reference >())
      .def("getComment", &Mantid::API::MatrixWorkspace::getComment, 
	   return_value_policy< copy_const_reference >() )
      .def("getMemorySize", &Mantid::API::Workspace::getMemorySize)
      ;
  }

  /**
   *  A proxy struct for implementing workspace algebra operator overloads
   */
  struct MatrixWorkspaceProxy
  {
    typedef API::MatrixWorkspace wraptype;
    typedef boost::shared_ptr<wraptype> wraptype_ptr;
    
    /// Binary operation for 2 workspaces
    static wraptype_ptr performBinaryOp(const wraptype_ptr lhs, const wraptype_ptr rhs, char op)
    {
      wraptype_ptr result;
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
      std::string name = lhs->getName() +  std::string("_") + op
	+ std::string("_") +  rhs->getName();
      Mantid::API::AnalysisDataService::Instance().addOrReplace(name, result);
      return result;
    }
    /// Plus workspace
    static wraptype_ptr plus(const wraptype_ptr lhs, const wraptype_ptr rhs)
    {
      return performBinaryOp(lhs, rhs, 'p');
    }
    /// Minus workspace
    static wraptype_ptr minus(const wraptype_ptr lhs, const wraptype_ptr rhs)
    {
      return performBinaryOp(lhs, rhs, 'm');
    }
    /// Multiply workspace
    static wraptype_ptr times(const wraptype_ptr lhs, const wraptype_ptr rhs)
    {
      return performBinaryOp(lhs, rhs, 't');
    }
    /// Divide workspace
    static wraptype_ptr divide(const wraptype_ptr lhs, const wraptype_ptr rhs)
    {
      return performBinaryOp(lhs, rhs, 'd');
    }
    /// Binary operation for a workspace and a double
    static wraptype_ptr performBinaryOp(const wraptype_ptr lhs, double rhs, char op)
    {
      wraptype_ptr result;
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
      std::string name = lhs->getName() +  std::string("_") + op
	+ std::string("_");
      std::ostringstream os;
      os << rhs;
      Mantid::API::AnalysisDataService::Instance().addOrReplace(name + os.str(), result);
      return result;
    }
    /// Plus
    static wraptype_ptr plus(const wraptype_ptr lhs, double rhs)
    {
      return performBinaryOp(lhs, rhs, 'p');
    }
    /// Minus
    static wraptype_ptr minus(const wraptype_ptr lhs, double rhs)
    {
      return performBinaryOp(lhs, rhs, 'm');
    }
    /// Multiply
    static wraptype_ptr times(const wraptype_ptr lhs, double rhs)
    {
      return performBinaryOp(lhs, rhs, 't');
    }
    /// Divide
    static wraptype_ptr divide(const wraptype_ptr lhs, double rhs)
    {
      return performBinaryOp(lhs, rhs, 'd');
    }
  };

  //Overload get and set members
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(MatrixWorkspace_isDistribution_overloads_1, isDistribution, 0, 0)
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(MatrixWorkspace_isDistribution_overloads_2, isDistribution, 1, 1)
  
  void export_matrixworkspace()
  {
    /// Pointer registration
    register_ptr_to_python<MatrixWorkspace*>();
    /// Shared pointer registration
    register_ptr_to_python<boost::shared_ptr<MatrixWorkspace> >();

    // A vector of MatrixWorkspace pointers
    Mantid::PythonAPI::vector_proxy<MatrixWorkspace*>::wrap("stl_vector_matrixworkspace");
 
    //Operator overloads dispatch through the above structure. The typedefs save some typing
    typedef Mantid::API::MatrixWorkspace_sptr(*binary_fn1)(const Mantid::API::MatrixWorkspace_sptr, const Mantid::API::MatrixWorkspace_sptr);
    typedef Mantid::API::MatrixWorkspace_sptr(*binary_fn2)(const Mantid::API::MatrixWorkspace_sptr, double);

    //MatrixWorkspace class
    class_< Mantid::API::MatrixWorkspace, bases<Mantid::API::Workspace>, boost::noncopyable >("MatrixWorkspace", no_init)
      .def("getNumberHistograms", &Mantid::API::MatrixWorkspace::getNumberHistograms)
      .def("readX", &Mantid::API::MatrixWorkspace::readX, return_value_policy< return_by_value >()) 
      .def("readY", &Mantid::API::MatrixWorkspace::readY, return_value_policy< return_by_value >()) 
      .def("readE", &Mantid::API::MatrixWorkspace::readE, return_value_policy< return_by_value >()) 
      .def("blocksize", &Mantid::API::MatrixWorkspace::blocksize)
      .def("isDistribution", (const bool& (Mantid::API::MatrixWorkspace::*)() const)&Mantid::API::MatrixWorkspace::isDistribution, 
	   return_value_policy< copy_const_reference >(), MatrixWorkspace_isDistribution_overloads_1())
      .def("isDistribution", (bool& (Mantid::API::MatrixWorkspace::*)(bool))&Mantid::API::MatrixWorkspace::isDistribution, 
	   return_value_policy< copy_non_const_reference >(), MatrixWorkspace_isDistribution_overloads_2())
      .def("getInstrument", &Mantid::API::MatrixWorkspace::getInstrument)
      .def("getDetector", &Mantid::API::MatrixWorkspace::getDetector)
      .def("getSampleDetails", &Mantid::API::MatrixWorkspace::getSample)
      .def("__add__", (binary_fn1)&MatrixWorkspaceProxy::plus)
      .def("__add__", (binary_fn2)&MatrixWorkspaceProxy::plus)
      .def("__radd__",(binary_fn2)&MatrixWorkspaceProxy::plus)
      .def("__sub__", (binary_fn1)&MatrixWorkspaceProxy::minus)
      .def("__sub__", (binary_fn2)&MatrixWorkspaceProxy::minus)
      .def("__rsub__",(binary_fn2)&MatrixWorkspaceProxy::minus)
      .def("__mul__", (binary_fn1)&MatrixWorkspaceProxy::times)
      .def("__mul__", (binary_fn2)&MatrixWorkspaceProxy::times)
      .def("__rmul__",(binary_fn2)&MatrixWorkspaceProxy::times)
      .def("__div__", (binary_fn1)&MatrixWorkspaceProxy::divide)
      .def("__div__", (binary_fn2)&MatrixWorkspaceProxy::divide)
      .def("__rdiv__",(binary_fn2)&MatrixWorkspaceProxy::divide)
      ;
  }

  // Table workspace
  // Some function pointers since MSVC can't figure out the function to call when placing this directly in the .def functions below
  /// A function pointer to retrieve a integer from a name column and index
  int& (Mantid::API::ITableWorkspace::*ITableWorkspace_GetInteger)(const std::string &, int) = 
    &Mantid::API::ITableWorkspace::getRef<int>;
  /// A function pointer to retrieve a double from a name column and index
  double& (Mantid::API::ITableWorkspace::*ITableWorkspace_GetDouble)(const std::string &, int) = 
    &Mantid::API::ITableWorkspace::getRef<double>;
  /// A function pointer to retrieve a string from a name column and index
  std::string& (Mantid::API::ITableWorkspace::*ITableWorkspace_GetString)(const std::string &, int) = 
    &Mantid::API::ITableWorkspace::getRef<std::string>;
  void export_tableworkspace()
  {
    
    // Declare the pointer
    register_ptr_to_python<Mantid::API::ITableWorkspace*>();

    // TableWorkspace class
    class_< Mantid::API::ITableWorkspace, bases<Mantid::API::Workspace>, boost::noncopyable >("ITableWorkspace", no_init)
      .def("getColumnCount", &Mantid::API::ITableWorkspace::columnCount)
      .def("getRowCount", &Mantid::API::ITableWorkspace::rowCount)
      .def("getColumnNames",&Mantid::API::ITableWorkspace::getColumnNames)
      .def("getInt", ITableWorkspace_GetInteger, return_value_policy< copy_non_const_reference >())
      .def("getDouble", ITableWorkspace_GetDouble, return_value_policy< copy_non_const_reference >())
      .def("getString", ITableWorkspace_GetString, return_value_policy< copy_non_const_reference >())
     ;
  }

  // WorkspaceGroup
  void export_workspacegroup()
  {
    // Pointer
    register_ptr_to_python<Mantid::API::WorkspaceGroup*>();
    
    class_< Mantid::API::WorkspaceGroup, boost::noncopyable >("WorkspaceGroup", no_init)
      .def("size", &Mantid::API::WorkspaceGroup::getNumberOfEntries)
      .def("getNames", &Mantid::API::WorkspaceGroup::getNames, return_value_policy< copy_const_reference >())
      .def("add", &Mantid::API::WorkspaceGroup::add)
      .def("remove", &Mantid::API::WorkspaceGroup::remove)
      ;
  }
  
  // Sample class
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Sample_getLogData_overloads, getLogData, 0, 1)
  
  void export_sample()
  {
    //Pointer
    register_ptr_to_python<boost::shared_ptr<Mantid::API::Sample> >();

    //Sample class
    class_< Mantid::API::Sample, boost::noncopyable >("Sample", no_init)
      .def("getLogData", (Mantid::Kernel::Property* (Mantid::API::Sample::*)(const std::string&) const)0, 
	   return_value_policy< reference_existing_object>(), Sample_getLogData_overloads())
     .def("getName", &Mantid::API::Sample::getName, return_value_policy<copy_const_reference>())
     .def("getProtonCharge", &Mantid::API::Sample::getProtonCharge, return_value_policy< copy_const_reference>())
     ;
  }

  void export_instrument()
  {
    //Pointer to the interface
    register_ptr_to_python<boost::shared_ptr<Mantid::API::IInstrument> >();
    
    //IInstrument class
    class_< Mantid::API::IInstrument, boost::python::bases<Mantid::Geometry::ICompAssembly>, boost::noncopyable>("IInstrument", no_init)
      .def("getSample", &Mantid::API::IInstrument::getSample)
      .def("getSource", &Mantid::API::IInstrument::getSource)
      .def("getComponentByName", &Mantid::API::IInstrument::getComponentByName)
      ;

    /** Concrete implementations so that Python knows about them */
    
    //Instrument class
    class_< Mantid::API::Instrument, boost::python::bases<Mantid::API::IInstrument>, boost::noncopyable>("Instrument", no_init)
      ;
    //Instrument class
    class_< Mantid::API::ParInstrument, boost::python::bases<Mantid::API::IInstrument>, boost::noncopyable>("ParInstrument", no_init)
     ;

  }

  void export_api_namespace()
  {
    export_frameworkmanager();
    export_ialgorithm();
    export_workspace();
    export_matrixworkspace();
    export_tableworkspace();
    export_workspacegroup();
    export_sample();
    export_instrument();
  }
  //@}
  //@endcond
}
}
