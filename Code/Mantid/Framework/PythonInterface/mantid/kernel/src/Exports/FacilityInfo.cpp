#include "MantidKernel/FacilityInfo.h"
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Kernel::FacilityInfo;
using Mantid::Kernel::InstrumentInfo;
using namespace boost::python;

void export_FacilityInfo()
{

  register_ptr_to_python<FacilityInfo*>();

  class_<FacilityInfo>("FacilityInfo", no_init)

    .def("name", &FacilityInfo::name, return_value_policy<copy_const_reference>(),
         "Returns name of the facility as definined in the Facilities.xml file")

    .def("__str__", &FacilityInfo::name, return_value_policy<copy_const_reference>(),
         "Returns name of the facility as definined in the Facilities.xml file")

    .def("zeroPadding", &FacilityInfo::zeroPadding,
         "Returns default zero padding for this facility")

    .def("delimiter", &FacilityInfo::delimiter, return_value_policy<copy_const_reference>(),
         "Returns the delimiter between the instrument name and the run number.")

    .def("extensions", &FacilityInfo::extensions,
         "Returns the list of file extensions that are considered as instrument data files.")

    .def("preferredExtension",&FacilityInfo::preferredExtension, return_value_policy<copy_const_reference>(),
         "Returns the extension that is preferred for this facility")

    .def("getSoapEndPoint", &FacilityInfo::getSoapEndPoint, return_value_policy<copy_const_reference>(),
         "Returns the name of the SOAP end point")

    .def("archiveSearch", &FacilityInfo::archiveSearch, return_value_policy<copy_const_reference>(),
         "Return the archive search interface names")

    .def("instruments", (const std::vector<InstrumentInfo> & (FacilityInfo::*)()const)&FacilityInfo::instruments,
         return_value_policy<copy_const_reference>(),
         "Returns a list of instruments of this facility as defined in the Facilities.xml file")

    .def("instruments", (std::vector<InstrumentInfo> (FacilityInfo::*)(const std::string&)const)&FacilityInfo::instruments,
         "Returns a list of instruments of given technique")

    .def("instrument", &FacilityInfo::instrument, return_value_policy<copy_const_reference>(),
         "Returns the instrument with the given name")

    .def("catalogName", &FacilityInfo::catalogName, return_value_policy<copy_const_reference>(),
         "Returns the catalog name used at this facility")

    .def("liveListener", &FacilityInfo::liveListener, return_value_policy<copy_const_reference>(),
         "Returns the name of the default live listener")

    .def("computeResources", &FacilityInfo::computeResources,
         "Returns a vector of the available compute resources")
   ;
}

