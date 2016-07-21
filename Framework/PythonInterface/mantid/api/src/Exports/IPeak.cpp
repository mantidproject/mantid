#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include "MantidPythonInterface/kernel/Converters/PyObjectToMatrix.h"
#include <boost/optional.hpp>

using Mantid::Geometry::IPeak;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(IPeak)

namespace {
using namespace Mantid::PythonInterface;
Mantid::Geometry::PeakShape_sptr getPeakShape(IPeak &peak) {
  // Use clone to make a copy of the PeakShape.
  return Mantid::Geometry::PeakShape_sptr(peak.getPeakShape().clone());
}
void setQLabFrame1(IPeak &peak, Mantid::Kernel::V3D qLabFrame) {
  // Set the q lab frame. No explicit detector distance.
  return peak.setQLabFrame(qLabFrame, boost::optional<double>());
}
void setQLabFrame2(IPeak &peak, Mantid::Kernel::V3D qLabFrame,
                   double distance) {
  // Set the q lab frame. Detector distance specified.
  return peak.setQLabFrame(qLabFrame, distance);
}

void setQSampleFrame1(IPeak &peak, Mantid::Kernel::V3D qSampleFrame) {
  // Set the qsample frame. No explicit detector distance.
  return peak.setQSampleFrame(qSampleFrame, boost::optional<double>());
}

void setQSampleFrame2(IPeak &peak, Mantid::Kernel::V3D qSampleFrame,
                      double distance) {
  // Set the qsample frame. Detector distance specified.
  return peak.setQSampleFrame(qSampleFrame, distance);
}

void setGoniometerMatrix(IPeak &self, const object &data) {
  self.setGoniometerMatrix(Converters::PyObjectToMatrix(data)());
}
}

