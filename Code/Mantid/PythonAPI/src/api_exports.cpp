//
// Wrappers for classes in the Mantid::API namespace
//
#include <MantidPythonAPI/api_exports.h>
#include <MantidPythonAPI/stl_proxies.h>
#include <string>
#include <ostream>

// API
#include <MantidAPI/ITableWorkspace.h>
#include <MantidAPI/WorkspaceGroup.h>
#include <MantidAPI/Instrument.h>
#include <MantidAPI/ParInstrument.h>
#include <MantidAPI/Sample.h>
#include <MantidAPI/WorkspaceProperty.h>
#include <MantidPythonAPI/PyAlgorithmWrapper.h>

namespace Mantid
{
namespace PythonAPI
{
  using namespace Mantid::API;
  using namespace boost::python;

  //@cond
  //---------------------------------------------------------------------------
  // Class export functions
  //---------------------------------------------------------------------------

  void export_frameworkmanager()
  {
    /** 
     * Python Framework class (note that this is not the Mantid::API::FrameworkManager, there is another in 
     * PythonAPI::FrameworkManager)
     * This is the main class through which Python interacts with Mantid and with the exception of PyAlgorithm and V3D, 
     * is the only one directly instantiable in Python
     */
    class_<FrameworkManagerWrapper, boost::noncopyable>("FrameworkManager")
      .def("clear", &FrameworkManagerProxy::clear)
      .def("clearAlgorithms", &FrameworkManagerProxy::clearAlgorithms)
      .def("clearData", &FrameworkManagerProxy::clearData)
      .def("clearInstruments", &FrameworkManagerProxy::clearInstruments)
      .def("createAlgorithm", (createAlg_overload1)&FrameworkManagerProxy::createAlgorithm, 
	   return_value_policy< reference_existing_object >())
      .def("createAlgorithm", (createAlg_overload2)&FrameworkManagerProxy::createAlgorithm, 
	   return_value_policy< reference_existing_object >())
      .def("createAlgorithm", (createAlg_overload3)&FrameworkManagerProxy::createAlgorithm, 
	   return_value_policy< reference_existing_object >())
      .def("createAlgorithm", (createAlg_overload4)&FrameworkManagerProxy::createAlgorithm, 
	   return_value_policy< reference_existing_object >())
      .def("execute", (exec_ptr)&FrameworkManagerProxy::execute, 
	   return_value_policy< reference_existing_object >(), FrameworkManager_execute_overloads())
      .def("deleteWorkspace", &FrameworkManagerProxy::deleteWorkspace)
      .def("getWorkspaceNames", &FrameworkManagerProxy::getWorkspaceNames)
      .def("getWorkspaceGroupNames", &FrameworkManagerProxy::getWorkspaceGroupNames)
      .def("getWorkspaceGroupEntries", &FrameworkManagerProxy::getWorkspaceGroupEntries)
      .def("createPythonSimpleAPI", &FrameworkManagerProxy::createPythonSimpleAPI)
      .def("sendLogMessage", &FrameworkManagerProxy::sendLogMessage)
      .def("workspaceExists", &FrameworkManagerProxy::workspaceExists)
      .def("getConfigProperty", &FrameworkManagerProxy::getConfigProperty)
      .def("_getRawMatrixWorkspacePointer", &FrameworkManagerProxy::retrieveMatrixWorkspace)
      .def("_getRawTableWorkspacePointer", &FrameworkManagerProxy::retrieveTableWorkspace)
      .def("_getRawWorkspaceGroupPointer", &FrameworkManagerProxy::retrieveWorkspaceGroup)
      .def("_workspaceRemoved", &FrameworkManagerProxy::workspaceRemoved, 
	   &FrameworkManagerWrapper::default_workspaceRemoved)
      .def("_workspaceReplaced", &FrameworkManagerProxy::workspaceReplaced, 
	   &FrameworkManagerWrapper::default_workspaceReplaced)
      .def("_workspaceAdded", &FrameworkManagerProxy::workspaceAdded, 
	   &FrameworkManagerWrapper::default_workspaceAdded)
      .def("_workspaceStoreCleared", &FrameworkManagerProxy::workspaceStoreCleared, 
	   &FrameworkManagerWrapper::default_workspaceStoreCleared)
      .def("_setGILRequired", &FrameworkManagerProxy::setGILRequired)
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
    
    //PyAlgorithm
    //Save some typing for all of the templated declareProperty and getProperty methods
#define EXPORT_DECLAREPROPERTY(type, pyname)\
    .def(pyname,(void(PyAlgorithmWrapper::*)(const std::string &, type, const std::string &,const unsigned int))&Mantid::PythonAPI::PyAlgorithmWrapper::_declareProperty<type>)
    
#define EXPORT_GETPROPERTY(type, pyname)\
    .def(pyname,(type(PyAlgorithmWrapper::*)(const std::string &))&Mantid::PythonAPI::PyAlgorithmWrapper::_getProperty<type>)
    
    class_< Mantid::PythonAPI::PyAlgorithmWrapper, boost::noncopyable >("PyAlgorithm")
      .def("initialize", &Mantid::PythonAPI::PyAlgorithmWrapper::initialize)
      .def("execute", &Mantid::PythonAPI::PyAlgorithmWrapper::execute)
      .def("setPropertyValue", &Mantid::PythonAPI::PyAlgorithmWrapper::setPropertyValue)
      .def("getPropertyValue", &Mantid::PythonAPI::PyAlgorithmWrapper::getPropertyValue)
      .def("getProperties", &Mantid::PythonAPI::PyAlgorithmWrapper::getProperties, return_value_policy< copy_const_reference >())
      .def("_setMatrixWorkspaceProperty", &Mantid::PythonAPI::PyAlgorithmWrapper::_setMatrixWorkspaceProperty)
      .def("_setTableWorkspaceProperty", &Mantid::PythonAPI::PyAlgorithmWrapper::_setTableWorkspaceProperty)
      .def("_declareFileProperty", &Mantid::PythonAPI::PyAlgorithmWrapper::_declareFileProperty)
      .def("_declareMatrixWorkspace", &Mantid::PythonAPI::PyAlgorithmWrapper::_declareMatrixWorkspace)
      .def("_declareTableWorkspace", &Mantid::PythonAPI::PyAlgorithmWrapper::_declareTableWorkspace)
      EXPORT_DECLAREPROPERTY(int, "declareProperty_int")
      EXPORT_DECLAREPROPERTY(double, "declareProperty_dbl")
      EXPORT_DECLAREPROPERTY(std::string, "declareProperty_str")
      EXPORT_GETPROPERTY(int, "getProperty_int")
      EXPORT_GETPROPERTY(double, "getProperty_dbl")
      ;

    //Leave the place tidy 
#undef EXPORT_DECLAREPROPERTY
#undef EXPORT_GETPROPERTY

  }

