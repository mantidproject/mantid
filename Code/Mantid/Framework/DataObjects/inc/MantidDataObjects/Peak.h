#ifndef MANTID_DATAOBJECTS_PEAK_H_
#define MANTID_DATAOBJECTS_PEAK_H_
    
#include "MantidKernel/System.h"
#include "MantidGeometry/Math/Matrix.h"


namespace Mantid
{
namespace DataObjects
{

  /** Structure describing a single-crystal peak
   * 
   * @author Janik Zikovsky
   * @date 2011-04-15 13:24:07.963491
   */
  class Peak
  {
  public:
    Peak();
    ~Peak();
    int getDetectorID() const;
    double getFinalEnergy() const;
    double getH() const;
    double getInitialEnergy() const;
    double getIntensity() const;
    double getK() const;
    double getL() const;
    double getSigIntensity() const;
    void setDetectorID(int m_DetectorID);
    void setFinalEnergy(double m_FinalEnergy);
    void setH(double m_H);
    void setInitialEnergy(double m_InitialEnergy);
    void setIntensity(double m_Intensity);
    void setK(double m_K);
    void setL(double m_L);
    void setSigIntensity(double m_SigIntensity);

    Mantid::Geometry::Matrix<double> getOrientationMatrix() const;
    void setOrientationMatrix(Mantid::Geometry::Matrix<double> m_OrientationMatrix);

    // -- Still to implement --
    double getWavelength();
    double getDSpacing();
    std::string getBankName();
    int getRow();
    int getCol();

  protected:
    /// Integrated peak intensity
    double m_Intensity;

    /// Error (sigma) on peak intensity
    double m_SigIntensity;

    /// Final energy of the peak
    double m_FinalEnergy;

    /// Initial Energy of the peak
    double m_InitialEnergy;

    /// ID of the detector
    int m_DetectorID;

    /// H of the peak
    double m_H;

    /// K of the peak
    double m_K;

    /// L of the peak
    double m_L;

    /// Sample orientation matrix
    Mantid::Geometry::Matrix<double> m_OrientationMatrix;
  };


} // namespace Mantid
} // namespace DataObjects

#endif  /* MANTID_DATAOBJECTS_PEAK_H_ */
