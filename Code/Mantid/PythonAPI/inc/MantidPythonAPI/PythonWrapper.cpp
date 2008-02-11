
// Boost Includes ==============================================================
#include <boost/python.hpp>
#include <boost/cstdint.hpp>

// Includes ====================================================================
#include <PythonInterface.h>

// Using =======================================================================
using namespace boost::python;

// Module ======================================================================
BOOST_PYTHON_MODULE(libMantidPythonAPI)
{
    class_< Mantid::PythonAPI::PythonInterface >("PythonInterface", init<  >())
        .def(init< const Mantid::PythonAPI::PythonInterface& >())
        .def("InitialiseFrameworkManager", &Mantid::PythonAPI::PythonInterface::InitialiseFrameworkManager)
        .def("CreateAlgorithm", &Mantid::PythonAPI::PythonInterface::CreateAlgorithm)
        .def("ExecuteAlgorithm", &Mantid::PythonAPI::PythonInterface::ExecuteAlgorithm)
        .def("LoadIsisRawFile", &Mantid::PythonAPI::PythonInterface::LoadIsisRawFile)
        .def("GetXData", &Mantid::PythonAPI::PythonInterface::GetXData)
    ;

}

