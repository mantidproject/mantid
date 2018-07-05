#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidPythonInterface/kernel/Converters/CloneToNumpy.h"
#include "MantidPythonInterface/kernel/Converters/PyObjectToMatrix.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidPythonInterface/kernel/Policies/MatrixToNumpy.h"
#include <boost/optional.hpp>
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Geometry::IPeak;
using namespace Mantid::PythonInterface;
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
  return peak.setQLabFrame(qLabFrame, boost::none);
}
void setQLabFrame2(IPeak &peak, Mantid::Kernel::V3D qLabFrame,
                   double distance) {
  // Set the q lab frame. Detector distance specified.
  return peak.setQLabFrame(qLabFrame, distance);
}

void setQSampleFrame1(IPeak &peak, Mantid::Kernel::V3D qSampleFrame) {
  // Set the qsample frame. No explicit detector distance.
  return peak.setQSampleFrame(qSampleFrame, boost::none);
}

void setQSampleFrame2(IPeak &peak, Mantid::Kernel::V3D qSampleFrame,
                      double distance) {
  // Set the qsample frame. Detector distance specified.
  return peak.setQSampleFrame(qSampleFrame, distance);
}

void setGoniometerMatrix(IPeak &self, const object &data) {
  self.setGoniometerMatrix(Converters::PyObjectToMatrix(data)());
}

} // namespace

