#include "MantidDataObjects/Peak.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/System.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

namespace Mantid
{
namespace DataObjects
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   *
   * @param m_inst :: Shared pointer to the instrument for this peak detection
   * @param m_DetectorID :: ID to the detector of the center of the peak
   * @param m_InitialEnergy :: incident neutron energy, in mEv (UNITS??)
   * @return
   */
  Peak::Peak(Mantid::Geometry::IInstrument_sptr m_inst, int m_DetectorID, double m_InitialEnergy)
    : m_inst(m_inst),
      m_H(0), m_K(0), m_L(0),
      m_Intensity(0), m_SigIntensity(0),
      m_InitialEnergy(m_InitialEnergy),
      m_FinalEnergy(m_InitialEnergy),
      m_OrientationMatrix(3,3)
  {
    this->setDetectorID(m_DetectorID);
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
    if (m_inst)
      this->m_det = m_inst->getDetector(this->m_DetectorID);
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

  void Peak::setHKL(double H, double K, double L)
  {
    m_H = H;
    m_K = K;
    m_L = L;
  }

  // -------------------------------------------------------------------------------------
  /** Calculate the neutron wavelength at the peak */
  double Peak::getWavelength()
  {
    return 0.0; //TODO:
  }

  // -------------------------------------------------------------------------------------
  /** Calculate the d-spacing of the peak  */
  double Peak::getDSpacing()
  {
    return 0.0; //TODO:
  }

  // -------------------------------------------------------------------------------------
  /** Find the name of the bank that is the parent of the detector. This works
   * best for RectangularDetector instruments (goes up one level)
   * @return name of the bank.
   */
  std::string Peak::getBankName()
  {
    if (!m_det) return "";
    if (!m_det->getParent()) return "";
    return m_det->getParent()->getName();
  }

  // -------------------------------------------------------------------------------------
  /** For RectangularDetectors only, returns the row (y) of the pixel of the detector. */
  int Peak::getRow()
  {
    if (!m_det) throw std::runtime_error("No detector in Peak!");
    if (!m_det->getParent()) throw std::runtime_error("No parent to the detector in Peak!");
    RectangularDetector_const_sptr retDet = boost::dynamic_pointer_cast<const RectangularDetector>(m_det->getParent());
    if (!retDet) throw std::runtime_error("Parent of the detector in Peak is not RectangularDetector: can't find the row!");
    std::pair<int,int> xy = retDet->getXYForDetectorID(m_DetectorID);
    return xy.second;
  }

  // -------------------------------------------------------------------------------------
  /** For RectangularDetectors only, returns the row of the pixel of the detector. */
  int Peak::getCol()
  {
    if (!m_det) throw std::runtime_error("No detector in Peak!");
    if (!m_det->getParent()) throw std::runtime_error("No parent to the detector in Peak!");
    RectangularDetector_const_sptr retDet = boost::dynamic_pointer_cast<const RectangularDetector>(m_det->getParent());
    if (!retDet) throw std::runtime_error("Parent of the detector in Peak is not RectangularDetector: can't find the col!");
    std::pair<int,int> xy = retDet->getXYForDetectorID(m_DetectorID);
    return xy.first;
  }




} // namespace Mantid
} // namespace DataObjects

