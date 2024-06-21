// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/BasePeak.h"
#include "MantidDataObjects/LeanElasticPeak.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/V3D.h"
#include <boost/optional.hpp>
#include <memory>

namespace Mantid {

namespace Geometry {
class InstrumentRayTracer;
}

namespace DataObjects {

/** Structure describing a single-crystal peak. The peak is described
 * by the physical detector position (determined either from detector
 * information or calculated from Q-lab) and initial/final energy
 * (calculated from Q-lab or provided wavelength)
 *
 */
class MANTID_DATAOBJECTS_DLL Peak : public BasePeak {
public:
  Peak();
  Peak(const Geometry::Instrument_const_sptr &m_inst, const Mantid::Kernel::V3D &QLabFrame,
       boost::optional<double> detectorDistance = boost::none);
  Peak(const Geometry::Instrument_const_sptr &m_inst, const Mantid::Kernel::V3D &QSampleFrame,
       const Mantid::Kernel::Matrix<double> &goniometer, boost::optional<double> detectorDistance = boost::none);
  Peak(const Geometry::Instrument_const_sptr &m_inst, int m_detectorID, double m_Wavelength);
  Peak(const Geometry::Instrument_const_sptr &m_inst, int m_detectorID, double m_Wavelength,
       const Mantid::Kernel::V3D &HKL);
  Peak(const Geometry::Instrument_const_sptr &m_inst, int m_detectorID, double m_Wavelength,
       const Mantid::Kernel::V3D &HKL, const Mantid::Kernel::Matrix<double> &goniometer);
  Peak(const Geometry::Instrument_const_sptr &m_inst, double scattering, double m_Wavelength);

  /// Copy constructor
  Peak(const Peak &other);

  // MSVC 2015/17 can build with noexcept = default however
  // intellisense still incorrectly reports this as an error despite compiling.
  // https://connect.microsoft.com/VisualStudio/feedback/details/1795240/visual-c-2015-default-move-constructor-and-noexcept-keyword-bug
  // For that reason we still use the supplied default which should be noexcept
  // once the above is fixed we can remove this workaround

#if defined(_MSC_VER) && _MSC_VER <= 1910
  Peak(Peak &&) = default;
  Peak &operator=(Peak &&) = default;
#else
  Peak(Peak &&) noexcept = default;
  Peak &operator=(Peak &&) noexcept = default;
#endif

  // Construct a peak from a reference to the interface

  explicit Peak(const Geometry::IPeak &ipeak);

  // Construct a peak from LeanPeak
  Peak(const Mantid::DataObjects::LeanElasticPeak &lpeak, const Geometry::Instrument_const_sptr &inst,
       boost::optional<double> detectorDistance = boost::none);

  void setDetectorID(int id);
  int getDetectorID() const override;
  void addContributingDetID(const int id);
  void removeContributingDetector(const int id);
  const std::set<int> &getContributingDetIDs() const;

  void setInstrument(const Geometry::Instrument_const_sptr &inst);
  Geometry::IDetector_const_sptr getDetector() const;
  Geometry::Instrument_const_sptr getInstrument() const;
  std::shared_ptr<const Geometry::ReferenceFrame> getReferenceFrame() const override;

  int getCol() const override;
  void setCol(int col);

  int getRow() const override;
  void setRow(int row);

  std::string getBankName() const;
  void setBankName(std::string bankName);

  bool findDetector();
  bool findDetector(const Geometry::InstrumentRayTracer &tracer);

  Mantid::Kernel::V3D getQLabFrame() const override;
  Mantid::Kernel::V3D getQSampleFrame() const override;
  Mantid::Kernel::V3D getDetectorPosition() const;
  Mantid::Kernel::V3D getDetectorPositionNoCheck() const;

  void setQSampleFrame(const Mantid::Kernel::V3D &QSampleFrame,
                       boost::optional<double> detectorDistance = boost::none) override;
  void setQLabFrame(const Mantid::Kernel::V3D &qLab, boost::optional<double> detectorDistance = boost::none) override;

  void setWavelength(double wavelength) override;
  double getWavelength() const override;
  double getScattering() const override;
  double getAzimuthal() const override;
  double getDSpacing() const override;
  double getTOF() const override;
  const Mantid::Kernel::V3D &getCachedDetectorPosition() const { return detPos; }

  double getInitialEnergy() const override;
  double getFinalEnergy() const override;
  double getEnergyTransfer() const override;
  void setInitialEnergy(double m_initialEnergy) override;
  void setFinalEnergy(double m_finalEnergy) override;

  virtual Mantid::Kernel::V3D getDetPos() const;
  double getL1() const override;
  double getL2() const override;

  Mantid::Kernel::V3D getDetectorDirectionSampleFrame() const override;
  Mantid::Kernel::V3D getSourceDirectionSampleFrame() const override;

  /// Assignment
  Peak &operator=(const Peak &other);

  /// Get the approximate position of a peak that falls off the detectors
  Kernel::V3D getVirtualDetectorPosition(const Kernel::V3D &detectorDir) const;

  double getValueByColName(std::string colName) const override;

private:
  bool findDetector(const Mantid::Kernel::V3D &beam, const Geometry::InstrumentRayTracer &tracer);

  /// Shared pointer to the instrument (for calculating some values )
  Geometry::Instrument_const_sptr m_inst;

  /// Detector pointed to
  Geometry::IDetector_const_sptr m_det;

  /// Name of the parent bank
  std::string m_bankName;

  /// Cached row in the detector
  int m_row;

  /// Cached column in the detector
  int m_col;

  /// ID of the detector
  int m_detectorID;

  /// Initial energy of neutrons at the peak
  double m_initialEnergy;

  /// Final energy of the neutrons at peak (normally same as m_InitialEnergy)
  double m_finalEnergy;

  /// Cached source position
  Mantid::Kernel::V3D sourcePos;

  /// Cached detector position
  Mantid::Kernel::V3D detPos;

  /// List of contributing detectors IDs
  std::set<int> m_detIDs;

  /// Static logger
  static Mantid::Kernel::Logger g_log;
};

using Peak_uptr = std::unique_ptr<Peak>;

} // namespace DataObjects
} // namespace Mantid
