#include <vector>

// Boost Includes ==============================================================
#include "boost/python.hpp"
#include "boost/python/suite/indexing/vector_indexing_suite.hpp"
#include "boost/cstdint.hpp"

// Includes ====================================================================
#include "MantidPythonAPI/PythonInterface.h"

// Using =======================================================================
using namespace boost::python;

// Module ======================================================================

typedef std::vector< std::string > string_vec;
typedef std::vector< double > double_vec;

#if _WIN32
BOOST_PYTHON_MODULE(MantidPythonAPI)
#else
BOOST_PYTHON_MODULE(libMantidPythonAPI)
#endif
{
	
	//Vectors
	class_< string_vec >( "StringVec" )
	.def( vector_indexing_suite< string_vec >() )
        ;
	
	class_< double_vec >( "DoubleVec" )
	.def( vector_indexing_suite< double_vec >() )
	;
	
	//Mantid stuff
	class_< Mantid::PythonAPI::PythonInterface >("PythonInterface", init<  >())
        .def(init< const Mantid::PythonAPI::PythonInterface& >())
        .def("InitialiseFrameworkManager", &Mantid::PythonAPI::PythonInterface::InitialiseFrameworkManager)
        .def("CreateAlgorithm", &Mantid::PythonAPI::PythonInterface::CreateAlgorithm)
        .def("ExecuteAlgorithm", &Mantid::PythonAPI::PythonInterface::ExecuteAlgorithm)
        .def("LoadIsisRawFile", &Mantid::PythonAPI::PythonInterface::LoadIsisRawFile)
        .def("GetXData", &Mantid::PythonAPI::PythonInterface::GetXData)
        .def("GetYData", &Mantid::PythonAPI::PythonInterface::GetYData)
	.def("GetAddressXData", &Mantid::PythonAPI::PythonInterface::GetAddressXData)
        .def("GetAddressYData", &Mantid::PythonAPI::PythonInterface::GetAddressYData)
	;

}

