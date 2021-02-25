// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidGeometry/Crystal/PeakShape.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"
#include <boost/optional.hpp>
#include <memory>

namespace Mantid {

namespace Geometry {
class InstrumentRayTracer;
}

namespace DataObjects {

/** Structure describing a single-crystal peak
 *
 * @author Janik Zikovsky
 * @date 2011-04-15 13:24:07.963491
 */
class DLLExport LeanPeak : public Geometry::IPeak {
public:
  /// Allow PeakColumn class to directly access members.
  friend class PeakColumn;

  LeanPeak();
  LeanPeak(const Mantid::Kernel::V3D &QLabFrame,
           const Mantid::Kernel::Matrix<double> &goniometer);
  LeanPeak(const Mantid::Kernel::V3D &QSampleFrame);

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

  void setDetectorID(int id) override;
  int getDetectorID() const override;

  void setInstrument(const Geometry::Instrument_const_sptr &inst) override;
  Geometry::IDetector_const_sptr getDetector() const override;
  Geometry::Instrument_const_sptr getInstrument() const override;

  bool findDetector() override;
  bool findDetector(const Geometry::InstrumentRayTracer &tracer) override;

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
  void setBankName(std::string m_bankName);
  void setHKL(double H, double K, double L) override;
  void setHKL(const Mantid::Kernel::V3D &HKL) override;
  void setIntHKL(const Kernel::V3D &HKL) override;
  void setIntMNP(const Mantid::Kernel::V3D &MNP) override;
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

  double getIntensity() const override;
  double getSigmaIntensity() const override;
  double getIntensityOverSigma() const override;

  void setIntensity(double m_intensity) override;
  void setSigmaIntensity(double m_sigmaIntensity) override;

  double getBinCount() const override;
  void setBinCount(double m_binCount) override;

  Mantid::Kernel::Matrix<double> getGoniometerMatrix() const override;
  void setGoniometerMatrix(
      const Mantid::Kernel::Matrix<double> &goniometerMatrix) override;

  std::string getBankName() const override;
  int getRow() const override;
  int getCol() const override;
  void setRow(int m_row);
  void setCol(int m_col);
  void setPeakNumber(int m_peakNumber) override;
  int getPeakNumber() const override;

  virtual Mantid::Kernel::V3D getDetPos() const override;
  virtual Mantid::Kernel::V3D getSamplePos() const override;
  double getL1() const override;
  double getL2() const override;

  double getValueByColName(std::string colName) const;

  /// Get the peak shape.
  const Mantid::Geometry::PeakShape &getPeakShape() const override;

  /// Set the PeakShape
  void setPeakShape(Mantid::Geometry::PeakShape *shape);

  /// Set the PeakShape
  void setPeakShape(Mantid::Geometry::PeakShape_const_sptr shape);

  /// Assignment
  LeanPeak &operator=(const LeanPeak &other);

  void setAbsorptionWeightedPathLength(double pathLength) override;
  double getAbsorptionWeightedPathLength() const override;

private:
  /// Name of the parent bank
  std::string m_bankName;

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

  /// Initial energy of neutrons at the peak
  double m_initialEnergy;

  /// Final energy of the neutrons at peak (normally same as m_InitialEnergy)
  double m_finalEnergy;

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

  /// Cached row in the detector
  int m_row;

  /// Cached column in the detector
  int m_col;

  /// Q_sample vector
  Mantid::Kernel::V3D m_Qsample;

  int m_peakNumber;
  Mantid::Kernel::V3D m_intHKL;
  Mantid::Kernel::V3D m_intMNP;

  /// Peak shape
  Mantid::Geometry::PeakShape_const_sptr m_peakShape;

  /// Static logger
  static Mantid::Kernel::Logger g_log;

  // ki-kf for Inelastic convention; kf-ki for Crystallography convention
  std::string convention;
};

} // namespace DataObjects
} // namespace Mantid
