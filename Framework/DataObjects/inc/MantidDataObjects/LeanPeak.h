// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/BasePeak.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"
#include <boost/optional.hpp>
#include <memory>

namespace Mantid {

namespace DataObjects {

/** Structure describing a single-crystal peak. This is a version of
 * Peak that doesn't require the instrument. The peak is described
 * only by the Q-sample position. Optionally if the wavelength and
 * goniometer is provided other properties can be calculated.
 *
 */
class DLLExport LeanPeak : public BasePeak {
public:
  /// Allow PeakColumn class to directly access members.
  friend class PeakColumn;

  LeanPeak();
  LeanPeak(const Mantid::Kernel::V3D &QSampleFrame);
  LeanPeak(const Mantid::Kernel::V3D &QSampleFrame,
           const Mantid::Kernel::Matrix<double> &goniometer);
  LeanPeak(const Mantid::Kernel::V3D &QSampleFrame, double wavelength);
  LeanPeak(const Mantid::Kernel::V3D &QSampleFrame,
           const Mantid::Kernel::Matrix<double> &goniometer, double wavelength);

  /// Copy constructor
  LeanPeak(const LeanPeak &other);

  // MSVC 2015/17 can build with noexcept = default however
  // intellisense still incorrectly reports this as an error despite compiling.
  // https://connect.microsoft.com/VisualStudio/feedback/details/1795240/visual-c-2015-default-move-constructor-and-noexcept-keyword-bug
  // For that reason we still use the supplied default which should be noexcept
  // once the above is fixed we can remove this workaround
#if defined(_MSC_VER) && _MSC_VER <= 1910
  LeanPeak(LeanPeak &&) = default;
  LeanPeak &operator=(LeanPeak &&) = default;
#elif ((__GNUC__ < 4) || (__GNUC__ == 4 && __GNUC_MINOR__ <= 8))
  // The noexcept default declaration was fixed in GCC 4.9.0
  // so for versions 4.8.x and below use default only
  // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53903
  LeanPeak(LeanPeak &&) = default;
  LeanPeak &operator=(LeanPeak &&) = default;
#else
  LeanPeak(LeanPeak &&) noexcept = default;
  LeanPeak &operator=(LeanPeak &&) noexcept = default;
#endif

  // Construct a peak from a reference to the interface
  explicit LeanPeak(const Geometry::IPeak &ipeak);

  void setDetectorID(int id) override;
  int getDetectorID() const override;

  void setInstrument(const Geometry::Instrument_const_sptr &inst) override;
  Geometry::IDetector_const_sptr getDetector() const override;
  Geometry::Instrument_const_sptr getInstrument() const override;

  bool findDetector() override;
  bool findDetector(const Geometry::InstrumentRayTracer &tracer) override;

  void setSamplePos(double samX, double samY, double samZ) override;
  void setSamplePos(const Mantid::Kernel::V3D &XYZ) override;

  Mantid::Kernel::V3D getQLabFrame() const override;
  Mantid::Kernel::V3D getQSampleFrame() const override;
  Mantid::Kernel::V3D getDetectorPosition() const override;
  Mantid::Kernel::V3D getDetectorPositionNoCheck() const override;

  void setQSampleFrame(
      const Mantid::Kernel::V3D &QSampleFrame,
      boost::optional<double> detectorDistance = boost::none) override;
  void
  setQLabFrame(const Mantid::Kernel::V3D &qLab,
               boost::optional<double> detectorDistance = boost::none) override;

  double getScattering() const override;
  double getAzimuthal() const override;
  double getDSpacing() const override;
  double getTOF() const override;

  virtual Mantid::Kernel::V3D getDetPos() const override;
  virtual Mantid::Kernel::V3D getSamplePos() const override;
  double getL1() const override;
  double getL2() const override;

  /// Assignment
  LeanPeak &operator=(const LeanPeak &other);

private:
  /// Q_sample vector
  Mantid::Kernel::V3D m_Qsample;

  /// Static logger
  static Mantid::Kernel::Logger g_log;
};

} // namespace DataObjects
} // namespace Mantid
