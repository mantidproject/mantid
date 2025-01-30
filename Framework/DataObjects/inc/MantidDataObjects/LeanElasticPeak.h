// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/BasePeak.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/V3D.h"
#include <memory>
#include <optional>

namespace Mantid {

namespace DataObjects {

/** Structure describing a single-crystal peak. This is a version of
 * Peak that doesn't require the instrument and also assuming elastic
 * scattering. The peak is described only by the Q-sample
 * position. Optionally if the wavelength and goniometer is provided
 * other properties can be calculated.
 *
 */
class MANTID_DATAOBJECTS_DLL LeanElasticPeak : public BasePeak {
public:
  LeanElasticPeak();
  LeanElasticPeak(const Mantid::Kernel::V3D &QSampleFrame);
  LeanElasticPeak(const Mantid::Kernel::V3D &QSampleFrame, const Mantid::Kernel::Matrix<double> &goniometer,
                  std::optional<std::shared_ptr<const Geometry::ReferenceFrame>> refFrame = std::nullopt);
  LeanElasticPeak(const Mantid::Kernel::V3D &QSampleFrame, double wavelength);

  /// Copy constructor
  LeanElasticPeak(const LeanElasticPeak &other);

  // MSVC 2015/17 can build with noexcept = default however
  // intellisense still incorrectly reports this as an error despite compiling.
  // https://connect.microsoft.com/VisualStudio/feedback/details/1795240/visual-c-2015-default-move-constructor-and-noexcept-keyword-bug
  // For that reason we still use the supplied default which should be noexcept
  // once the above is fixed we can remove this workaround
#if defined(_MSC_VER) && _MSC_VER <= 1910
  LeanElasticPeak(LeanElasticPeak &&) = default;
  LeanElasticPeak &operator=(LeanElasticPeak &&) = default;
#else
  LeanElasticPeak(LeanElasticPeak &&) noexcept = default;
  LeanElasticPeak &operator=(LeanElasticPeak &&) noexcept = default;
#endif

  // Construct a peak from a reference to the interface
  explicit LeanElasticPeak(const Geometry::IPeak &ipeak);

  std::shared_ptr<const Geometry::ReferenceFrame> getReferenceFrame() const override;

  int getDetectorID() const override;

  int getCol() const override;
  int getRow() const override;

  Mantid::Kernel::V3D getQLabFrame() const final; // Marked final since used in constructor
  Mantid::Kernel::V3D getQSampleFrame() const override;

  void setQSampleFrame(const Mantid::Kernel::V3D &QSampleFrame, std::optional<double> = std::nullopt) override;
  void setQSampleFrame(const Mantid::Kernel::V3D &QSampleFrame, const Mantid::Kernel::Matrix<double> &goniometer);
  void setQLabFrame(const Mantid::Kernel::V3D &qLab, std::optional<double> = std::nullopt) override;

  void setWavelength(double wavelength) override;
  double getWavelength() const override;
  double getScattering() const override;
  double getAzimuthal() const override;
  double getDSpacing() const override;
  double getTOF() const override;

  double getInitialEnergy() const override;
  double getFinalEnergy() const override;
  double getEnergyTransfer() const override;
  void setInitialEnergy(double m_initialEnergy) override;
  void setFinalEnergy(double m_finalEnergy) override;

  double getL1() const override;
  double getL2() const override;

  Mantid::Kernel::V3D getDetectorDirectionSampleFrame() const override;
  Mantid::Kernel::V3D getSourceDirectionSampleFrame() const override;

  /// Assignment
  LeanElasticPeak &operator=(const LeanElasticPeak &other);

private:
  void setReferenceFrame(std::shared_ptr<const Geometry::ReferenceFrame> frame);

  /// Q_sample vector
  Mantid::Kernel::V3D m_Qsample;

  /// Wavelength of neutrons at the peak
  double m_wavelength;

  std::shared_ptr<const Geometry::ReferenceFrame> m_refFrame;

  /// Static logger
  static Mantid::Kernel::Logger g_log;
};

} // namespace DataObjects
} // namespace Mantid
