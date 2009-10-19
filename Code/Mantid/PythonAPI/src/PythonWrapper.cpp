//------------------------
// Includes
//------------------------
// std
#include <vector>

// Boost
#include <boost/python/class.hpp>
#include <boost/python/bases.hpp>
#include <boost/python/call_method.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/module.hpp>
#include <boost/python/reference_existing_object.hpp>
#include <boost/python/return_internal_reference.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/copy_non_const_reference.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/cstdint.hpp>
#include <boost/python/def.hpp>

#include <boost/python/pure_virtual.hpp>

// Kernel
#include <MantidKernel/EnvironmentHistory.h>
#include <MantidKernel/Property.h>
#include <MantidKernel/PropertyManager.h>

// API
#include <MantidAPI/IAlgorithm.h>
#include <MantidAPI/MatrixWorkspace.h>
#include <MantidAPI/ITableWorkspace.h>
#include <MantidAPI/WorkspaceGroup.h>
#include <MantidAPI/Instrument.h>
#include <MantidAPI/ParInstrument.h>
#include <MantidAPI/Sample.h>


// Geometry
#include <MantidGeometry/Instrument/ObjComponent.h>
#include <MantidGeometry/Instrument/ParObjComponent.h>
#include <MantidGeometry/Instrument/Component.h>
#include <MantidGeometry/Instrument/ParametrizedComponent.h>
#include <MantidGeometry/Instrument/CompAssembly.h>
#include <MantidGeometry/Instrument/ParCompAssembly.h>
#include <MantidGeometry/Instrument/Detector.h>
#include <MantidGeometry/Instrument/ParDetector.h>

#include <MantidGeometry/Quat.h>
#include <MantidGeometry/V3D.h>

// PythonAPI
#include <MantidPythonAPI/FrameworkManager.h>

// I'll put this back in when we start to do something with it
//#include "MantidPythonAPI/PyAlgorithm.h"


//Given its frequent use, this makes things more readable
using namespace boost::python;

namespace Mantid 
{

namespace PythonAPI
{

  /**
   * Wrappers for the C++ code
   * @name Python wrappers
   */
  //@cond
  //@{

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
//@endcond
//@}


/// Declare functions with overloaded names
/**@name Function overloads. */
//@{
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Mantid_PythonAPI_FrameworkManager_createAlgorithm_overloads_1, createAlgorithm, 1, 1)
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Mantid_PythonAPI_FrameworkManager_createAlgorithm_overloads_2, createAlgorithm, 2, 2)
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Mantid_PythonAPI_FrameworkManager_createAlgorithm_overloads_3, createAlgorithm, 2, 2)
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Mantid_PythonAPI_FrameworkManager_createAlgorithm_overloads_4, createAlgorithm, 3, 3)
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Mantid_PythonAPI_FrameworkManager_execute_overloads_1, execute, 2, 2)
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Mantid_PythonAPI_FrameworkManager_execute_overloads_2, execute, 3, 3)
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Mantid_API_MatrixWorkspace_isDistribution_overloads_1, isDistribution, 0, 0)
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Mantid_API_MatrixWorkspace_isDistribution_overloads_2, isDistribution, 1, 1)
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Mantid_API_Sample_getLogData_overloads_1, getLogData, 0, 0)
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Mantid_API_Sample_getLogData_overloads_2, getLogData, 1, 1)
//@}

} //PythonAPI namespace

} //Mantid namespace


/**
 * A macro to quickly register container classes
 */
#define REGISTER_VECTOR_WITH_PYTHON(classname, pythonname) \
  class_< std::vector<classname> >( pythonname ) \
  .def( vector_indexing_suite<std::vector<classname> >() ) \
  .def("__len__", &std::vector<classname>::size) \
  ;

/**
 * A macro to register shared pointers to Python. It registers
 * both const and non-const versions
 */
#define REGISTER_SHAREDPTR_WITH_PYTHON(type) \
  register_ptr_to_python< boost::shared_ptr<type> >();

/**
 * The actual module definition begins here. The names are different for
 * Windows and Linux due to the difference in library names
 */
