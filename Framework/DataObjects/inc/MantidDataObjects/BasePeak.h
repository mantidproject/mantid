// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/DllConfig.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidGeometry/Crystal/PeakShape.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/V3D.h"
#include <memory>
#include <optional>

namespace Mantid {

namespace Geometry {
class InstrumentRayTracer;
}

namespace DataObjects {

/** Structure describing a single-crystal peak. This is a partial
 * implementation of IPeak and should contain everything that is common
 * to Peak and LeanPeak.
 *
 */
class MANTID_DATAOBJECTS_DLL BasePeak : public Geometry::IPeak {
public:
  BasePeak();
  BasePeak(const Mantid::Kernel::Matrix<double> &goniometer);
  /// Copy constructor
  BasePeak(const BasePeak &other);

  // MSVC 2015/17 can build with noexcept = default however
  // intellisense still incorrectly reports this as an error despite compiling.
  // https://connect.microsoft.com/VisualStudio/feedback/details/1795240/visual-c-2015-default-move-constructor-and-noexcept-keyw
  // ord-bug For that reason we still use the supplied default which should be
  // noexcept once the above is fixed we can remove this workaround
#if defined(_MSC_VER) && _MSC_VER <= 1910
  BasePeak(BasePeak &&) = default;
  BasePeak &operator=(BasePeak &&) = default;
#else
  BasePeak(BasePeak &&) noexcept = default;
  BasePeak &operator=(BasePeak &&) noexcept = default;
#endif

  // Construct a peak from a reference to the interface
  explicit BasePeak(const Geometry::IPeak &ipeak);

  int getRunNumber() const override;
  void setRunNumber(int m_runNumber) override;

  double getMonitorCount() const override;
  void setMonitorCount(double m_monitorCount) override;

  double getH() const override;
  double getK() const override;
  double getL() const override;
  Mantid::Kernel::V3D getHKL() const override;
  bool isIndexed() const override;
  Mantid::Kernel::V3D getIntHKL() const override;
  Mantid::Kernel::V3D getIntMNP() const override;
  void setH(double m_H) override;
  void setK(double m_K) override;
  void setL(double m_L) override;
  void setHKL(double H, double K, double L) override;
  void setHKL(const Mantid::Kernel::V3D &HKL) override;
  void setIntHKL(const Kernel::V3D &HKL) override;
  void setIntMNP(const Mantid::Kernel::V3D &MNP) override;

  Mantid::Kernel::V3D getSamplePos() const override;
  void setSamplePos(double samX, double samY, double samZ) override;
  void setSamplePos(const Mantid::Kernel::V3D &XYZ) override;

  double getIntensity() const override;
  double getSigmaIntensity() const override;
  double getIntensityOverSigma() const override;

  void setIntensity(double m_intensity) override;
  void setSigmaIntensity(double m_sigmaIntensity) override;

  double getBinCount() const override;
  void setBinCount(double m_binCount) override;

  Mantid::Kernel::Matrix<double> getGoniometerMatrix() const override;
  Mantid::Kernel::Matrix<double> getInverseGoniometerMatrix() const;
  void setGoniometerMatrix(const Mantid::Kernel::Matrix<double> &goniometerMatrix) override;

  void setPeakNumber(int m_peakNumber) override;
  int getPeakNumber() const override;

  virtual double getValueByColName(std::string colName) const;

  /// Get the peak shape.
  const Mantid::Geometry::PeakShape &getPeakShape() const override;

  /// Set the PeakShape
  void setPeakShape(Mantid::Geometry::PeakShape *shape) override;

  /// Set the PeakShape
  void setPeakShape(Mantid::Geometry::PeakShape_const_sptr shape) override;

  /// Assignment
  BasePeak &operator=(const BasePeak &other);

  void setAbsorptionWeightedPathLength(double pathLength) override;
  double getAbsorptionWeightedPathLength() const override;

protected:
  double calculateWavelengthFromQLab(const Mantid::Kernel::V3D &qLab);

  // ki-kf for Inelastic convention; kf-ki for Crystallography convention
  std::string m_convention;

  /// Cached sample position
  Mantid::Kernel::V3D m_samplePos;

private:
  /// H of the peak
  double m_H;

  /// K of the peak
  double m_K;

  /// L of the peak
  double m_L;

  /// Integrated peak intensity
  double m_intensity;

  /// Error (sigma) on peak intensity
  double m_sigmaIntensity;

  /// Count in the bin at the peak
  double m_binCount;

  /// absorption weighted path length (aka t bar)
  double m_absorptionWeightedPathLength;

  /// Orientation matrix of the goniometer angles.
  Mantid::Kernel::Matrix<double> m_GoniometerMatrix;

  /// Inverse of the goniometer rotation matrix; used to go from Q in lab frame
  /// to Q in sample frame
  Mantid::Kernel::Matrix<double> m_InverseGoniometerMatrix;

  /// Originating run number for this peak
  int m_runNumber;

  /// Integrated monitor count over TOF range for this run
  double m_monitorCount;

  int m_peakNumber;
  Mantid::Kernel::V3D m_intHKL;
  Mantid::Kernel::V3D m_intMNP;

  /// Peak shape
  Mantid::Geometry::PeakShape_const_sptr m_peakShape;

  /// Static logger
  static Mantid::Kernel::Logger g_log;
};

} // namespace DataObjects
} // namespace Mantid