void export_IPeak() {
  // return_value_policy for read-only numpy array
  using return_copy_to_numpy = return_value_policy<Policies::MatrixToNumpy>;

  register_ptr_to_python<IPeak *>();

  class_<IPeak, boost::noncopyable>("IPeak", no_init)
      .def("getDetectorID", &IPeak::getDetectorID, arg("self"),
           "Get the ID of the :class:`~mantid.geometry.Detector` at the center "
           "of the peak")
      .def("setDetectorID", &IPeak::setDetectorID, (arg("self"), arg("det_id")),
           "Set the :class:`~mantid.geometry.Detector` ID and look up and "
           "cache values related to it.")
      .def("getRunNumber", &IPeak::getRunNumber, arg("self"),
           "Return the run number this peak was measured at")
      .def("getPeakNumber", &IPeak::getPeakNumber, arg("self"),
           "Return the peak number for this peak")
      .def("getBankName", &IPeak::getBankName, arg("self"),
           "Return the bank name for this peak")
      .def("setRunNumber", &IPeak::setRunNumber,
           (arg("self"), arg("run_number")),
           "Set the run number that measured this peak")
      .def("setPeakNumber", &IPeak::setPeakNumber,
           (arg("self"), arg("peak_number")),
           "Set the peak number for this peak")
      .def("getMonitorCount", &IPeak::getMonitorCount, arg("self"),
           "Get the monitor count set for this peak")
      .def("setMonitorCount", &IPeak::setMonitorCount,
           (arg("self"), arg("monitor_count")),
           "Set the monitor count for this peak")
      .def("getH", &IPeak::getH, arg("self"), "Get the H index of the peak")
      .def("getK", &IPeak::getK, arg("self"), "Get the K index of the peak")
      .def("getL", &IPeak::getL, arg("self"), "Get the L index of the peak")
      .def("getHKL", &IPeak::getHKL, arg("self"),
           "Get HKL as a :class:`~mantid.kernel.V3D` object")
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
           "The Q is in the Lab frame: the "
           ":class:`~mantid.geometry.Goniometer` rotation was NOT taken "
           "out.\n"
           "Note: There is no 2*pi factor used, so \\|Q| = 1/wavelength.")
      .def("findDetector", (bool (IPeak::*)()) & IPeak::findDetector,
           arg("self"),
           "Using the :class:`~mantid.geometry.Instrument` set in the peak, "
           "perform ray tracing to find "
           "the exact :class:`~mantid.geometry.Detector`.")
      .def("getQSampleFrame", &IPeak::getQSampleFrame, arg("self"),
           "Return the Q change (of the lattice, k_i - k_f) for this peak."
           "The Q is in the Sample frame: the "
           ":class:`~mantid.geometry.Goniometer` rotation WAS taken "
           "out. ")
      .def("setQLabFrame", setQLabFrame1, (arg("self"), arg("qlab_frame")),
           "Set the peak using the peak's "
           "position in reciprocal space, in "
           "the lab frame.")
      .def("setQLabFrame", setQLabFrame2,
           (arg("self"), arg("qlab_frame"), arg("distance")),
           "Set the peak using the peak's position in reciprocal space, in the "
           "lab frame. :class:`~mantid.geometry.Detector` distance explicitly "
           "supplied.") // two argument
                        // overload
      .def("setQSampleFrame", setQSampleFrame1,
           (arg("self"), arg("qsample_frame")),
           "Set the peak using the peak's "
           "position in reciprocal space, "
           "in the sample frame.")
      .def("setQSampleFrame", setQSampleFrame2,
           (arg("self"), arg("qsample_frame"), arg("distance")),
           "Set the peak using the peak's position in reciprocal space, in the "
           "sample frame. :class:`~mantid.geometry.Detector` distance "
           "explicitly supplied.")
      .def("setWavelength", &IPeak::setWavelength,
           (arg("self"), arg("wave_length")),
           "Set the incident wavelength of the neutron. Calculates the energy "
           "from this assuming elastic scattering.")
      .def("getWavelength", &IPeak::getWavelength, arg("self"),
           "Return the incident wavelength")
      .def("getScattering", &IPeak::getScattering, arg("self"),
           "Calculate the scattering angle of the peak")
      .def("getAzimuthal", &IPeak::getAzimuthal, arg("self"),
           "Calculate the azimuthal angle of the peak")
      .def("getDSpacing", &IPeak::getDSpacing, arg("self"),
           "Calculate the d-spacing of the peak, in 1/Angstroms")
      .def("getTOF", &IPeak::getTOF, arg("self"),
           "Calculate the time of flight (in "
           "microseconds) of the neutrons for this "
           "peak")
      .def("getInitialEnergy", &IPeak::getInitialEnergy, arg("self"),
           "Get the initial (incident) neutron energy in meV.")
      .def("getFinalEnergy", &IPeak::getFinalEnergy, arg("self"),
           "Get the final neutron energy in meV.")
      .def("getEnergyTransfer", &IPeak::getEnergyTransfer, arg("self"),
           "Get the initial neutron energy minus the final neutron energy in "
           "meV."
           "\n\n.. versionadded:: 3.12.0")
      .def("setInitialEnergy", &IPeak::setInitialEnergy,
           (arg("self"), arg("initial_energy")),
           "Set the initial (incident) neutron energy in meV.")
      .def("setFinalEnergy", &IPeak::setFinalEnergy,
           (arg("self"), arg("final_energy")),
           "Set the final neutron energy in meV.")
      .def("getIntensity", &IPeak::getIntensity, arg("self"),
           "Return the integrated peak intensity")
      .def("getSigmaIntensity", &IPeak::getSigmaIntensity, arg("self"),
           "Return the error on the integrated peak intensity")
      .def("getIntensityOverSigma", &IPeak::getIntensityOverSigma, arg("self"),
           "Return the error on the integrated peak intensity divided by the "
           "error in intensity.\n\n.. versionadded:: 3.12.0")
      .def("setIntensity", &IPeak::setIntensity,
           (arg("self"), arg("intensity")), "Set the integrated peak intensity")
      .def("setSigmaIntensity", &IPeak::setSigmaIntensity,
           (arg("self"), arg("sigma_intensity")),
           "Set the error on the integrated peak intensity")
      .def("getBinCount", &IPeak::getBinCount, arg("self"),
           "Return the # of counts in the bin at its peak")
      .def("setBinCount", &IPeak::setBinCount, (arg("self"), arg("bin_count")),
           "Set the # of counts in the bin at its peak")
      .def("getGoniometerMatrix", &IPeak::getGoniometerMatrix, arg("self"),
           return_copy_to_numpy(),
           "Get the :class:`~mantid.geometry.Goniometer` rotation matrix of "
           "this peak."
           "\n\n.. versionadded:: 3.12.0")
      .def("setGoniometerMatrix", &setGoniometerMatrix,
           (arg("self"), arg("goniometerMatrix")),
           "Set the :class:`~mantid.geometry.Goniometer` rotation matrix of "
           "this peak.")
      .def("getRow", &IPeak::getRow, arg("self"),
           "For :class:`~mantid.geometry.RectangularDetector` s only, returns "
           "the row (y) of the pixel of the "
           "detector.")
      .def("getCol", &IPeak::getCol, arg("self"),
           "For :class:`~mantid.geometry.RectangularDetector` s only, returns "
           "the column (x) of the pixel of the "
           ":class:`~mantid.geometry.Detector`.")
      .def("getDetPos", &IPeak::getDetPos, arg("self"),
           "Return the :class:`~mantid.geometry.Detector` position vector")
      .def("getL1", &IPeak::getL1, arg("self"),
           "Return the L1 flight path length (source to "
           ":class:`~mantid.api.Sample`), in meters. ")
      .def("getL2", &IPeak::getL2, arg("self"),
           "Return the L2 flight path length (:class:`~mantid.api.Sample` to "
           ":class:`~mantid.geometry.Detector`), in meters.")
      .def("getPeakShape", getPeakShape, arg("self"), "Get the peak shape");
}
