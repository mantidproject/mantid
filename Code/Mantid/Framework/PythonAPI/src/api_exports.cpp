//
// Wrappers for classes in the API namespace
//
#include "MantidPythonAPI/api_exports.h"
#include "MantidPythonAPI/stl_proxies.h"
#include "MantidPythonAPI/WorkspaceProxies.h"
#include <string>
#include <ostream>

// API
#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/IEventList.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

#include "MantidPythonAPI/PyAlgorithmWrapper.h"
//Poco
#include <Poco/ActiveResult.h>
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IPeak.h"

namespace Mantid
{
namespace PythonAPI
{
using namespace API;
using namespace Geometry;
using namespace boost::python;

  //@cond
  //---------------------------------------------------------------------------
  // Class export functions
  //---------------------------------------------------------------------------

  // Overloads for create*Algorithms function which has 1 optional argument
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(FM_createManagedAlgorithmOverloader, PythonAPI::FrameworkManagerProxy::createManagedAlgorithm, 1, 2)
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(FM_createUnmanagedAlgorithmOverloader, PythonAPI::FrameworkManagerProxy::createUnmanagedAlgorithm, 1, 2)
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(FM_getRegisteredAlgorithmOverloader, PythonAPI::FrameworkManagerProxy::getRegisteredAlgorithms, 0, 1)

  void export_frameworkmanager()
  {
    /** 
     * Python Framework class (note that this is not the API::FrameworkManager, there is another in 
     * PythonAPI::FrameworkManager)
     * This is the main class through which Python interacts with Mantid and with the exception of PyAlgorithm and V3D, 
     * is the only one directly instantiable in Python
     */
    class_<FrameworkManagerProxy, FrameworkProxyCallback, boost::noncopyable>("FrameworkManager")
      .def("clear", &FrameworkManagerProxy::clear)
      .def("clearAlgorithms", &FrameworkManagerProxy::clearAlgorithms)
      .def("clearData", &FrameworkManagerProxy::clearData)
      .def("clearInstruments", &FrameworkManagerProxy::clearInstruments)
      .def("isAlgorithmName", &FrameworkManagerProxy::isAlgorithmName)
      .def("algorithmDeprecationMessage", &FrameworkManagerProxy::algorithmDeprecationMessage)
      .def("createManagedAlgorithm", &FrameworkManagerProxy::createManagedAlgorithm, 
	   FM_createManagedAlgorithmOverloader()[return_internal_reference<>()] )
      .def("createUnmanagedAlgorithm", &FrameworkManagerProxy::createUnmanagedAlgorithm, 
	   FM_createUnmanagedAlgorithmOverloader()[return_value_policy< return_by_value >()])
      .def("_getPropertyOrder", &FrameworkManagerProxy::getPropertyOrder, return_internal_reference<>())
      .def("createAlgorithmDocs", &FrameworkManagerProxy::createAlgorithmDocs)
      .def("registerPyAlgorithm", &FrameworkManagerProxy::registerPyAlgorithm)
      .def("_getRegisteredAlgorithms", &FrameworkManagerProxy::getRegisteredAlgorithms, 
	   FM_getRegisteredAlgorithmOverloader())
      .def("_observeAlgFactoryUpdates", &FrameworkManagerProxy::observeAlgFactoryUpdates)
      .def("deleteWorkspace", &FrameworkManagerProxy::deleteWorkspace)
      .def("getWorkspaceNames", &FrameworkManagerProxy::getWorkspaceNames)
      .def("getWorkspaceGroupNames", &FrameworkManagerProxy::getWorkspaceGroupNames)
      .def("getWorkspaceGroupEntries", &FrameworkManagerProxy::getWorkspaceGroupEntries)
      .def("sendErrorMessage", &FrameworkManagerProxy::sendErrorMessage)
      .def("sendWarningMessage", &FrameworkManagerProxy::sendWarningMessage)
      .def("sendLogMessage", &FrameworkManagerProxy::sendLogMessage)
      .def("sendInformationMessage", &FrameworkManagerProxy::sendInformationMessage)
      .def("sendDebugMessage", &FrameworkManagerProxy::sendDebugMessage)
      .def("workspaceExists", &FrameworkManagerProxy::workspaceExists)
      .def("getConfigProperty", &FrameworkManagerProxy::getConfigProperty)
      .def("releaseFreeMemory", &FrameworkManagerProxy::releaseFreeMemory)
      .def("_getRawIEventWorkspacePointer", &FrameworkManagerProxy::retrieveIEventWorkspace)
      .def("_getRawIPeaksWorkspacePointer", &FrameworkManagerProxy::retrieveIPeaksWorkspace)
      .def("_getRawIMDWorkspacePointer", &FrameworkManagerProxy::retrieveIMDWorkspace)
      .def("_getRawIMDHistoWorkspacePointer", &FrameworkManagerProxy::retrieveIMDHistoWorkspace)
      .def("_getRawIMDEventWorkspacePointer", &FrameworkManagerProxy::retrieveIMDEventWorkspace)
      .def("_getRawMatrixWorkspacePointer", &FrameworkManagerProxy::retrieveMatrixWorkspace)
      .def("_getRawTableWorkspacePointer", &FrameworkManagerProxy::retrieveTableWorkspace)
      .def("_getRawWorkspaceGroupPointer", &FrameworkManagerProxy::retrieveWorkspaceGroup)
      .def("_workspaceRemoved", &FrameworkProxyCallback::default_workspaceRemoved)
      .def("_workspaceReplaced", &FrameworkProxyCallback::default_workspaceReplaced)
      .def("_workspaceAdded", &FrameworkProxyCallback::default_workspaceAdded)
      .def("_workspaceStoreCleared", &FrameworkProxyCallback::default_workspaceStoreCleared)
      .def("_algorithmFactoryUpdated", &FrameworkProxyCallback::default_algorithmFactoryUpdated) 
    ;
  }

