#ifndef MANTID_DATAOBJECTS_PEAK_H_
#define MANTID_DATAOBJECTS_PEAK_H_
    
#include "MantidKernel/System.h"
#include "MantidGeometry/Math/Matrix.h"
#include "MantidGeometry/IInstrument.h"


namespace Mantid
{
namespace DataObjects
{

  /** Structure describing a single-crystal peak
   * 
   * @author Janik Zikovsky
   * @date 2011-04-15 13:24:07.963491
   */
  class DLLExport Peak
  {
  public:
    Peak(Mantid::Geometry::IInstrument_sptr m_inst, int m_DetectorID, double m_InitialEnergy);
    ~Peak();

    int getDetectorID() const;

    double getH() const;
    double getK() const;
    double getL() const;

    double getInitialEnergy() const;
    double getFinalEnergy() const;

    double getIntensity() const;
    double getSigIntensity() const;

    void setDetectorID(int m_DetectorID);

    void setH(double m_H);
    void setK(double m_K);
    void setL(double m_L);
    void setHKL(double H, double K, double L);

    void setInitialEnergy(double m_InitialEnergy);
    void setFinalEnergy(double m_FinalEnergy);

    void setIntensity(double m_Intensity);
    void setSigIntensity(double m_SigIntensity);

    Mantid::Geometry::Matrix<double> getOrientationMatrix() const;
    void setOrientationMatrix(Mantid::Geometry::Matrix<double> m_OrientationMatrix);

    // ------ Still to implement -------
    double getWavelength();
    double getDSpacing();
    std::string getBankName();
    int getRow();
    int getCol();

  protected:
    /// Shared pointer to the instrument (for calculating some values )
    Mantid::Geometry::IInstrument_sptr m_inst;

    /// Detector pointed to
    Mantid::Geometry::IDetector_sptr m_det;

    /// ID of the detector
    int m_DetectorID;

    /// H of the peak
    double m_H;

    /// K of the peak
    double m_K;

    /// L of the peak
    double m_L;

    /// Integrated peak intensity
    double m_Intensity;

    /// Error (sigma) on peak intensity
    double m_SigIntensity;

    /// Initial energy of neutrons at the peak
    double m_InitialEnergy;

    /// Final energy of the neutrons at peak (normally same as m_InitialEnergy)
    double m_FinalEnergy;

    /// Sample orientation matrix
    Mantid::Geometry::Matrix<double> m_OrientationMatrix;
  };


} // namespace Mantid
} // namespace DataObjects

#endif  /* MANTID_DATAOBJECTS_PEAK_H_ */
