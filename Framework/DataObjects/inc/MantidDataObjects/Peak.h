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
  Peak(Geometry::Instrument_const_sptr m_inst, Mantid::Kernel::V3D QSampleFrame,
       Mantid::Kernel::Matrix<double> goniometer,
       boost::optional<double> detectorDistance = boost::optional<double>());
  Peak(Geometry::Instrument_const_sptr m_inst, int m_DetectorID,
       double m_Wavelength);
  Peak(Geometry::Instrument_const_sptr m_inst, int m_DetectorID,
       double m_Wavelength, Mantid::Kernel::V3D HKL);
  Peak(Geometry::Instrument_const_sptr m_inst, int m_DetectorID,
       double m_Wavelength, Mantid::Kernel::V3D HKL,
       Mantid::Kernel::Matrix<double> goniometer);
  Peak(Geometry::Instrument_const_sptr m_inst, double scattering,
       double m_Wavelength);

  /// Copy constructor
  Peak(const Peak &other);

  // Construct a peak from a reference to the interface

  explicit Peak(const Geometry::IPeak &ipeak);
  virtual ~Peak();

  void setDetectorID(int id);
  int getDetectorID() const;
  void addContributingDetID(const int id);
  void removeContributingDetector(const int id);
  const std::set<int> &getContributingDetIDs() const;

  void setInstrument(Geometry::Instrument_const_sptr inst);
  Geometry::IDetector_const_sptr getDetector() const;
  Geometry::Instrument_const_sptr getInstrument() const;

  bool findDetector();

  int getRunNumber() const;
  void setRunNumber(int m_RunNumber);

  double getMonitorCount() const;
  void setMonitorCount(double m_MonitorCount);

  double getH() const;
  double getK() const;
  double getL() const;
  Mantid::Kernel::V3D getHKL() const;
  void setH(double m_H);
  void setK(double m_K);
  void setL(double m_L);
  void setBankName(std::string m_BankName);
  void setHKL(double H, double K, double L);
  void setHKL(Mantid::Kernel::V3D HKL);
  void resetHKL();

  Mantid::Kernel::V3D getQLabFrame() const;
  Mantid::Kernel::V3D getQSampleFrame() const;
  Mantid::Kernel::V3D getDetectorPosition() const;
  Mantid::Kernel::V3D getDetectorPositionNoCheck() const;

  void setQSampleFrame(
      Mantid::Kernel::V3D QSampleFrame,
      boost::optional<double> detectorDistance = boost::optional<double>());
  void setQLabFrame(
      Mantid::Kernel::V3D QLabFrame,
      boost::optional<double> detectorDistance = boost::optional<double>());

  void setWavelength(double wavelength);
  double getWavelength() const;
  double getScattering() const;
  double getDSpacing() const;
  double getTOF() const;

  double getInitialEnergy() const;
  double getFinalEnergy() const;
  void setInitialEnergy(double m_InitialEnergy);
  void setFinalEnergy(double m_FinalEnergy);

  double getIntensity() const;
  double getSigmaIntensity() const;

  void setIntensity(double m_Intensity);
  void setSigmaIntensity(double m_SigmaIntensity);

  double getBinCount() const;
  void setBinCount(double m_BinCount);

  Mantid::Kernel::Matrix<double> getGoniometerMatrix() const;
  void setGoniometerMatrix(Mantid::Kernel::Matrix<double> m_GoniometerMatrix);

  std::string getBankName() const;
  int getRow() const;
  int getCol() const;

  Mantid::Kernel::V3D getDetPos() const;
  double getL1() const;
  double getL2() const;

  double getValueByColName(const std::string &name) const;

  /// Get the peak shape.
  const Mantid::Geometry::PeakShape &getPeakShape() const;

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
};

} // namespace Mantid
} // namespace DataObjects

#endif /* MANTID_DATAOBJECTS_PEAK_H_ */