  void export_workspace()
  {
    /// Shared pointer registration
    register_ptr_to_python<boost::shared_ptr<Workspace> >();
    
    class_<Mantid::API::Workspace, boost::noncopyable>("Workspace", no_init)
      .def("getTitle", &Mantid::API::Workspace::getTitle, 
	   return_value_policy< copy_const_reference >())
      .def("getComment", &Mantid::API::MatrixWorkspace::getComment, 
	   return_value_policy< copy_const_reference >() )
      .def("getMemorySize", &Mantid::API::Workspace::getMemorySize)
      .def("getName", &Mantid::API::Workspace::getName, return_value_policy< copy_const_reference >())
      .def("__str__", &Mantid::API::Workspace::getName, return_value_policy< copy_const_reference >())
      ;
  }

  void export_matrixworkspace()
  {
    /// Shared pointer registration
    register_ptr_to_python<boost::shared_ptr<MatrixWorkspace> >();

    // A vector of MatrixWorkspace pointers
    vector_proxy<MatrixWorkspace*>::wrap("stl_vector_matrixworkspace");
 
    //Operator overloads dispatch through the above structure. The typedefs save some typing
    typedef MatrixWorkspaceProxy::wraptype_ptr(*binary_fn1)(const MatrixWorkspaceProxy::wraptype_ptr, const MatrixWorkspaceProxy::wraptype_ptr);
    typedef MatrixWorkspaceProxy::wraptype_ptr(*binary_fn2)(const MatrixWorkspaceProxy::wraptype_ptr, double);

    //MatrixWorkspace class
    class_< Mantid::API::MatrixWorkspace, bases<Mantid::API::Workspace>, boost::noncopyable >("MatrixWorkspace", no_init)
      .def("getNumberHistograms", &Mantid::API::MatrixWorkspace::getNumberHistograms)
      .def("readX", &Mantid::API::MatrixWorkspace::readX, return_value_policy< return_by_value >()) 
      .def("readY", &Mantid::API::MatrixWorkspace::readY, return_value_policy< return_by_value >()) 
      .def("readE", &Mantid::API::MatrixWorkspace::readE, return_value_policy< return_by_value >()) 
      .def("blocksize", &Mantid::API::MatrixWorkspace::blocksize)
      .def("isDistribution", (const bool& (Mantid::API::MatrixWorkspace::*)() const)&Mantid::API::MatrixWorkspace::isDistribution, 
	   return_value_policy< copy_const_reference >(), MatrixWorkspace_isDistribution_overloads_1())
      .def("isDistribution", (bool& (Mantid::API::MatrixWorkspace::*)(bool)const)&Mantid::API::MatrixWorkspace::isDistribution, 
	   return_value_policy< copy_non_const_reference >(), MatrixWorkspace_isDistribution_overloads_2())
      .def("getInstrument", &Mantid::API::MatrixWorkspace::getInstrument)
      .def("getDetector", &Mantid::API::MatrixWorkspace::getDetector)
      .def("getSampleDetails", &Mantid::API::MatrixWorkspace::sample, return_value_policy< copy_const_reference >() )
      .def("__add__", (binary_fn1)&MatrixWorkspaceProxy::plus)
      .def("__add__", (binary_fn2)&MatrixWorkspaceProxy::plus)
      .def("__radd__",(binary_fn2)&MatrixWorkspaceProxy::rplus)
      .def("__iadd__",(binary_fn1)&MatrixWorkspaceProxy::inplace_plus)
      .def("__iadd__",(binary_fn2)&MatrixWorkspaceProxy::inplace_plus)
      .def("__sub__", (binary_fn1)&MatrixWorkspaceProxy::minus)
      .def("__sub__", (binary_fn2)&MatrixWorkspaceProxy::minus)
      .def("__rsub__",(binary_fn2)&MatrixWorkspaceProxy::rminus)
      .def("__isub__",(binary_fn1)&MatrixWorkspaceProxy::inplace_minus)
      .def("__isub__",(binary_fn2)&MatrixWorkspaceProxy::inplace_minus)
      .def("__mul__", (binary_fn1)&MatrixWorkspaceProxy::times)
      .def("__mul__", (binary_fn2)&MatrixWorkspaceProxy::times)
      .def("__rmul__",(binary_fn2)&MatrixWorkspaceProxy::rtimes)
      .def("__imul__",(binary_fn1)&MatrixWorkspaceProxy::inplace_times)
      .def("__imul__",(binary_fn2)&MatrixWorkspaceProxy::inplace_times)
      .def("__div__", (binary_fn1)&MatrixWorkspaceProxy::divide)
      .def("__div__", (binary_fn2)&MatrixWorkspaceProxy::divide)
      .def("__rdiv__", (binary_fn2)&MatrixWorkspaceProxy::rdivide)
      .def("__idiv__",(binary_fn1)&MatrixWorkspaceProxy::inplace_divide)
      .def("__idiv__",(binary_fn2)&MatrixWorkspaceProxy::inplace_divide)
      ;
  }


