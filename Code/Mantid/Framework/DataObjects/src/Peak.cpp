#include "MantidDataObjects/Peak.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace DataObjects
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  Peak::Peak()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  Peak::~Peak()
  {
  }

  int Peak::getDetectorID() const
  {
    return m_DetectorID;
  }

  double Peak::getFinalEnergy() const
  {
    return m_FinalEnergy;
  }

  double Peak::getH() const
  {
    return m_H;
  }

  double Peak::getInitialEnergy() const
  {
    return m_InitialEnergy;
  }

  double Peak::getIntensity() const
  {
    return m_Intensity;
  }

  double Peak::getK() const
  {
    return m_K;
  }

  double Peak::getL() const
  {
    return m_L;
  }

  double Peak::getSigIntensity() const
  {
    return m_SigIntensity;
  }

  void Peak::setDetectorID(int m_DetectorID)
  {
    this->m_DetectorID = m_DetectorID;
  }

  void Peak::setFinalEnergy(double m_FinalEnergy)
  {
    this->m_FinalEnergy = m_FinalEnergy;
  }

  void Peak::setH(double m_H)
  {
    this->m_H = m_H;
  }

  void Peak::setInitialEnergy(double m_InitialEnergy)
  {
    this->m_InitialEnergy = m_InitialEnergy;
  }

  void Peak::setIntensity(double m_Intensity)
  {
    this->m_Intensity = m_Intensity;
  }

  void Peak::setK(double m_K)
  {
    this->m_K = m_K;
  }

  void Peak::setL(double m_L)
  {
    this->m_L = m_L;
  }

  void Peak::setSigIntensity(double m_SigIntensity)
  {
    this->m_SigIntensity = m_SigIntensity;
  }

  Mantid::Geometry::Matrix<double> Peak::getOrientationMatrix() const
  {
    return this->m_OrientationMatrix;
  }

  void Peak::setOrientationMatrix(Mantid::Geometry::Matrix<double> m_OrientationMatrix)
  {
    this->m_OrientationMatrix = m_OrientationMatrix;
  }



} // namespace Mantid
} // namespace DataObjects