#ifdef _WIN32
BOOST_PYTHON_MODULE(MantidPythonAPI)
#else
BOOST_PYTHON_MODULE(libMantidPythonAPI)
#endif
{
  /**
   * Python needs to know how to handle the some standard containers
   * @name Standard containers
   */
  //@{
  /// A vector of strings
  REGISTER_VECTOR_WITH_PYTHON(std::string, "StringVector")
  /// A vector of doubles
  REGISTER_VECTOR_WITH_PYTHON(double, "DoubleVector")
  ///  A vector of integers
  REGISTER_VECTOR_WITH_PYTHON(int, "IntVector")
  /// A vector of AlgorithmHistory objects
  REGISTER_VECTOR_WITH_PYTHON(Mantid::API::AlgorithmHistory, "AlgHistVector")
  /// A vector of PropertyHistory objects
  REGISTER_VECTOR_WITH_PYTHON(Mantid::Kernel::PropertyHistory, "PropHistVector")	  
  /// A vector of Property pointers
  REGISTER_VECTOR_WITH_PYTHON(Mantid::Kernel::Property*, "PropPtrVector")	  
  /// A vector of MatrixWorkspace pointers
  REGISTER_VECTOR_WITH_PYTHON(Mantid::API::MatrixWorkspace*, "MatrixWorkspacePtrVector")	  

  //@}

  /**
   * Register shared pointers with Python
   * @name Mantid shared pointers
   */
  //@{
  /// A pointer to a MatrixWorkspace object
  register_ptr_to_python<Mantid::API::MatrixWorkspace*>();

  /// A pointer to a TableWorkspace object
  register_ptr_to_python<Mantid::API::ITableWorkspace*>();

  /// A pointer to a WorkspaceGroup object
  register_ptr_to_python<Mantid::API::WorkspaceGroup*>();

  /// A pointer to an Sample
  REGISTER_SHAREDPTR_WITH_PYTHON( Mantid::API::Sample )

  /// A pointer to an IInstrument object
  REGISTER_SHAREDPTR_WITH_PYTHON( Mantid::API::IInstrument )
    
  /// A pointer to a Unit object
  REGISTER_SHAREDPTR_WITH_PYTHON( Mantid::Kernel::Unit )

  /// A pointer to an IComponent
  REGISTER_SHAREDPTR_WITH_PYTHON( Mantid::Geometry::IComponent )

  /// A pointer to an IComponent
  REGISTER_SHAREDPTR_WITH_PYTHON( Mantid::Geometry::ICompAssembly )

  /// A pointer to an IDetector object
  REGISTER_SHAREDPTR_WITH_PYTHON( Mantid::Geometry::IDetector )

  /// A pointer to an IObjComponent
  REGISTER_SHAREDPTR_WITH_PYTHON( Mantid::Geometry::IObjComponent )
  //@}
  
  //Make this namespace available
  using namespace Mantid::PythonAPI;

  //===================================================================================================================================
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
     .def("createAlgorithm", (Mantid::API::IAlgorithm* (Mantid::PythonAPI::FrameworkManager::*)(const std::string&) )&Mantid::PythonAPI::FrameworkManager::createAlgorithm, 
	  return_value_policy< reference_existing_object >(), Mantid_PythonAPI_FrameworkManager_createAlgorithm_overloads_1())
     .def("createAlgorithm", (Mantid::API::IAlgorithm* (Mantid::PythonAPI::FrameworkManager::*)(const std::string&, const int&) )&Mantid::PythonAPI::FrameworkManager::createAlgorithm, 
	  return_value_policy< reference_existing_object >(), Mantid_PythonAPI_FrameworkManager_createAlgorithm_overloads_2())
     .def("createAlgorithm", (Mantid::API::IAlgorithm* (Mantid::PythonAPI::FrameworkManager::*)(const std::string&, const std::string&) )&Mantid::PythonAPI::FrameworkManager::createAlgorithm, 
	  return_value_policy< reference_existing_object >(), Mantid_PythonAPI_FrameworkManager_createAlgorithm_overloads_3())
     .def("createAlgorithm", (Mantid::API::IAlgorithm* (Mantid::PythonAPI::FrameworkManager::*)(const std::string&, const std::string&, const int&) )&Mantid::PythonAPI::FrameworkManager::createAlgorithm, 
	  return_value_policy< reference_existing_object >(), Mantid_PythonAPI_FrameworkManager_createAlgorithm_overloads_4())
     .def("execute", (Mantid::API::IAlgorithm* (Mantid::PythonAPI::FrameworkManager::*)(const std::string&, const std::string&) )&Mantid::PythonAPI::FrameworkManager::execute, 
	  return_value_policy< reference_existing_object >(), Mantid_PythonAPI_FrameworkManager_execute_overloads_1())
     .def("execute", (Mantid::API::IAlgorithm* (Mantid::PythonAPI::FrameworkManager::*)(const std::string&, const std::string&, const int&) ) &Mantid::PythonAPI::FrameworkManager::execute, 
	  return_value_policy< reference_existing_object >(), Mantid_PythonAPI_FrameworkManager_execute_overloads_2())
     .def("getMatrixWorkspace", &Mantid::PythonAPI::FrameworkManager::getMatrixWorkspace, return_value_policy< reference_existing_object >())
     .def("getTableWorkspace", &Mantid::PythonAPI::FrameworkManager::getTableWorkspace, return_value_policy< reference_existing_object >())
     .def("getMatrixWorkspaceGroup", &Mantid::PythonAPI::FrameworkManager::getMatrixWorkspaceGroup)
     .def("deleteWorkspace", &Mantid::PythonAPI::FrameworkManager::deleteWorkspace)
     .def("getWorkspaceNames", &Mantid::PythonAPI::FrameworkManager::getWorkspaceNames)
     .def("getWorkspaceGroupNames", &Mantid::PythonAPI::FrameworkManager::getWorkspaceGroupNames)
     .def("getWorkspaceGroupEntries", &Mantid::PythonAPI::FrameworkManager::getWorkspaceGroupEntries)
     .def("createPythonSimpleAPI", &Mantid::PythonAPI::FrameworkManager::createPythonSimpleAPI)
     .def("sendLogMessage", &Mantid::PythonAPI::FrameworkManager::sendLogMessage)
     .def("addPythonAlgorithm", &Mantid::PythonAPI::FrameworkManager::addPythonAlgorithm)
     .def("executePythonAlgorithm", &Mantid::PythonAPI::FrameworkManager::executePythonAlgorithm)
     ;
   //===================================================================================================================================

   /**
    * IAlgorithm wrapper
    * Interface for setting algorithm properties and running the algorithm. Note that execute is not threaded
    */
   class_< Mantid::API::IAlgorithm, boost::noncopyable>("IAlgorithm", no_init)
     .def("initialize", &Mantid::API::IAlgorithm::initialize)
     .def("execute", &Mantid::API::IAlgorithm::execute)
     .def("isInitialized", &Mantid::API::IAlgorithm::isInitialized)
     .def("isExecuted", &Mantid::API::IAlgorithm::isExecuted)
     .def("setPropertyValue", &Mantid::API::IAlgorithm::setPropertyValue)
     .def("getPropertyValue", &Mantid::API::IAlgorithm::getPropertyValue)
     .def("getProperties", &Mantid::API::IAlgorithm::getProperties, return_value_policy< copy_const_reference >())
     ;

   /**
    * Base workspace interface
    */
   class_<Mantid::API::Workspace, boost::noncopyable>("Workspace", no_init)
     .def("getTitle", &Mantid::API::Workspace::getTitle, return_value_policy< copy_const_reference >())
     .def("getComment", &Mantid::API::MatrixWorkspace::getComment, return_value_policy< copy_const_reference >() )
     .def("getMemorySize", &Mantid::API::Workspace::getMemorySize)
     ;
   
   //MatrixWorkspace class
   class_< Mantid::API::MatrixWorkspace, bases<Mantid::API::Workspace>, boost::noncopyable >("MatrixWorkspace", no_init)
     .def("getNumberHistograms", &Mantid::API::MatrixWorkspace::getNumberHistograms)
     .def("readX", &Mantid::API::MatrixWorkspace::readX, return_value_policy< return_by_value >()) 
     .def("readY", &Mantid::API::MatrixWorkspace::readY, return_value_policy< return_by_value >()) 
     .def("readE", &Mantid::API::MatrixWorkspace::readE, return_value_policy< return_by_value >()) 
     .def("blocksize", &Mantid::API::MatrixWorkspace::blocksize)
     .def("isDistribution", (const bool& (Mantid::API::MatrixWorkspace::*)() const)&Mantid::API::MatrixWorkspace::isDistribution, return_value_policy< copy_const_reference >(), 
	  Mantid_API_MatrixWorkspace_isDistribution_overloads_1())
     .def("isDistribution", (bool& (Mantid::API::MatrixWorkspace::*)(bool))&Mantid::API::MatrixWorkspace::isDistribution, return_value_policy< copy_non_const_reference >(),
	  Mantid_API_MatrixWorkspace_isDistribution_overloads_2())
     .def("getInstrument", &Mantid::API::MatrixWorkspace::getInstrument)
     .def("getDetector", &Mantid::API::MatrixWorkspace::getDetector)
     .def("getSampleDetails", &Mantid::API::MatrixWorkspace::getSample)
     ;
   
   // TableWorkspace class
   class_< Mantid::API::ITableWorkspace, bases<Mantid::API::Workspace>, boost::noncopyable >("ITableWorkspace", no_init)
     .def("getColumnCount", &Mantid::API::ITableWorkspace::columnCount)
     .def("getRowCount", &Mantid::API::ITableWorkspace::rowCount)
     .def("getColumnNames",&Mantid::API::ITableWorkspace::getColumnNames)
     .def("getInt", Mantid::PythonAPI::ITableWorkspace_GetInteger, return_value_policy< copy_non_const_reference >())
     .def("getDouble", Mantid::PythonAPI::ITableWorkspace_GetDouble, return_value_policy< copy_non_const_reference >())
     .def("getString", Mantid::PythonAPI::ITableWorkspace_GetString, return_value_policy< copy_non_const_reference >())
     ;

   // Workspace group class
   class_< Mantid::API::WorkspaceGroup, boost::noncopyable >("WorkspaceGroup", no_init)
     .def("size", &Mantid::API::WorkspaceGroup::getNumberOfEntries)
     .def("getNames", &Mantid::API::WorkspaceGroup::getNames, return_value_policy< copy_const_reference >())
     .def("add", &Mantid::API::WorkspaceGroup::add)
     .def("remove", &Mantid::API::WorkspaceGroup::remove)
     ;

   //Sample class
   class_< Mantid::API::Sample, boost::noncopyable >("Sample", no_init)
     .def("getLogData", (const std::vector<Mantid::Kernel::Property*>& (Mantid::API::Sample::*)() const)&Mantid::API::Sample::getLogData, return_value_policy<copy_const_reference>(),
	  Mantid_API_Sample_getLogData_overloads_1())
     .def("getLogData", (Mantid::Kernel::Property* (Mantid::API::Sample::*)(const std::string&) const)&Mantid::API::Sample::getLogData, return_value_policy< reference_existing_object>(),
	  Mantid_API_Sample_getLogData_overloads_2())
     .def("getName", &Mantid::API::Sample::getName, return_value_policy<copy_const_reference>())
     .def("getProtonCharge", &Mantid::API::Sample::getProtonCharge, return_value_policy< copy_const_reference>())
     ;

   //V3D class
   class_< Mantid::Geometry::V3D >("V3D", init<double, double, double>())
     .def("getX", &Mantid::Geometry::V3D::X, return_value_policy< copy_const_reference >())
     .def("getY", &Mantid::Geometry::V3D::Y, return_value_policy< copy_const_reference >())
     .def("getZ", &Mantid::Geometry::V3D::Z, return_value_policy< copy_const_reference >())
     .def("distance", &Mantid::Geometry::V3D::distance)
     .def("angle", &Mantid::Geometry::V3D::angle)
     .def("zenith", &Mantid::Geometry::V3D::zenith)
     .def("scalar_prod", &Mantid::Geometry::V3D::scalar_prod)
     .def("cross_prod", &Mantid::Geometry::V3D::scalar_prod)
     .def("norm", &Mantid::Geometry::V3D::norm)
     .def("norm2", &Mantid::Geometry::V3D::norm2)
     .def(self + self)
     .def(self += self)
     .def(self - self)
     .def(self -= self)
     .def(self * self)
     .def(self *= self)
     .def(self / self)
     .def(self /= self)
     .def(self * int())
     .def(self *= int())
     .def(self * double())
     .def(self *= double())
     .def(self < self)
     .def(self == self)
     .def(self_ns::str(self))
     ;
   //Quat class
   class_< Mantid::Geometry::Quat >("Quaternion", init<double, double, double, double>())
     .def("rotate", &Mantid::Geometry::Quat::rotate)
     .def("real", &Mantid::Geometry::Quat::real)
     .def("imagI", &Mantid::Geometry::Quat::imagI)
     .def("imagJ", &Mantid::Geometry::Quat::imagJ)
     .def("imagK", &Mantid::Geometry::Quat::imagK)
     .def(self + self)
     .def(self += self)
     .def(self - self)
     .def(self -= self)
     .def(self * self)
     .def(self *= self)
     .def(self == self)
     .def(self != self)
     .def(self_ns::str(self))
     ;

   //=================================== Geometry classes ===================================
   /**
    * Both the interface and concrete implementations need to be exposed here so that Python
    * sees the correct object type and not just always IComponent. This enables getComponentByName
    * to return an IComponent pointer in C++ while still having an underlying object in Python that will accept
    * the usage of ICompAssembly methods
    */
   //IComponent class
   class_<Mantid::Geometry::IComponent, boost::noncopyable>("IComponent", no_init)
     .def("getPos", &Mantid::Geometry::IComponent::getPos)
     .def("getName", &Mantid::Geometry::IComponent::getName)
     .def("type", &Mantid::Geometry::IComponent::type)
     ;
   
   //ICompAssembly class
   class_<Mantid::Geometry::ICompAssembly, boost::python::bases<Mantid::Geometry::IComponent>, boost::noncopyable>("ICompAssembly", no_init)
     .def("nElements", &Mantid::Geometry::ICompAssembly::nelements)
     .def("__getitem__", &Mantid::Geometry::ICompAssembly::operator[])
     ;
   
   //IInstrument class
   class_< Mantid::API::IInstrument, boost::python::bases<Mantid::Geometry::ICompAssembly>, boost::noncopyable>("IInstrument", no_init)
     .def("getSample", &Mantid::API::IInstrument::getSample)
     .def("getSource", &Mantid::API::IInstrument::getSource)
     .def("getComponentByName", &Mantid::API::IInstrument::getComponentByName)
     ;

   //IObjComponent class
   class_< Mantid::Geometry::IObjComponent, boost::python::bases<Mantid::Geometry::IComponent>, boost::noncopyable>("IObjComponent", no_init)
     ;

   //IDetector Class
   class_< Mantid::Geometry::IDetector, boost::noncopyable>("IDetector", no_init)
     .def("getID", &Mantid::Geometry::IDetector::getID)
     .def("isMasked", &Mantid::Geometry::IDetector::isMasked)
     .def("isMonitor", &Mantid::Geometry::IDetector::isMonitor)
     .def("solidAngle", &Mantid::Geometry::IDetector::solidAngle)
     .def("getPos", &Mantid::Geometry::IDetector::getPos)
     .def("getDistance", &Mantid::Geometry::IDetector::getDistance)
     .def("getTwoTheta", &Mantid::Geometry::IDetector::getTwoTheta)
     .def("getPhi", &Mantid::Geometry::IDetector::getPhi)
     .def("getNumberParameter", &Mantid::Geometry::IDetector::getNumberParameter)
     .def("getPositionParameter", &Mantid::Geometry::IDetector::getPositionParameter)
     .def("getRotationParameter", &Mantid::Geometry::IDetector::getRotationParameter)
   ;
   //-------------------------------------------------------------------
   /**
    * Concrete implementations
    */
   //Component class
   class_<Mantid::Geometry::Component, bases<Mantid::Geometry::IComponent>, boost::noncopyable>("Component", no_init)
     ;
   //ParameterizedComponent
   class_<Mantid::Geometry::ParametrizedComponent, bases<Mantid::Geometry::IComponent>, boost::noncopyable>("ParameterizedComponent", no_init)
     ;
   //CompAssembly class
   class_<Mantid::Geometry::CompAssembly, boost::python::bases<Mantid::Geometry::ICompAssembly>, boost::noncopyable>("CompAssembly", no_init)
     ;
   //ParCompAssembly class
   class_<Mantid::Geometry::ParCompAssembly, boost::python::bases<Mantid::Geometry::ICompAssembly>, boost::noncopyable>("ParCompAssembly", no_init)
     ;
   
   //Instrument class
   class_< Mantid::API::Instrument, boost::python::bases<Mantid::API::IInstrument>, boost::noncopyable>("Instrument", no_init)
     ;
   //Instrument class
   class_< Mantid::API::ParInstrument, boost::python::bases<Mantid::API::IInstrument>, boost::noncopyable>("ParInstrument", no_init)
     ;
   //Detector
   class_< Mantid::Geometry::Detector, boost::python::bases<Mantid::Geometry::IDetector>, boost::noncopyable>("Detector", no_init)
     ;
   //ParDetector
   class_< Mantid::Geometry::ParDetector, boost::python::bases<Mantid::Geometry::IDetector>, boost::noncopyable>("ParDetector", no_init)
    ;
   //-------------------------------------------------------------------
   
//========================================= Python algorithms ======================================
   /** I'll put this back in when we start doing something with them */
   //    //PyAlgorithm Class
   //    class_< Mantid::PythonAPI::PyAlgorithm, boost::noncopyable, Mantid_PythonAPI_PyAlgorithm_Wrapper >("PyAlgorithm", init< std::string >())
   //      .def("name", &Mantid::PythonAPI::PyAlgorithm::name, &Mantid_PythonAPI_PyAlgorithm_Wrapper::default_name)
   //      .def("PyInit", &Mantid::PythonAPI::PyAlgorithm::PyInit, &Mantid_PythonAPI_PyAlgorithm_Wrapper::default_PyInit)
   //      .def("PyExec", &Mantid::PythonAPI::PyAlgorithm::PyExec, &Mantid_PythonAPI_PyAlgorithm_Wrapper::default_PyExec)
//      ;

}

