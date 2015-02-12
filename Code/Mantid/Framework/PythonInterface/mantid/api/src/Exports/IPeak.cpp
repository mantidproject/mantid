#include "MantidAPI/IPeak.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::API::IPeak;
using namespace boost::python;

namespace {
Mantid::Geometry::PeakShape_sptr getPeakShape(IPeak &peak) {
  // Use clone to make a copy of the PeakShape.
  return Mantid::Geometry::PeakShape_sptr(peak.getPeakShape().clone());
}
void setQLabFrame(IPeak &peak, Mantid::Kernel::V3D qLabFrame, double distance) {
  // Set the qlab frame
  return peak.setQLabFrame(qLabFrame, distance);
}
}

void export_IPeak()
{
  register_ptr_to_python<IPeak*>();

  class_<IPeak, boost::noncopyable>("IPeak", no_init)
    .def("getDetectorID", &IPeak::getDetectorID, "Get the ID of the detector at the center of the peak")
    .def("setDetectorID", &IPeak::setDetectorID, "Set the detector ID and look up and cache values related to it.")
    .def("getRunNumber", &IPeak::getRunNumber, "Return the run number this peak was measured at")
    .def("setRunNumber", &IPeak::setRunNumber, "Set the run number that measured this peak")
    .def("getMonitorCount", &IPeak::getMonitorCount, "Get the monitor count set for this peak")
    .def("setMonitorCount", &IPeak::setMonitorCount, "Set the monitor count for this peak")
    .def("getH", &IPeak::getH, "Get the H index of the peak")
    .def("getK", &IPeak::getK, "Get the K index of the peak")
    .def("getL", &IPeak::getL, "Get the L index of the peak")
    .def("getHKL", &IPeak::getHKL, "Get HKL as a V3D object")
    .def("setHKL", (void (IPeak::*)(double,double,double)) &IPeak::setHKL, "Set the HKL values of this peak")
    .def("setH", &IPeak::setH, "Get the H index of the peak")
    .def("setK", &IPeak::setK, "Get the K index of the peak")
    .def("setL", &IPeak::setL, "Get the L index of the peak")
    .def("getQLabFrame", &IPeak::getQLabFrame, "Return the Q change (of the lattice, k_i - k_f) for this peak.\n"
         "The Q is in the Lab frame: the goniometer rotation was NOT taken out.\n"
         "Note: There is no 2*pi factor used, so \\|Q| = 1/wavelength.")
    .def("findDetector", &IPeak::findDetector,
         "Using the instrument set in the peak, perform ray tracing to find the exact detector.")
    .def("getQSampleFrame", &IPeak::getQSampleFrame, "Return the Q change (of the lattice, k_i - k_f) for this peak."
         "The Q is in the Sample frame: the goniometer rotation WAS taken out. ")
    .def("setQLabFrame", (void (IPeak::*)(Mantid::Kernel::V3D))&IPeak::setQLabFrame)
    .def("setQLabFrame", setQLabFrame, "Set the peak using the peak's position in reciprocal space, in the lab frame.")
    .def("setQSampleFrame", &IPeak::setQSampleFrame, "Set the peak using the peak's position in reciprocal space, in the sample frame.")
    .def("setWavelength", &IPeak::setWavelength, "Set the incident wavelength of the neutron. Calculates the energy from this assuming elastic scattering.")
    .def("getWavelength", &IPeak::getWavelength, "Return the incident wavelength")
    .def("getScattering", &IPeak::getScattering, "Calculate the scattering angle of the peak")
    .def("getDSpacing", &IPeak::getDSpacing, "Calculate the d-spacing of the peak, in 1/Angstroms")
    .def("getTOF", &IPeak::getTOF, "Calculate the time of flight (in microseconds) of the neutrons for this peak")
    .def("getInitialEnergy", &IPeak::getInitialEnergy, "Get the initial (incident) neutron energy")
    .def("getFinalEnergy", &IPeak::getFinalEnergy, "Get the final neutron energy")
    .def("setInitialEnergy", &IPeak::setInitialEnergy, "Set the initial (incident) neutron energy")
    .def("setFinalEnergy", &IPeak::setFinalEnergy, "Set the final neutron energy")
    .def("getIntensity", &IPeak::getIntensity, "Return the integrated peak intensity")
    .def("getSigmaIntensity", &IPeak::getSigmaIntensity, "Return the error on the integrated peak intensity")
    .def("setIntensity", &IPeak::setIntensity, "Set the integrated peak intensity")
    .def("setSigmaIntensity", &IPeak::setSigmaIntensity, "Set the error on the integrated peak intensity")
    .def("getBinCount", &IPeak::getBinCount, "Return the # of counts in the bin at its peak")
    .def("setBinCount", &IPeak::setBinCount, "Set the # of counts in the bin at its peak")
    .def("getRow", &IPeak::getRow, "For RectangularDetectors only, returns the row (y) of the pixel of the detector.")
    .def("getCol", &IPeak::getCol, "For RectangularDetectors only, returns the column (x) of the pixel of the detector.")
    .def("getDetPos", &IPeak::getDetPos, "Return the detector position vector")
    .def("getL1", &IPeak::getL1, "Return the L1 flight path length (source to sample), in meters. ")
    .def("getL2", &IPeak::getL2, "Return the L2 flight path length (sample to detector), in meters.")
    .def("getPeakShape", getPeakShape, "Get the peak shape")
    ;
}

