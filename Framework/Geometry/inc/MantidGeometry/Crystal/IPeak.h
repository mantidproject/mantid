// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_IPEAK_H_
#define MANTID_API_IPEAK_H_

#include "MantidGeometry/Crystal/PeakShape.h"
#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/V3D.h"
#include <boost/optional.hpp>

namespace Mantid {
namespace Geometry {
class InstrumentRayTracer;

/** Structure describing a single-crystal peak
 *
 * @author Janik Zikovsky
 * @date 2011-04-15 13:24:07.963491
 */
class MANTID_GEOMETRY_DLL IPeak {
public:
  virtual ~IPeak() = default;

  virtual void setInstrument(const Geometry::Instrument_const_sptr &inst) = 0;

  virtual int getDetectorID() const = 0;
  virtual void setDetectorID(int m_DetectorID) = 0;
  virtual Geometry::IDetector_const_sptr getDetector() const = 0;
  virtual Geometry::Instrument_const_sptr getInstrument() const = 0;

  virtual int getRunNumber() const = 0;
  virtual void setRunNumber(int m_RunNumber) = 0;

  virtual double getMonitorCount() const = 0;
  virtual void setMonitorCount(double m_MonitorCount) = 0;

  virtual double getH() const = 0;
  virtual double getK() const = 0;
  virtual double getL() const = 0;
  virtual Mantid::Kernel::V3D getHKL() const = 0;
  virtual Mantid::Kernel::V3D getIntHKL() const = 0;
  virtual void setH(double m_H) = 0;
  virtual void setK(double m_K) = 0;
  virtual void setL(double m_L) = 0;
  virtual void setHKL(double H, double K, double L) = 0;
  virtual void setHKL(const Mantid::Kernel::V3D &HKL) = 0;
  virtual void setIntHKL(const Mantid::Kernel::V3D HKL) = 0;
  virtual void setSamplePos(double samX, double samY, double samZ) = 0;
  virtual void setSamplePos(const Mantid::Kernel::V3D &XYZ) = 0;
  virtual Mantid::Kernel::V3D getSamplePos() const = 0;
  virtual Mantid::Kernel::V3D getDetectorPosition() const = 0;
  virtual Mantid::Kernel::V3D getDetectorPositionNoCheck() const = 0;

  virtual Mantid::Kernel::V3D getQLabFrame() const = 0;
  virtual Mantid::Kernel::V3D getQSampleFrame() const = 0;
  virtual bool findDetector() = 0;
  virtual bool findDetector(const InstrumentRayTracer &tracer) = 0;

  virtual void setQSampleFrame(const Mantid::Kernel::V3D &QSampleFrame,
                               boost::optional<double> detectorDistance) = 0;
  virtual void setQLabFrame(const Mantid::Kernel::V3D &QLabFrame,
                            boost::optional<double> detectorDistance) = 0;

  virtual void setWavelength(double wavelength) = 0;
  virtual double getWavelength() const = 0;
  virtual double getScattering() const = 0;
  virtual double getAzimuthal() const = 0;
  virtual double getDSpacing() const = 0;
  virtual double getTOF() const = 0;

  virtual double getInitialEnergy() const = 0;
  virtual double getFinalEnergy() const = 0;
  virtual double getEnergyTransfer() const = 0;
  virtual void setInitialEnergy(double m_InitialEnergy) = 0;
  virtual void setFinalEnergy(double m_FinalEnergy) = 0;

  virtual double getIntensity() const = 0;
  virtual double getSigmaIntensity() const = 0;
  virtual double getIntensityOverSigma() const = 0;
  virtual void setIntensity(double m_Intensity) = 0;
  virtual void setSigmaIntensity(double m_SigmaIntensity) = 0;

  virtual double getBinCount() const = 0;
  virtual void setBinCount(double m_BinCount) = 0;

  virtual int getPeakNumber() const = 0;
  virtual void setPeakNumber(int m_PeakNumber) = 0;

  virtual Mantid::Kernel::V3D getIntMNP() const = 0;
  virtual void setIntMNP(const Mantid::Kernel::V3D MNP) = 0;

  virtual Mantid::Kernel::Matrix<double> getGoniometerMatrix() const = 0;
  virtual void setGoniometerMatrix(
      const Mantid::Kernel::Matrix<double> &m_GoniometerMatrix) = 0;

  virtual std::string getBankName() const = 0;
  virtual int getRow() const = 0;
  virtual int getCol() const = 0;

  virtual Mantid::Kernel::V3D getDetPos() const = 0;
  virtual double getL1() const = 0;
  virtual double getL2() const = 0;

  virtual const Mantid::Geometry::PeakShape &getPeakShape() const = 0;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_API_IPEAK_H_ */