  void export_tableworkspace()
  {
    // Declare the pointer
    register_ptr_to_python<Mantid::API::ITableWorkspace_sptr>();
    
    // Table workspace
    // Some function pointers since MSVC can't figure out the function to call when 
    // placing this directly in the .def functions below
    typedef int&(ITableWorkspace::*get_integer_ptr)(const std::string &, int);
    typedef double&(ITableWorkspace::*get_double_ptr)(const std::string &, int);
    typedef std::string&(ITableWorkspace::*get_string_ptr)(const std::string &, int);

    // TableWorkspace class
    class_< ITableWorkspace, bases<Mantid::API::Workspace>, boost::noncopyable >("ITableWorkspace", no_init)
      .def("getColumnCount", &ITableWorkspace::columnCount)
      .def("getRowCount", &ITableWorkspace::rowCount)
      .def("getColumnNames",&ITableWorkspace::getColumnNames)
      .def("getInt", (get_integer_ptr)&ITableWorkspace::getRef<int>, return_value_policy<copy_non_const_reference>())
      .def("getDouble", (get_double_ptr)&ITableWorkspace::getRef<double>, return_value_policy<copy_non_const_reference>())
      .def("getString", (get_string_ptr)&ITableWorkspace::getRef<std::string>, return_value_policy<copy_non_const_reference>())
     ;
  }