  /**
   * Function used to define the IAlgorithm.setWorkspaceProperty() method in python.
   * Since getProperty/setProperty only deal with strings, we need a new method if we
   * want to set workspace properties by passing a pointer to an actual workspace.
   * This is necessary to be able to run sub-algorithm within a PythonAlgorithm.
   * TODO: This can be removed if we properly expose the getProperty/setProperty
   * methods to deal with the type of the properties as opposed to strings.
   */
  void _setMatrixWorkspaceProperty(API::IAlgorithm& self, const std::string & prop_name, API::MatrixWorkspace_sptr workspace)
  {
    self.setProperty(prop_name,workspace);
  }

  /**
   * Function used to define the IAlgorithm.getWorkspaceProperty() method in python.
   */
  API::MatrixWorkspace_sptr _getMatrixWorkspaceProperty(API::IAlgorithm& self, const std::string & prop_name)
  {
    return self.getProperty(prop_name);
  }

  // Overloads for createSubAlgorithm function which has 1 optional argument
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(PyAlgorithmBase_createSubAlgorithmOverloader, PythonAPI::PyAlgorithmBase::_createSubAlgorithm, 1, 2)

  void export_ialgorithm()
  {
    
    class_<Poco::ActiveResult<bool> >("ActiveResult_bool", no_init)
      .def("available", &Poco::ActiveResult<bool>::available)
      .def("wait", (void (Poco::ActiveResult<bool>::*)())&Poco::ActiveResult<bool>::wait)
      .def("data", (bool& (Poco::ActiveResult<bool>::*)() const)&Poco::ActiveResult<bool>::data,return_value_policy< copy_non_const_reference >())
      ;

    register_ptr_to_python<API::IAlgorithm*>();
    register_ptr_to_python<boost::shared_ptr<API::IAlgorithm> >();

    class_< API::IAlgorithm, boost::noncopyable>("IAlgorithm", no_init)
      .def("name", &API::IAlgorithm::name)
      .def("version", &API::IAlgorithm::version)
      .def("category", &API::IAlgorithm::category)
      .def("categories", &API::IAlgorithm::categories)
      .def("alias", &API::IAlgorithm::alias)
      .def("getOptionalMessage", &API::IAlgorithm::getOptionalMessage)
      .def("getWikiSummary", &API::IAlgorithm::getWikiSummary)
      .def("getWikiDescription", &API::IAlgorithm::getWikiDescription)
      .def("initialize", &API::IAlgorithm::initialize)
      .def("execute", &API::IAlgorithm::execute)
      .def("executeAsync", &API::IAlgorithm::executeAsync)
      .def("isRunningAsync", &API::IAlgorithm::isRunningAsync)
      .def("isInitialized", &API::IAlgorithm::isInitialized)
      .def("isLogging", &API::IAlgorithm::isLogging)
      .def("isExecuted", &API::IAlgorithm::isExecuted)
      .def("setChild", &API::IAlgorithm::setChild)
      .def("setLogging", &API::IAlgorithm::setLogging)
      .def("setAlwaysStoreInADS", &API::IAlgorithm::setAlwaysStoreInADS)
      .def("setRethrows", &API::IAlgorithm::setRethrows)
      .def("existsProperty", &API::IAlgorithm::existsProperty)
      .def("setPropertyValue", &API::IAlgorithm::setPropertyValue)
      .def("getPropertyValue", &API::IAlgorithm::getPropertyValue)
      .def("getProperties", &API::IAlgorithm::getProperties, return_value_policy< copy_const_reference >())
      .def("getProperty", &API::IAlgorithm::getPointerToProperty, return_value_policy<return_by_value>())
      .def("_setWorkspaceProperty", &_setMatrixWorkspaceProperty)
      .def("_getWorkspaceProperty", &_getMatrixWorkspaceProperty)
      // Special methods
      .def("__str__", &API::IAlgorithm::toString)
      ;
    class_< API::Algorithm, bases<API::IAlgorithm>, boost::noncopyable>("IAlgorithm", no_init)
      ;
    class_< API::CloneableAlgorithm, bases<API::Algorithm>, boost::noncopyable>("CloneableAlgorithm", no_init)
      ;
    
    /// Algorithm properties
    class_<Mantid::Kernel::PropertyWithValue<IAlgorithm_sptr>,
      bases<Mantid::Kernel::Property>, boost::noncopyable>("PropertyWithValue_AlgorithmProperty", no_init)
    .add_property("value", make_function(&Mantid::Kernel::PropertyWithValue<IAlgorithm_sptr>::operator(), return_value_policy<copy_const_reference>()))
      ;
  class_<API::AlgorithmProperty, bases<Kernel::PropertyWithValue<IAlgorithm_sptr> >, boost::noncopyable>("AlgorithmProperty", no_init)
      ;
    
    //PyAlgorithmBase
    //Save some typing for all of the templated declareProperty and getProperty methods
#define EXPORT_DECLAREPROPERTY(type, suffix)\
    .def("declareProperty_"#suffix,(void(PyAlgorithmBase::*)(const std::string &, type, const std::string &,const unsigned int))&PyAlgorithmBase::_declareProperty<type>) \
    .def("declareProperty_"#suffix,(void(PyAlgorithmBase::*)(const std::string &, type, Kernel::IValidator<type> &,const std::string &,const unsigned int))&PyAlgorithmBase::_declareProperty<type>) \
    .def("declareListProperty_"#suffix,(void(PyAlgorithmBase::*)(const std::string &, boost::python::list, const std::string &,const unsigned int))&PyAlgorithmBase::_declareListProperty<type>)\
    .def("declareListProperty_"#suffix,(void(PyAlgorithmBase::*)(const std::string &, boost::python::list, Kernel::IValidator<type> &,const std::string &,const unsigned int))&PyAlgorithmBase::_declareListProperty<type>) \
    .def("declareListProperty_"#suffix,(void(PyAlgorithmBase::*)(const std::string &, boost::python::list, Kernel::IValidator<std::vector<type> > &,const std::string &,const unsigned int))&PyAlgorithmBase::_declareListProperty<type>)
    
#define EXPORT_GETPROPERTY(type, suffix)\
    .def("getProperty_"#suffix,(type(PyAlgorithmBase::*)(const std::string &))&PyAlgorithmBase::_getProperty<type>)

#define EXPORT_GETLISTPROPERTY(type, suffix)\
    .def("getListProperty_"#suffix,(std::vector<type>(PyAlgorithmBase::*)(const std::string &))&PyAlgorithmBase::_getListProperty<type>)
    
    class_< PyAlgorithmBase, boost::shared_ptr<PyAlgorithmCallback>, bases<API::CloneableAlgorithm>, 
      boost::noncopyable >("PyAlgorithmBase")
      .enable_pickling()
      .def("_setWorkspaceProperty", &PyAlgorithmBase::_setWorkspaceProperty)
      .def("_setMatrixWorkspaceProperty", &PyAlgorithmBase::_setMatrixWorkspaceProperty)
      .def("_setTableWorkspaceProperty", &PyAlgorithmBase::_setTableWorkspaceProperty)
      .def("_declareFileProperty", &PyAlgorithmBase::_declareFileProperty)
      .def("_declareWorkspace", (void(PyAlgorithmBase::*)(const std::string &, const std::string &,const std::string &, const unsigned int))&PyAlgorithmBase::_declareWorkspace)
      .def("_declareWorkspace", (void(PyAlgorithmBase::*)(const std::string &, const std::string &,Kernel::IValidator<boost::shared_ptr<API::Workspace> >&,const std::string &, const unsigned int))&PyAlgorithmBase::_declareWorkspace)
      .def("_declareMatrixWorkspace", (void(PyAlgorithmBase::*)(const std::string &, const std::string &,const std::string &, const unsigned int))&PyAlgorithmBase::_declareMatrixWorkspace)
      .def("_declareMatrixWorkspace", (void(PyAlgorithmBase::*)(const std::string &, const std::string &,Kernel::IValidator<boost::shared_ptr<API::MatrixWorkspace> >&,const std::string &, const unsigned int))&PyAlgorithmBase::_declareMatrixWorkspace)
      .def("_declareTableWorkspace", &PyAlgorithmBase::_declareTableWorkspace)
      .def("_declareAlgorithmProperty", &PyAlgorithmBase::_declareAlgorithmProperty)
      .def("_setAlgorithmProperty", &PyAlgorithmBase::_setAlgorithmProperty)
      .def("_getAlgorithmProperty", &PyAlgorithmBase::_getAlgorithmProperty)
      .def("log", &PyAlgorithmBase::getLogger, return_internal_reference<>())
      .def("_createSubAlgorithm", &PyAlgorithmBase::_createSubAlgorithm, PyAlgorithmBase_createSubAlgorithmOverloader()[return_value_policy< return_by_value >()] )
      EXPORT_DECLAREPROPERTY(int, int)
      EXPORT_DECLAREPROPERTY(double, dbl)
      EXPORT_DECLAREPROPERTY(std::string, str)
      EXPORT_DECLAREPROPERTY(bool, bool)
      EXPORT_GETPROPERTY(int, int)
      EXPORT_GETLISTPROPERTY(int, int)
      EXPORT_GETPROPERTY(double, dbl)
      EXPORT_GETLISTPROPERTY(double, dbl)
      EXPORT_GETPROPERTY(bool, bool)
      ;

    //Leave the place tidy 
#undef EXPORT_DECLAREPROPERTY
#undef EXPORT_GETPROPERTY
#undef EXPORT_GETLISTPROPERTY
  }

  // Overloads for createSubAlgorithm function which has 1 optional argument
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Workspace_isDirtyOverloader, API::Workspace::isDirty, 0, 1)

  //-----------------------------------------------------------------------------------------------
  void export_workspace()
  {
    /// Shared pointer registration
    register_ptr_to_python<boost::shared_ptr<Workspace> >();
    
    class_<API::Workspace, boost::noncopyable>("Workspace", no_init)
      .def("getTitle", &API::Workspace::getTitle, 
         return_value_policy< return_by_value >())
      .def("setTitle", &API::Workspace::setTitle)
      .def("getComment", &API::MatrixWorkspace::getComment,
         return_value_policy< copy_const_reference >() )
      .def("getMemorySize", &API::Workspace::getMemorySize)
      .def("isDirty", &API::Workspace::isDirty, Workspace_isDirtyOverloader()[return_value_policy< return_by_value >()])
      .def("getName", &API::Workspace::getName, return_value_policy< copy_const_reference >())
      .def("__str__", &API::Workspace::getName, return_value_policy< copy_const_reference >())
      .def("getHistory", &API::Workspace::getHistory, return_internal_reference<>())
      ;
  }

  // Overloads for binIndexOf function which has 1 optional argument
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(MatrixWorkspace_binIndexOfOverloads, API::MatrixWorkspace::binIndexOf, 1, 2)

  //-----------------------------------------------------------------------------------------------
  void export_matrixworkspace()
  {
    /// Shared pointer registration
    register_ptr_to_python<boost::shared_ptr<MatrixWorkspace> >();

    // A vector of MatrixWorkspace pointers
    vector_proxy<MatrixWorkspace*>::wrap("stl_vector_matrixworkspace");
 

    /// Typedef for data access
    typedef MantidVec&(API::MatrixWorkspace::*data_modifier)(const std::size_t);

    //MatrixWorkspace class
    class_< API::MatrixWorkspace, bases<API::Workspace>, MatrixWorkspaceWrapper,
      boost::noncopyable >("MatrixWorkspace", no_init)
      .def("getNumberHistograms", &API::MatrixWorkspace::getNumberHistograms)
      .def("getNumberBins", &API::MatrixWorkspace::blocksize)
      .def("binIndexOf", &API::MatrixWorkspace::binIndexOf, MatrixWorkspace_binIndexOfOverloads() )
      .def("readX", &PythonAPI::MatrixWorkspaceWrapper::readX)
      .def("readY", &PythonAPI::MatrixWorkspaceWrapper::readY)
      .def("readE", &PythonAPI::MatrixWorkspaceWrapper::readE)
      .def("readDx", &PythonAPI::MatrixWorkspaceWrapper::readDx)
      .def("dataX", (data_modifier)&API::MatrixWorkspace::dataX, return_internal_reference<>() ) 
      .def("dataY", (data_modifier)&API::MatrixWorkspace::dataY, return_internal_reference<>() )
      .def("dataE", (data_modifier)&API::MatrixWorkspace::dataE, return_internal_reference<>() )
      .def("dataDx", (data_modifier)&API::MatrixWorkspace::dataDx, return_internal_reference<>() )
      .def("isDistribution", (const bool& (API::MatrixWorkspace::*)() const)&API::MatrixWorkspace::isDistribution, 
         return_value_policy< copy_const_reference >() )
      .def("setYUnitLabel", &API::MatrixWorkspace::setYUnitLabel)
      .def("setYUnit", &API::MatrixWorkspace::setYUnit)
      .def("setDistribution", (bool& (API::MatrixWorkspace::*)(const bool))&API::MatrixWorkspace::isDistribution, 
         return_value_policy<return_by_value>() )
      .def("getInstrument", (Geometry::Instrument_sptr (API::ExperimentInfo::*)())&API::ExperimentInfo::getInstrument)
      .def("getSpectrum", (ISpectrum * (MatrixWorkspace::*)(const size_t))&API::MatrixWorkspace::getSpectrum, return_internal_reference<>() )
      .def("getDetector", (Geometry::IDetector_sptr (API::MatrixWorkspace::*) (const size_t) const)&API::MatrixWorkspace::getDetector)
      .def("getRun", &API::MatrixWorkspace::mutableRun, return_internal_reference<>() )
      .def("getSample", &API::MatrixWorkspace::sample, return_internal_reference<>() )
      .def("getSampleInfo", &API::MatrixWorkspace::sample, return_internal_reference<>() )
      .def("getNumberAxes", &API::MatrixWorkspace::axes)
      .def("getAxis", &API::MatrixWorkspace::getAxis, return_internal_reference<>())
      .def("replaceAxis", &API::MatrixWorkspace::replaceAxis)
      // Deprecated, here for backwards compatability
      .def("blocksize", &API::MatrixWorkspace::blocksize)
      .def("getSampleDetails", &API::MatrixWorkspace::run, return_internal_reference<>() )
      ;

    //Operator overloads dispatch through the above structure. The typedefs save some typing
    typedef bool(*binary_fn5)(const API::MatrixWorkspace_sptr, const API::MatrixWorkspace_sptr,double);

      // Binary operations helpers
    def("_equals_op", (binary_fn5)&API::equals);

  }


  //-----------------------------------------------------------------------------------------------
  void export_IMDWorkspace()
  {
    register_ptr_to_python<API::IMDWorkspace_sptr>();

    // EventWorkspace class
    class_< IMDWorkspace, bases<API::Workspace>, boost::noncopyable >("IMDWorkspace", no_init)
        .def("getNPoints", &IMDWorkspace::getNPoints)
        .def("getNumDims", &IMDWorkspace::getNumDims)
        .def("getDimension", &IMDWorkspace::getDimension )
        ;

    //Operator overloads dispatch through the above structure. The typedefs save some typing
    typedef IMDWorkspace_sptr(*binary_fn_md_md)(const API::IMDWorkspace_sptr, const API::IMDWorkspace_sptr, const std::string &,const std::string &,bool, bool);
    typedef MatrixWorkspace_sptr(*binary_fn_mw_mw)(const API::MatrixWorkspace_sptr, const API::MatrixWorkspace_sptr, const std::string &,const std::string &,bool, bool);
    typedef WorkspaceGroup_sptr(*binary_fn_md_gp)(const API::IMDWorkspace_sptr, const API::WorkspaceGroup_sptr, const std::string &,const std::string &,bool, bool);
    typedef WorkspaceGroup_sptr(*binary_fn_gp_md)(const API::WorkspaceGroup_sptr, const API::IMDWorkspace_sptr, const std::string &,const std::string &,bool, bool);
    typedef WorkspaceGroup_sptr(*binary_fn_gp_gp)(const API::WorkspaceGroup_sptr, const API::WorkspaceGroup_sptr, const std::string &,const std::string &,bool, bool);

    typedef IMDHistoWorkspace_sptr(*binary_fn_mh_mh)(const API::IMDHistoWorkspace_sptr, const API::IMDHistoWorkspace_sptr, const std::string &,const std::string &,bool, bool);

    typedef IMDWorkspace_sptr(*binary_fn_md_db)(const API::IMDWorkspace_sptr, double, const std::string&,const std::string &,bool,bool);
    typedef MatrixWorkspace_sptr(*binary_fn_mw_db)(const API::MatrixWorkspace_sptr, double, const std::string&,const std::string &,bool,bool);
    typedef IMDHistoWorkspace_sptr(*binary_fn_mh_db)(const API::IMDHistoWorkspace_sptr, double, const std::string&,const std::string &,bool,bool);
    typedef WorkspaceGroup_sptr(*binary_fn_gp_db)(const API::WorkspaceGroup_sptr, double, const std::string&,const std::string &,bool,bool);

      // Binary operations helpers
    def("_binary_op", (binary_fn_md_md)&PythonAPI::performBinaryOp);
    def("_binary_op", (binary_fn_mw_mw)&PythonAPI::performBinaryOp);
    def("_binary_op", (binary_fn_md_gp)&PythonAPI::performBinaryOp);
    def("_binary_op", (binary_fn_gp_md)&PythonAPI::performBinaryOp);
    def("_binary_op", (binary_fn_gp_gp)&PythonAPI::performBinaryOp);
    def("_binary_op", (binary_fn_mh_mh)&PythonAPI::performBinaryOp);

    def("_binary_op", (binary_fn_md_db)&PythonAPI::performBinaryOpWithDouble);
    def("_binary_op", (binary_fn_mw_db)&PythonAPI::performBinaryOpWithDouble);
    def("_binary_op", (binary_fn_mh_db)&PythonAPI::performBinaryOpWithDouble);
    def("_binary_op", (binary_fn_gp_db)&PythonAPI::performBinaryOpWithDouble);

  }

  //-----------------------------------------------------------------------------------------------
  void export_IMDHistoWorkspace()
  {
    register_ptr_to_python<API::IMDHistoWorkspace_sptr>();

    // EventWorkspace class
    class_< IMDHistoWorkspace, bases<API::IMDWorkspace>, boost::noncopyable >("IMDHistoWorkspace", no_init)
      .def("signalAt", &IMDHistoWorkspace::signalAt, return_value_policy<copy_non_const_reference>())
      .def("errorSquaredAt", &IMDHistoWorkspace::errorSquaredAt, return_value_policy<copy_non_const_reference>())
      .def("setSignalAt", &IMDHistoWorkspace::setSignalAt)
      .def("setErrorSquaredAt", &IMDHistoWorkspace::setErrorSquaredAt)
      .def("setTo", &IMDHistoWorkspace::setTo)
      .def("getInverseVolume", &IMDHistoWorkspace::getInverseVolume, return_value_policy< return_by_value >())
      .def("getLinearIndex", (size_t(IMDHistoWorkspace::*)(size_t,size_t) const)  &IMDHistoWorkspace::getLinearIndex, return_value_policy< return_by_value >())
      .def("getLinearIndex", (size_t(IMDHistoWorkspace::*)(size_t,size_t,size_t) const)  &IMDHistoWorkspace::getLinearIndex, return_value_policy< return_by_value >())
      .def("getLinearIndex", (size_t(IMDHistoWorkspace::*)(size_t,size_t,size_t,size_t) const)  &IMDHistoWorkspace::getLinearIndex, return_value_policy< return_by_value >())
      .def("getCenter", &IMDHistoWorkspace::getCenter, return_value_policy< return_by_value >())
//      .def("__getitem__", &IMDHistoWorkspace::operator[])
        ;


  }

  //-----------------------------------------------------------------------------------------------
  void export_IMDDimension()
  {
    register_ptr_to_python<Geometry::IMDDimension_sptr>();

    class_< IMDDimension, boost::noncopyable >("IMDDimension", no_init)
        .def("getName", &IMDDimension::getName)
        .def("getMaximum", &IMDDimension::getMaximum)
        .def("getMinimum", &IMDDimension::getMinimum)
        .def("getNBins", &IMDDimension::getNBins)
        .def("getX", &IMDDimension::getX)
        .def("getDimensionId", &IMDDimension::getDimensionId)
        ;
  }

  //-----------------------------------------------------------------------------------------------
  void export_BoxController()
  {
    register_ptr_to_python<API::BoxController_sptr>();

    class_< BoxController, boost::noncopyable >("BoxController", no_init)
            .def("getNDims", &BoxController::getNDims)
            .def("getSplitThreshold", &BoxController::getSplitThreshold)
            .def("getSplitInto", &BoxController::getSplitInto)
            .def("getMaxDepth", &BoxController::getMaxDepth)
            .def("getTotalNumMDBoxes", &BoxController::getTotalNumMDBoxes)
            .def("getTotalNumMDGridBoxes", &BoxController::getTotalNumMDGridBoxes)
            .def("getAverageDepth", &BoxController::getAverageDepth)
            .def("isFileBacked", &BoxController::isFileBacked)
            .def("getFilename", &BoxController::getFilename, return_value_policy< copy_const_reference >())
            .def("useWriteBuffer", &BoxController::useWriteBuffer)
        ;
  }

  //-----------------------------------------------------------------------------------------------
  void export_IMDEventWorkspace()
  {
    register_ptr_to_python<API::IMDEventWorkspace_sptr>();

    // MDEventWorkspace class
    class_< IMDEventWorkspace, bases<API::Workspace>, boost::noncopyable >("IMDEventWorkspace", no_init)
            .def("getNPoints", &IMDEventWorkspace::getNPoints)
            .def("getNumDims", &IMDEventWorkspace::getNumDims)
            .def("getBoxController", (BoxController_sptr(IMDEventWorkspace::*)() )  &IMDEventWorkspace::getBoxController)
        ;
  }

  //-----------------------------------------------------------------------------------------------
  void export_IPeaksWorkspace()
  {
    register_ptr_to_python<API::IPeaksWorkspace_sptr>();

    // IPeaksWorkspace class
    class_< IPeaksWorkspace, bases<API::Workspace>, boost::noncopyable >("IPeaksWorkspace", no_init)
            .def("getNumberPeaks", &IPeaksWorkspace::getNumberPeaks)
            .def("addPeak", &IPeaksWorkspace::addPeak)
            .def("removePeak", &IPeaksWorkspace::removePeak)
            .def("getPeak", &IPeaksWorkspace::getPeakPtr, return_internal_reference<>() )
            .def("createPeak", &IPeaksWorkspace::createPeak, return_internal_reference<>() )
        ;
  }


  //-----------------------------------------------------------------------------------------------
  void export_IPeak()
  {
    register_ptr_to_python<IPeak*>();

    class_<IPeak, boost::noncopyable>("IPeak", no_init)
          .def("getDetectorID", &IPeak::getDetectorID)
          .def("setDetectorID", &IPeak::setDetectorID)
          .def("getRunNumber", &IPeak::getRunNumber)
          .def("setRunNumber", &IPeak::setRunNumber)
          .def("getH", &IPeak::getH)
          .def("getK", &IPeak::getK)
          .def("getL", &IPeak::getL)
          .def("getH", &IPeak::getH)
          .def("getHKL", &IPeak::getHKL)
          .def("setHKL", (void (IPeak::*)(double,double,double)) &IPeak::setHKL)
          .def("setH", &IPeak::setH)
          .def("setK", &IPeak::setK)
          .def("setL", &IPeak::setL)
          .def("getQLabFrame", &IPeak::getQLabFrame)
          .def("getQSampleFrame", &IPeak::getQSampleFrame)
          .def("setQLabFrame", &IPeak::setQLabFrame)
          .def("setQSampleFrame", &IPeak::setQSampleFrame)
          .def("setWavelength", &IPeak::setWavelength)
          .def("getWavelength", &IPeak::getWavelength)
          .def("getScattering", &IPeak::getScattering)
          .def("getDSpacing", &IPeak::getDSpacing)
          .def("getTOF", &IPeak::getTOF)
          .def("getInitialEnergy", &IPeak::getInitialEnergy)
          .def("getFinalEnergy", &IPeak::getFinalEnergy)
          .def("setInitialEnergy", &IPeak::setInitialEnergy)
          .def("setFinalEnergy", &IPeak::setFinalEnergy)
          .def("getIntensity", &IPeak::getIntensity)
          .def("getSigmaIntensity", &IPeak::getSigmaIntensity)
          .def("setIntensity", &IPeak::setIntensity)
          .def("setSigmaIntensity", &IPeak::setSigmaIntensity)
          .def("getBinCount", &IPeak::getBinCount)
          .def("setBinCount", &IPeak::setBinCount)
//          .def("getBankName", &IPeak::getBankName, return_value_policy< copy_const_reference >())
          .def("getRow", &IPeak::getRow)
          .def("getCol", &IPeak::getCol)
          .def("getDetPos", &IPeak::getDetPos)
          .def("getL1", &IPeak::getL1)
          .def("getL2", &IPeak::getL2)
          ;
  }


  //-----------------------------------------------------------------------------------------------
  void export_eventworkspace()
  {
    register_ptr_to_python<IEventWorkspace_sptr>();

    // EventWorkspace class
    class_< IEventWorkspace, bases<API::MatrixWorkspace>, boost::noncopyable >("EventWorkspace", no_init)
        .def("getNumberEvents", &IEventWorkspace::getNumberEvents)
        .def("getTofMin", &IEventWorkspace::getTofMin)
        .def("getTofMax", &IEventWorkspace::getTofMax)
        .def("getEventList", (IEventList*(IEventWorkspace::*)(const int) ) &IEventWorkspace::getEventListPtr, return_internal_reference<>())
        .def("clearMRU", &IEventWorkspace::clearMRU)
           ;
  }

  //-----------------------------------------------------------------------------------------------
  void export_ISpectrum()
  {
    register_ptr_to_python<ISpectrum*>();

    class_<ISpectrum, boost::noncopyable>("ISpectrum", no_init)
      .def("addDetectorID", &ISpectrum::addDetectorID)
      .def("setDetectorID", &ISpectrum::setDetectorID)
      .def("hasDetectorID", &ISpectrum::hasDetectorID)
      .def("clearDetectorIDs", &ISpectrum::clearDetectorIDs)
      .def("getSpectrumNo", &ISpectrum::getSpectrumNo)
      .def("setSpectrumNo", &ISpectrum::setSpectrumNo)
      ;
  }

  //-----------------------------------------------------------------------------------------------
  void export_EventList()
  {
    register_ptr_to_python<IEventList *>();

    class_< IEventList, boost::noncopyable >("IEventList", no_init)
        .def("getEventType", &IEventList::getEventType)
        .def("switchTo", &IEventList::switchTo)
        .def("clear", &IEventList::clear)
        .def("isSortedByTof", &IEventList::isSortedByTof)
        .def("getNumberEvents", &IEventList::getNumberEvents)
        .def("getMemorySize", &IEventList::getMemorySize)
        .def("integrate", &IEventList::integrate)
        .def("convertTof", &IEventList::convertTof)
        .def("scaleTof", &IEventList::scaleTof)
        .def("addTof", &IEventList::addTof)
        .def("addPulsetime", &IEventList::addPulsetime)
        .def("maskTof", &IEventList::maskTof)
        .def("getTofs", (std::vector<double>(IEventList::*)(void)const) &IEventList::getTofs,
            "Get a vector of the TOFs of the events")
        .def("getPulseTimes", &IEventList::getPulseTimes, "Get a vector of the pulse times of the events")
        .def("getTofMin", &IEventList::getTofMin)
        .def("getTofMax", &IEventList::getTofMax)
        .def("setTofs", &IEventList::setTofs)
        .def("multiply", (void(IEventList::*)(const double,const double)) &IEventList::multiply)
        .def("divide", (void(IEventList::*)(const double,const double)) &IEventList::multiply)
        ;
  }


  //-----------------------------------------------------------------------------------------------
  void export_tableworkspace()
  {
    // Declare the pointer
    register_ptr_to_python<API::ITableWorkspace_sptr>();
    
    // Table workspace
    // Some function pointers since MSVC can't figure out the function to call when 
    // placing this directly in the .def functions below
    typedef int&(ITableWorkspace::*get_integer_ptr)(const std::string &, size_t);
    typedef double&(ITableWorkspace::*get_double_ptr)(const std::string &, size_t);
    typedef std::string&(ITableWorkspace::*get_string_ptr)(const std::string &, size_t);

    // TableWorkspace class
    class_< ITableWorkspace, bases<API::Workspace>, boost::noncopyable >("ITableWorkspace", no_init)
      .def("getColumnCount", &ITableWorkspace::columnCount)
      .def("getRowCount", &ITableWorkspace::rowCount)
      .def("getColumnNames",&ITableWorkspace::getColumnNames)
      .def("getInt", (get_integer_ptr)&ITableWorkspace::getRef<int>, return_value_policy<copy_non_const_reference>())
      .def("getDouble", (get_double_ptr)&ITableWorkspace::getRef<double>, return_value_policy<copy_non_const_reference>())
      .def("getString", (get_string_ptr)&ITableWorkspace::getRef<std::string>, return_value_policy<copy_non_const_reference>())
     ;
  }

  //-----------------------------------------------------------------------------------------------
  // WorkspaceGroup
  void export_workspacegroup()
  {
    // Pointer
    register_ptr_to_python<API::WorkspaceGroup_sptr>();
    
    class_< API::WorkspaceGroup, bases<API::Workspace>, 
      boost::noncopyable >("WorkspaceGroup", no_init)
      .def("__len__", &API::WorkspaceGroup::getNumberOfEntries)
      .def("size", &API::WorkspaceGroup::getNumberOfEntries)
      .def("getNames", &API::WorkspaceGroup::getNames)
      .def("add", &API::WorkspaceGroup::add)
      .def("remove", &API::WorkspaceGroup::remove)
      ;
  }

  //-----------------------------------------------------------------------------------------------
  void export_axis()
  {
    // Pointer
    register_ptr_to_python<API::Axis*>();

    // Class
    class_< API::Axis, boost::noncopyable >("MantidAxis", no_init)
      .def("title", (std::string & (Mantid::API::Axis::*)() ) &API::Axis::title, return_internal_reference<>())
      .def("isSpectra", & API::Axis::isSpectra)
      .def("isNumeric", & API::Axis::isNumeric)
      .def("isText", & API::Axis::isText)
      .def("label", & API::Axis::label)
      .def("getUnit", (const Mantid::Kernel::Unit_sptr & (Mantid::API::Axis::*)() const) &API::Axis::unit, return_value_policy<copy_const_reference>() )
      .def("setUnit", & API::Axis::setUnit)
      .def("getValue", & API::NumericAxis::getValue)
      ;

    // Numeric Axis subclass
    class_< API::NumericAxis, bases<API::Axis>, boost::noncopyable >("NumericAxis", no_init)
      .def("setValue", & API::NumericAxis::setValue)
      ;
    // Spectra Axis subclass
    class_< API::SpectraAxis, bases<API::Axis>, boost::noncopyable >("SpectraAxis", no_init)
      .def("spectraNumber", (const specid_t & (Mantid::API::SpectraAxis::*)(const size_t &) const) & API::SpectraAxis::spectraNo, return_value_policy<copy_const_reference>() )
      .def("setValue", & API::SpectraAxis::setValue)
      .def("populateOneToOne", & API::SpectraAxis::populateOneToOne)
      ;
    // Text Axis subclass
    class_< API::TextAxis, bases<API::Axis>, boost::noncopyable >("TextAxis", no_init)
      .def("setValue", & API::TextAxis::setLabel)
      .def("getValue", & API::TextAxis::label)
      ;
    // Axis creation helpers
    def("createNumericAxis", & Mantid::PythonAPI::createNumericAxis, return_internal_reference<>());
    def("createSpectraAxis", & Mantid::PythonAPI::createSpectraAxis, return_internal_reference<>());
    def("createTextAxis", & Mantid::PythonAPI::createTextAxis, return_internal_reference<>());
  }
  
  //-----------------------------------------------------------------------------------------------
  void export_sample()
  {
    //Pointer
    register_ptr_to_python<API::Sample*>();
    register_ptr_to_python<boost::shared_ptr<API::Sample> >();

    //Sample class
    class_< API::Sample, boost::noncopyable >("Sample", no_init)
      .def("getName", &API::Sample::getName, return_value_policy<copy_const_reference>())
      .def("getGeometryFlag", &API::Sample::getGeometryFlag)
      .def("getThickness", &API::Sample::getThickness)
      .def("getHeight", &API::Sample::getHeight)
      .def("getWidth", &API::Sample::getWidth)
      .def("__getitem__", &API::Sample::operator[], return_internal_reference<>())
      .def("size", &API::Sample::size)
      .def("getOrientedLattice", (const OrientedLattice & (Sample::*)() const)&API::Sample::getOrientedLattice, return_value_policy<copy_const_reference>())
      .def("hasOrientedLattice", &API::Sample::hasOrientedLattice)
     ;
  }

  void export_run()
  {
    //Pointer
    register_ptr_to_python<API::Run*>();

#define EXPORT_ADDPROPERTY(type, suffix)\
    .def("addProperty_"#suffix,(void (API::Run::*)(const std::string &, const type&, bool))&Run::addProperty)

#define EXPORT_ADDPROPERTY_UNITS(type, suffix)\
    .def("addProperty_"#suffix,(void (API::Run::*)(const std::string &, const type&, const std::string &, bool))&Run::addProperty)

    //Run class
    class_< API::Run,  boost::noncopyable >("Run", no_init)
      .def("getLogData", (Kernel::Property* (API::Run::*)(const std::string&) const)&Run::getLogData, 
        return_internal_reference<>())
      .def("getLogData", (const std::vector<Kernel::Property*> & (API::Run::*)() const)&Run::getLogData, 
        return_internal_reference<>())
      .def("getProtonCharge", &API::Run::getProtonCharge)
      .def("hasProperty", &API::Run::hasProperty)
      .def("getProperty", &API::Run::getProperty, return_value_policy<return_by_value>())
      .def("getProperties", &API::Run::getProperties, return_internal_reference<>())
//      .def("addProperty", (void (API::Run::*) (const std::string&, const std::string&, bool))&Run::addProperty)
      .def("getGoniometer", (const Goniometer & (API::Run::*)() const)&API::Run::getGoniometer, return_value_policy<copy_const_reference>())
      EXPORT_ADDPROPERTY(int, int)
      EXPORT_ADDPROPERTY(double, dbl)
      EXPORT_ADDPROPERTY(std::string, str)
      EXPORT_ADDPROPERTY_UNITS(int, int)
      EXPORT_ADDPROPERTY_UNITS(double, dbl)
      EXPORT_ADDPROPERTY_UNITS(std::string, str)
     ;

#undef EXPORT_ADDPROPERTY
#undef EXPORT_ADDPROPERTY_UNITS
  }

  void export_workspace_property()
  {
    class_< IWorkspaceProperty, boost::noncopyable>("IWorkspaceProperty", no_init)
      ;
    // Workspaces of various types
    class_< WorkspaceProperty<Workspace>, bases<Kernel::Property, API::IWorkspaceProperty>, boost::noncopyable>("WorkspaceProperty", no_init)
      ;
    class_< WorkspaceProperty<WorkspaceGroup>, bases<Kernel::Property, API::IWorkspaceProperty>, boost::noncopyable>("WorkspaceGroupProperty", no_init)
      ;
    class_< WorkspaceProperty<MatrixWorkspace>, bases<Kernel::Property,API::IWorkspaceProperty>, boost::noncopyable>("MatrixWorkspaceProperty", no_init)
      ;
    class_< WorkspaceProperty<ITableWorkspace>, bases<Kernel::Property,API::IWorkspaceProperty>, boost::noncopyable>("TableWorkspaceProperty", no_init)
      ;
    class_< WorkspaceProperty<IEventWorkspace>, bases<Kernel::Property,API::IWorkspaceProperty>, boost::noncopyable>("EventWorkspaceProperty", no_init)
      ;
    class_< WorkspaceProperty<IMDWorkspace>, bases<Kernel::Property,API::IWorkspaceProperty>, boost::noncopyable>("MDWorkspaceProperty", no_init)
      ;
    class_< WorkspaceProperty<IMDHistoWorkspace>, bases<Kernel::Property,API::IWorkspaceProperty>, boost::noncopyable>("MDHistoWorkspaceProperty", no_init)
      ;

  }
  
  void export_fileproperty()
  {
    //FileProperty enum
    enum_<FileProperty::FileAction>("FileAction")
      .value("Save", FileProperty::Save)
      .value("OptionalSave", FileProperty::OptionalSave)
      .value("Load", FileProperty::Load)
      .value("OptionalLoad", FileProperty::OptionalLoad)
      .value("Directory", FileProperty::Directory)
      .value("OptionalDirectory", FileProperty::OptionalDirectory)
      ;
  }

 void export_workspacefactory()
  {
    class_< PythonAPI::WorkspaceFactoryProxy, boost::noncopyable>("WorkspaceFactoryProxy", no_init)
      .def("createMatrixWorkspace", &PythonAPI::WorkspaceFactoryProxy::createMatrixWorkspace)
      .staticmethod("createMatrixWorkspace")
      .def("createMatrixWorkspaceFromTemplate",&PythonAPI::WorkspaceFactoryProxy::createMatrixWorkspaceFromTemplate)
      .staticmethod("createMatrixWorkspaceFromTemplate")
      ;
  }

  void export_apivalidators()
  {
    class_<Kernel::IValidator<API::MatrixWorkspace_sptr>, boost::noncopyable>("IValidator_matrix", no_init)
      ;

    // Unit checking
    class_<API::WorkspaceUnitValidator<API::MatrixWorkspace>, 
      bases<Kernel::IValidator<API::MatrixWorkspace_sptr> > >("WorkspaceUnitValidator", init<std::string>())
      ;
    // Histogram checking
    class_<API::HistogramValidator<API::MatrixWorkspace>, 
      bases<Kernel::IValidator<API::MatrixWorkspace_sptr> > >("HistogramValidator", init<bool>())
      ;
    // Raw count checker
    class_<API::RawCountValidator<API::MatrixWorkspace>, 
	   bases<Kernel::IValidator<API::MatrixWorkspace_sptr> > >("RawCountValidator", init<bool>())
      ;
    // Check for common bins
    class_<API::CommonBinsValidator<API::MatrixWorkspace>, 
	   bases<Kernel::IValidator<API::MatrixWorkspace_sptr> > >("CommonBinsValidator")
      ;
	
  }

  void export_workspace_history()
  {
    class_<API::WorkspaceHistory, boost::noncopyable>("WorkspaceHistory", no_init)
      .def("lastAlgorithm", &WorkspaceHistory::lastAlgorithm)
      .def(self_ns::str(self))
      ;
  }

  void export_file_finder()
  {
    class_<PythonAPI::FileFinderWrapper, boost::noncopyable>("FileFinder", no_init)
      .def("getFullPath", &PythonAPI::FileFinderWrapper::getFullPath)
      .staticmethod("getFullPath")
      .def("findRuns", &PythonAPI::FileFinderWrapper::findRuns)
      .staticmethod("findRuns")
      ;
  }

  void export_api_namespace()
  {
    export_frameworkmanager();
    export_ialgorithm();
    export_workspace();
    export_matrixworkspace();
    export_eventworkspace();
    export_IMDWorkspace();
    export_IMDHistoWorkspace();
    export_IMDEventWorkspace();
    export_IPeaksWorkspace();
    export_IPeak();
    export_BoxController();
    export_tableworkspace();
    export_workspacegroup();
    export_axis();
    export_sample();
    export_run();
    export_workspace_property();
    export_fileproperty();
    export_workspacefactory();
    export_apivalidators();
    export_workspace_history();
    export_file_finder();
    export_IMDDimension();
    export_ISpectrum();
    export_EventList();
  }
  //@endcond

} // namespace PythonAPI
} // namespace Mantid