void export_IPeak() {
  register_ptr_to_python<IPeak *>();

  class_<IPeak, boost::noncopyable>("IPeak", no_init)
      .def("getDetectorID", &IPeak::getDetectorID, arg("self"),
           "Get the ID of the detector at the center of the peak")
      .def("setDetectorID", &IPeak::setDetectorID, (arg("self"), arg("det_id")),
           "Set the detector ID and look up and cache values related to it.")
      .def("getRunNumber", &IPeak::getRunNumber, arg("self"),
           "Return the run number this peak was measured at")
      .def("setRunNumber", &IPeak::setRunNumber,
           (arg("self"), arg("run_number")),
           "Set the run number that measured this peak")
      .def("getMonitorCount", &IPeak::getMonitorCount, arg("self"),
           "Get the monitor count set for this peak")
      .def("setMonitorCount", &IPeak::setMonitorCount,
           (arg("self"), arg("monitor_count")),
           "Set the monitor count for this peak")
      .def("getH", &IPeak::getH, arg("self"), "Get the H index of the peak")
      .def("getK", &IPeak::getK, arg("self"), "Get the K index of the peak")
      .def("getL", &IPeak::getL, arg("self"), "Get the L index of the peak")
      .def("getHKL", &IPeak::getHKL, arg("self"), "Get HKL as a V3D object")
      .def("setHKL", (void (IPeak::*)(double, double, double)) & IPeak::setHKL,
           (arg("self"), arg("h"), arg("k"), arg("l")),
           "Set the HKL values of this peak")
      .def("setH", &IPeak::setH, (arg("self"), arg("h")),
           "Get the H index of the peak")
      .def("setK", &IPeak::setK, (arg("self"), arg("k")),
           "Get the K index of the peak")
      .def("setL", &IPeak::setL, (arg("self"), arg("l")),
           "Get the L index of the peak")
      .def("getQLabFrame", &IPeak::getQLabFrame, arg("self"),
           "Return the Q change (of the lattice, k_i - k_f) for this peak.\n"
           "The Q is in the Lab frame: the goniometer rotation was NOT taken "
           "out.\n"
           "Note: There is no 2*pi factor used, so \\|Q| = 1/wavelength.")
      .def("findDetector", &IPeak::findDetector, arg("self"),
           "Using the instrument set in the peak, perform ray tracing to find "
           "the exact detector.")
      .def("getQSampleFrame", &IPeak::getQSampleFrame, arg("self"),
           "Return the Q change (of the lattice, k_i - k_f) for this peak."
           "The Q is in the Sample frame: the goniometer rotation WAS taken "
           "out. ")
      .def("setQLabFrame", setQLabFrame1, (arg("self"), arg("qlab_frame")),
           "Set the peak using the peak's "
           "position in reciprocal space, in "
           "the lab frame.")
      .def("setQLabFrame", setQLabFrame2,
           (arg("self"), arg("qlab_frame"), arg("distance")),
           "Set the peak using the peak's position in reciprocal space, in the "
           "lab frame. Detector distance explicitly supplied.") // two argument
                                                                // overload
      .def("setQSampleFrame", setQSampleFrame1,
           (arg("self"), arg("qsample_frame")), "Set the peak using the peak's "
                                                "position in reciprocal space, "
                                                "in the sample frame.")
      .def("setQSampleFrame", setQSampleFrame2,
           (arg("self"), arg("qsample_frame"), arg("distance")),
           "Set the peak using the peak's position in reciprocal space, in the "
           "sample frame. Detector distance explicitly supplied.")
      .def("setWavelength", &IPeak::setWavelength,
           (arg("self"), arg("wave_length")),
           "Set the incident wavelength of the neutron. Calculates the energy "
           "from this assuming elastic scattering.")
      .def("getWavelength", &IPeak::getWavelength, arg("self"),
           "Return the incident wavelength")
      .def("getScattering", &IPeak::getScattering, arg("self"),
           "Calculate the scattering angle of the peak")
      .def("getDSpacing", &IPeak::getDSpacing, arg("self"),
           "Calculate the d-spacing of the peak, in 1/Angstroms")
      .def("getTOF", &IPeak::getTOF, arg("self"),
           "Calculate the time of flight (in "
           "microseconds) of the neutrons for this "
           "peak")
      .def("getInitialEnergy", &IPeak::getInitialEnergy, arg("self"),
           "Get the initial (incident) neutron energy")
      .def("getFinalEnergy", &IPeak::getFinalEnergy, arg("self"),
           "Get the final neutron energy")
      .def("setInitialEnergy", &IPeak::setInitialEnergy,
           (arg("self"), arg("initial_energy")),
           "Set the initial (incident) neutron energy")
      .def("setFinalEnergy", &IPeak::setFinalEnergy,
           (arg("self"), arg("final_energy")), "Set the final neutron energy")
      .def("getIntensity", &IPeak::getIntensity, arg("self"),
           "Return the integrated peak intensity")
      .def("getSigmaIntensity", &IPeak::getSigmaIntensity, arg("self"),
           "Return the error on the integrated peak intensity")
      .def("setIntensity", &IPeak::setIntensity,
           (arg("self"), arg("intensity")), "Set the integrated peak intensity")
      .def("setSigmaIntensity", &IPeak::setSigmaIntensity,
           (arg("self"), arg("sigma_intensity")),
           "Set the error on the integrated peak intensity")
      .def("getBinCount", &IPeak::getBinCount, arg("self"),
           "Return the # of counts in the bin at its peak")
      .def("setBinCount", &IPeak::setBinCount, (arg("self"), arg("bin_count")),
           "Set the # of counts in the bin at its peak")
      .def("setGoniometerMatrix", &setGoniometerMatrix,
           (arg("self"), arg("goniometerMatrix")),
           "Set the goniometer of the peak")
      .def("getRow", &IPeak::getRow, arg("self"),
           "For RectangularDetectors only, returns "
           "the row (y) of the pixel of the "
           "detector.")
      .def("getCol", &IPeak::getCol, arg("self"),
           "For RectangularDetectors only, returns "
           "the column (x) of the pixel of the "
           "detector.")
      .def("getDetPos", &IPeak::getDetPos, arg("self"),
           "Return the detector position vector")
      .def("getL1", &IPeak::getL1, arg("self"),
           "Return the L1 flight path length (source to sample), in meters. ")
      .def("getL2", &IPeak::getL2, arg("self"),
           "Return the L2 flight path length (sample to detector), in meters.")
      .def("getPeakShape", getPeakShape, arg("self"), "Get the peak shape");
}