  // WorkspaceGroup
  void export_workspacegroup()
  {
    // Pointer
    register_ptr_to_python<Mantid::API::WorkspaceGroup_sptr>();
    
    class_< Mantid::API::WorkspaceGroup, bases<Mantid::API::Workspace>, 
      boost::noncopyable >("WorkspaceGroup", no_init)
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
    class_< Mantid::API::Sample >("Sample", no_init)
      .def("getLogData", (Mantid::Kernel::Property* (Mantid::API::Sample::*)(const std::string&) const)0, 
	   return_value_policy< reference_existing_object>(), Sample_getLogData_overloads())
      .def("getName", &Mantid::API::Sample::getName, return_value_policy<copy_const_reference>())
      .def("getProtonCharge", &Mantid::API::Sample::getProtonCharge, return_value_policy< copy_const_reference>())
      .def("getGeometryFlag", &Mantid::API::Sample::getGeometryFlag)
      .def("getThickness", &Mantid::API::Sample::getThickness)
      .def("getHeight", &Mantid::API::Sample::getHeight)
      .def("getWidth", &Mantid::API::Sample::getWidth)
     ;
  }

  void export_instrument()
  {
    //Pointer to the interface
    register_ptr_to_python<boost::shared_ptr<Mantid::API::IInstrument> >();
    
    //IInstrument class
    class_< Mantid::API::IInstrument, boost::python::bases<Mantid::Geometry::ICompAssembly>, 
      boost::noncopyable>("IInstrument", no_init)
      .def("getSample", &Mantid::API::IInstrument::getSample)
      .def("getSource", &Mantid::API::IInstrument::getSource)
      .def("getComponentByName", &Mantid::API::IInstrument::getComponentByName)
      ;

    /** Concrete implementations so that Python knows about them */
    
    //Instrument class
    class_< Mantid::API::Instrument, boost::python::bases<Mantid::API::IInstrument>, 
	    boost::noncopyable>("Instrument", no_init)
      ;
    //Instrument class
    class_< Mantid::API::ParInstrument, boost::python::bases<Mantid::API::IInstrument>, 
	    boost::noncopyable>("ParInstrument", no_init)
      ;
  }

  void export_workspace_property()
  {
    // Tell python about this so I can check if a property is a workspace
    class_< WorkspaceProperty<Workspace>, bases<Mantid::Kernel::Property>, boost::noncopyable>("WorkspaceProperty", no_init)
      ;
    // Tell python about this so I can check if a property is a workspace
    class_< WorkspaceProperty<MatrixWorkspace>, bases<Mantid::Kernel::Property>, boost::noncopyable>("MatrixWorkspaceProperty", no_init)
      ;
    // Tell python about this so I can check if a property is a workspace
    class_< WorkspaceProperty<ITableWorkspace>, bases<Mantid::Kernel::Property>, boost::noncopyable>("TableWorkspaceProperty", no_init)
      ;

  }

 void export_workspacefactory()
  {
    class_< PythonAPI::WorkspaceFactoryProxy, boost::noncopyable>("WorkspaceFactory", no_init)
      .def("createMatrixWorkspace", &PythonAPI::WorkspaceFactoryProxy::createMatrixWorkspace)
      .staticmethod("createMatrixWorkspace")
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
    export_workspace_property();
    export_workspacefactory();
  }
  //@endcond
}
}
