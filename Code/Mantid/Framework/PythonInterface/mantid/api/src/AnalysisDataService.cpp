#include "MantidAPI/AnalysisDataService.h"
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/reference_existing_object.hpp>

using Mantid::API::AnalysisDataServiceImpl;
using Mantid::API::AnalysisDataService;
using boost::python::class_;
using boost::python::def;
using boost::python::no_init;
using boost::python::return_value_policy;
using boost::python::reference_existing_object;

namespace
{
  ///@cond
  //----------------------------------------------------------------------------
  // Factory function to return the singleton reference
  AnalysisDataServiceImpl & getAnalysisDataService()
  {
    return AnalysisDataService::Instance();
  }
  ///@endcond

}

void export_AnalysisDataService()
{
  class_<AnalysisDataServiceImpl,boost::noncopyable>("AnalysisDataService", no_init)
    .def("retrieve", &AnalysisDataServiceImpl::retrieve, "Retrieve the named object")
    .def("remove", &AnalysisDataServiceImpl::remove, "Remove a named object")
    .def("size", &AnalysisDataServiceImpl::size, "Returns the number of objects within the service")
    // Make it act like a dictionary
    .def("__len__", &AnalysisDataServiceImpl::size)
    .def("__getitem__", &AnalysisDataServiceImpl::retrieve)
    .def("__contains__", &AnalysisDataServiceImpl::doesExist)
    .def("__delitem__", &AnalysisDataServiceImpl::remove)
    ;

  // Factory function
  def("get_analysis_data_service", &getAnalysisDataService, return_value_policy<reference_existing_object>(),
      "Return a reference to the ADS singleton");

}

