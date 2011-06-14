#ifndef MANTID_API_IPEAK_H_
#define MANTID_API_IPEAK_H_
    
#include "MantidAPI/DllConfig.h"
#include "MantidGeometry/IInstrument.h"
#include "MantidGeometry/Math/Matrix.h"
#include "MantidGeometry/V3D.h"
#include "MantidKernel/PhysicalConstants.h"
//#include "MantidKernel/System.h"


namespace Mantid
{
namespace API
{

  /** Structure describing a single-crystal peak
   * 
   * @author Janik Zikovsky
   * @date 2011-04-15 13:24:07.963491
   */
  class MANTID_API_DLL IPeak
  {
  public:

    virtual ~IPeak();

    virtual void setInstrument(Mantid::Geometry::IInstrument_const_sptr inst) = 0;

    virtual int getDetectorID() const = 0;
    virtual void setDetectorID(int m_DetectorID) = 0;
    virtual Mantid::Geometry::IDetector_const_sptr getDetector() const = 0;
    virtual Mantid::Geometry::IInstrument_const_sptr getInstrument() const = 0;

    virtual int getRunNumber() const = 0;
    virtual void setRunNumber(int m_RunNumber) = 0;

    virtual double getH() const = 0;
    virtual double getK() const = 0;
    virtual double getL() const = 0;
    virtual Mantid::Geometry::V3D getHKL() = 0;
    virtual void setH(double m_H) = 0;
    virtual void setK(double m_K) = 0;
    virtual void setL(double m_L) = 0;
    virtual void setHKL(double H, double K, double L) = 0;
    virtual void setHKL(Mantid::Geometry::V3D HKL) = 0;

    virtual Mantid::Geometry::V3D getQLabFrame() const = 0;
    virtual Mantid::Geometry::V3D getQSampleFrame() const = 0;

    virtual void setQSampleFrame(Mantid::Geometry::V3D QSampleFrame, double detectorDistance=1.0) = 0;
    virtual void setQLabFrame(Mantid::Geometry::V3D QLabFrame, double detectorDistance=1.0) = 0;

    virtual void setWavelength(double wavelength) = 0;
    virtual double getWavelength() const = 0;
    virtual double getDSpacing() const = 0;
    virtual double getTOF() const = 0;

    virtual double getInitialEnergy() const = 0;
    virtual double getFinalEnergy() const = 0;
    virtual void setInitialEnergy(double m_InitialEnergy) = 0;
    virtual void setFinalEnergy(double m_FinalEnergy) = 0;

    virtual double getIntensity() const = 0;
    virtual double getSigmaIntensity() const = 0;

    virtual void setIntensity(double m_Intensity) = 0;
    virtual void setSigmaIntensity(double m_SigmaIntensity) = 0;

    virtual double getBinCount() const = 0;
    virtual void setBinCount(double m_BinCount) = 0;

    virtual Mantid::Geometry::Matrix<double> getGoniometerMatrix() const = 0;
    virtual void setGoniometerMatrix(Mantid::Geometry::Matrix<double> m_GoniometerMatrix) = 0;

    virtual std::string getBankName() const = 0;
    virtual int getRow() const = 0;
    virtual int getCol() const = 0;

    virtual Mantid::Geometry::V3D getDetPos() const = 0;
    virtual double getL1() const = 0;
    virtual double getL2() const = 0;

  };


} // namespace Mantid
} // namespace API

#endif  /* MANTID_API_IPEAK_H_ */
