#ifndef MANTID_DATAOBJECTS_PEAK_H_
#define MANTID_DATAOBJECTS_PEAK_H_

#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/System.h"
#include "MantidGeometry/Crystal/PeakShape.h"
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>

namespace Mantid {
namespace DataObjects {

/** Structure describing a single-crystal peak
 *
 * @author Janik Zikovsky
 * @date 2011-04-15 13:24:07.963491
 */
class DLLExport Peak : public Geometry::IPeak {
public:
  /// Allow PeakColumn class to directly access members.
  friend class PeakColumn;

  Peak();
  Peak(Geometry::Instrument_const_sptr m_inst, Mantid::Kernel::V3D QLabFrame,
       boost::optional<double> detectorDistance = boost::optional<double>());
  Peak(const Geometry::Instrument_const_sptr &m_inst,
       const Mantid::Kernel::V3D &QSampleFrame,
       const Mantid::Kernel::Matrix<double> &goniometer,
       const boost::optional<double> &detectorDistance =
           boost::optional<double>());
  Peak(Geometry::Instrument_const_sptr m_inst, int m_detectorID,
       double m_Wavelength);
  Peak(Geometry::Instrument_const_sptr m_inst, int m_detectorID,
       double m_Wavelength, Mantid::Kernel::V3D HKL);
  Peak(Geometry::Instrument_const_sptr m_inst, int m_detectorID,
       double m_Wavelength, Mantid::Kernel::V3D HKL,
       Mantid::Kernel::Matrix<double> goniometer);
  Peak(Geometry::Instrument_const_sptr m_inst, double scattering,
       double m_Wavelength);

  /// Copy constructor
  Peak(const Peak &other);
  Peak(Peak &&) = default;
  Peak &operator=(Peak &&) = default;

  // Construct a peak from a reference to the interface

  explicit Peak(const Geometry::IPeak &ipeak);

  void setDetectorID(int id) override;
  int getDetectorID() const override;
  void addContributingDetID(const int id);
  void removeContributingDetector(const int id);
  const std::set<int> &getContributingDetIDs() const;

  void setInstrument(const Geometry::Instrument_const_sptr &inst) override;
  Geometry::IDetector_const_sptr getDetector() const override;
  Geometry::Instrument_const_sptr getInstrument() const override;

  bool findDetector() override;

  int getRunNumber() const override;
  void setRunNumber(int m_runNumber) override;

  double getMonitorCount() const override;
  void setMonitorCount(double m_monitorCount) override;

  double getH() const override;
  double getK() const override;
  double getL() const override;
  Mantid::Kernel::V3D getHKL() const override;
  void setH(double m_H) override;
  void setK(double m_K) override;
  void setL(double m_L) override;
  void setBankName(std::string m_bankName);
  void setHKL(double H, double K, double L) override;
  void setHKL(const Mantid::Kernel::V3D &HKL) override;
  void resetHKL();

  Mantid::Kernel::V3D getQLabFrame() const override;
  Mantid::Kernel::V3D getQSampleFrame() const override;
  Mantid::Kernel::V3D getDetectorPosition() const override;
  Mantid::Kernel::V3D getDetectorPositionNoCheck() const override;

  void setQSampleFrame(const Mantid::Kernel::V3D &QSampleFrame,
                       const boost::optional<double> &detectorDistance =
                           boost::optional<double>()) override;
  void setQLabFrame(const Mantid::Kernel::V3D &QLabFrame,
                    const boost::optional<double> &detectorDistance =
                        boost::optional<double>()) override;

  void setWavelength(double wavelength) override;
  double getWavelength() const override;
  double getScattering() const override;
  double getDSpacing() const override;
  double getTOF() const override;

  double getInitialEnergy() const override;
  double getFinalEnergy() const override;
  void setInitialEnergy(double m_initialEnergy) override;
  void setFinalEnergy(double m_finalEnergy) override;

  double getIntensity() const override;
  double getSigmaIntensity() const override;

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

  Mantid::Kernel::V3D getDetPos() const override;
  double getL1() const override;
  double getL2() const override;

  double getValueByColName(const std::string &name_in) const;

  /// Get the peak shape.
  const Mantid::Geometry::PeakShape &getPeakShape() const override;

  /// Set the PeakShape
  void setPeakShape(Mantid::Geometry::PeakShape *shape);

  /// Set the PeakShape
  void setPeakShape(Mantid::Geometry::PeakShape_const_sptr shape);

  /// Assignment
  Peak &operator=(const Peak &other);

private:
  bool findDetector(const Mantid::Kernel::V3D &beam);

  /// Shared pointer to the instrument (for calculating some values )
  Geometry::Instrument_const_sptr m_inst;

  /// Detector pointed to
  Geometry::IDetector_const_sptr m_det;

  /// Name of the parent bank
  std::string m_bankName;

  /// ID of the detector
  int m_detectorID;

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

  /// Cached source position
  Mantid::Kernel::V3D sourcePos;
  /// Cached sample position
  Mantid::Kernel::V3D samplePos;
  /// Cached detector position
  Mantid::Kernel::V3D detPos;

  /// save values before setHKL is called for use in SortHKL
  double m_orig_H;
  double m_orig_K;
  double m_orig_L;

  /// List of contributing detectors IDs
  std::set<int> m_detIDs;

  /// Peak shape
  Mantid::Geometry::PeakShape_const_sptr m_peakShape;

  /// Static logger
  static Mantid::Kernel::Logger g_log;

  // ki-kf for Inelastic convention; kf-ki for Crystallography convention
  std::string convention;
};

} // namespace Mantid
} // namespace DataObjects

#endif /* MANTID_DATAOBJECTS_PEAK_H_ */
