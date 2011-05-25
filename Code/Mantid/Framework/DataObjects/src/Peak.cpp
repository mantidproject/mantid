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
   * @param m_Wavelength :: incident neutron wavelength, in Angstroms
   * @return
   */
  Peak::Peak(Mantid::Geometry::IInstrument_sptr m_inst, int m_DetectorID, double m_Wavelength)
  : m_inst(m_inst),
    m_H(0), m_K(0), m_L(0),
    m_Intensity(0), m_SigmaIntensity(0), m_BinCount(0),
    m_GoniometerMatrix(3,3,true),
    m_InverseGoniometerMatrix(3,3,true),
    m_RunNumber(0)
  {
    this->setDetectorID(m_DetectorID);
    this->setWavelength(m_Wavelength);
  }


  //----------------------------------------------------------------------------------------------
  /** Constructor
   *
   * @param m_inst :: Shared pointer to the instrument for this peak detection
   * @param m_DetectorID :: ID to the detector of the center of the peak
   * @param m_Wavelength :: incident neutron wavelength, in Angstroms
   * @param HKL :: vector with H,K,L position of the peak
   * @return
   */
  Peak::Peak(Mantid::Geometry::IInstrument_sptr m_inst, int m_DetectorID, double m_Wavelength, Mantid::Geometry::V3D HKL) :
    m_inst(m_inst),
    m_H(HKL[0]), m_K(HKL[1]), m_L(HKL[2]),
    m_Intensity(0), m_SigmaIntensity(0), m_BinCount(0),
    m_GoniometerMatrix(3,3,true),
    m_InverseGoniometerMatrix(3,3,true),
    m_RunNumber(0)
  {
    this->setDetectorID(m_DetectorID);
    this->setWavelength(m_Wavelength);
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor
   *
   * @param m_inst :: Shared pointer to the instrument for this peak detection
   * @param m_DetectorID :: ID to the detector of the center of the peak
   * @param m_Wavelength :: incident neutron wavelength, in Angstroms
   * @param HKL :: vector with H,K,L position of the peak
   * @param goniometer :: a 3x3 rotation matrix
   * @return
   */
  Peak::Peak(Mantid::Geometry::IInstrument_sptr m_inst, int m_DetectorID, double m_Wavelength, Mantid::Geometry::V3D HKL,Mantid::Geometry::Matrix<double> goniometer) :
    m_inst(m_inst),
    m_H(HKL[0]), m_K(HKL[1]), m_L(HKL[2]),
    m_Intensity(0), m_SigmaIntensity(0), m_BinCount(0),
    m_GoniometerMatrix(goniometer),
    m_InverseGoniometerMatrix(goniometer),
    m_RunNumber(0)
  {
    m_InverseGoniometerMatrix.Invert();
    this->setDetectorID(m_DetectorID);
    this->setWavelength(m_Wavelength);
  }


//  /** Copy constructor
//   * @param other :: Peak to copy.
//   */
//  Peak::Peak(const Peak & other)
//  : m_inst(other.
//  {
//  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  Peak::~Peak()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Set the incident wavelength of the neutron. Calculates the energy from this. Assumes elastic scattering.
   *
   * @param wavelength :: wavelength in Angstroms.
   */
  void Peak::setWavelength(double wavelength)
  {
    // Velocity of the neutron (non-relativistic)
    double velocity = PhysicalConstants::h / (wavelength * 1e-10 * PhysicalConstants::NeutronMass );
    // Energy in J of the neutron
    double energy = PhysicalConstants::NeutronMass * velocity * velocity / 2.0;
    // Convert to meV
    m_InitialEnergy = energy / PhysicalConstants::meV;
    m_FinalEnergy = m_InitialEnergy;
  }

  //----------------------------------------------------------------------------------------------
  /** Set the detector ID and look up and cache values related to it.
   *
   * @param m_DetectorID :: ID of detector at the center of the peak
   */
  void Peak::setDetectorID(int m_DetectorID)
  {
    this->m_DetectorID = m_DetectorID;
    if (!m_inst) throw std::runtime_error("No instrument is set!");
    this->m_det = m_inst->getDetector(this->m_DetectorID);
    if (!m_det) throw std::runtime_error("No detector was found!");

    // Cache some positions
    const Geometry::IObjComponent_sptr sourceObj = m_inst->getSource();
    if (sourceObj == NULL)
      throw Exception::InstrumentDefinitionError("Failed to get source component from instrument");
    const Geometry::IObjComponent_sptr sampleObj = m_inst->getSample();
    if (sampleObj == NULL)
      throw Exception::InstrumentDefinitionError("Failed to get sample component from instrument");

    sourcePos = sourceObj->getPos();
    samplePos = sampleObj->getPos();
    detPos = m_det->getPos();

    // We now look for the row/column. -1 if not found.
    m_Row = -1;
    m_Col = -1;

    // Go up 2 parents to find the rectangular detector
    IComponent_const_sptr parent = m_det->getParent();
    if (!parent) return;
    m_BankName = parent->getName(); // Use the parent by default
    parent = parent->getParent();
    if (!parent) return;
    RectangularDetector_const_sptr retDet = boost::dynamic_pointer_cast<const RectangularDetector>(parent);
    if (!retDet) return;
    m_BankName = retDet->getName(); // Use the grand-parent for rectangular detectors

    std::pair<int,int> xy = retDet->getXYForDetectorID(m_DetectorID);
    m_Row = xy.second;
    m_Col = xy.first;
  }


  // -------------------------------------------------------------------------------------
  /** Calculate the neutron wavelength (in angstroms) at the peak (Note for inelastic scattering - it is the wavelength corresponding to the final energy)*/
  double Peak::getWavelength() const
  {
    // Energy in J of the neutron
    double energy =  PhysicalConstants::meV * m_FinalEnergy;
    // v = sqrt(2.0 * E / m)
    double velocity = sqrt(2.0*energy/PhysicalConstants::NeutronMass);
    // wavelength = h / mv
    double wavelength = PhysicalConstants::h / (PhysicalConstants::NeutronMass * velocity);
    // Return it in angstroms
    return wavelength * 1e10;
  }

  // -------------------------------------------------------------------------------------
  /** Calculate the time of flight (in microseconds) of the neutrons for this peak,
   * using the geometry of the detector  */
  double Peak::getTOF() const
  {
    // First, get the neutron traveling distances
    double L1 = this->getL1();
    double L2 = this->getL2();
    // Energy in J of the neutron
    double Ei =  PhysicalConstants::meV * m_InitialEnergy;
    double Ef =  PhysicalConstants::meV * m_FinalEnergy;
    // v = sqrt(2 * E / m)
    double vi = sqrt(2.0*Ei/PhysicalConstants::NeutronMass);
    double vf = sqrt(2.0*Ef/PhysicalConstants::NeutronMass);
    // Time of flight in seconds = distance / speed
    double tof = L1/vi+L2/vf;
    // Return in microsecond units
    return tof * 1e6;
  }

  // -------------------------------------------------------------------------------------
  /** Calculate the d-spacing of the peak, in 1/Angstroms  */
  double Peak::getDSpacing() const
  {
    // The detector is at 2 theta scattering angle
    V3D beamDir = samplePos - sourcePos;
    V3D detDir = detPos - samplePos;

    double two_theta = detDir.angle(beamDir);
    
    // In general case (2*pi/d)^2=k_i^2+k_f^2-2*k_i*k_f*cos(two_theta)
    // E_i,f=k_i,f^2*hbar^2/(2 m)
    return 1e10*PhysicalConstants::h/sqrt(2.0*PhysicalConstants::NeutronMass*PhysicalConstants::meV)/sqrt(m_InitialEnergy+m_FinalEnergy-2.0*sqrt(m_InitialEnergy*m_FinalEnergy)*cos(two_theta));
  }

  //----------------------------------------------------------------------------------------------
  /** Return the Q change (of the lattice, k_i - k_f) for this peak.
   * The Q is in the Lab frame: the goniometer rotation was NOT taken out.
   *
   * Note: There is no 2*pi factor used, so |Q| = 1/wavelength.
   * */
  Mantid::Geometry::V3D Peak::getQLabFrame() const
  {
    // Normalized beam direction
    V3D beamDir = samplePos - sourcePos;
    beamDir /= beamDir.norm();
    // Normalized detector direction
    V3D detDir = (detPos - samplePos);
    detDir /= detDir.norm();

    // Energy in J of the neutron
    double ei =  PhysicalConstants::meV * m_InitialEnergy;
    // v = sqrt(2.0 * E / m)
    double vi = sqrt(2.0*ei/PhysicalConstants::NeutronMass);
    // wavelength = h / mv
    double wi = PhysicalConstants::h / (PhysicalConstants::NeutronMass * vi);
    // in angstroms
    wi*= 1e10;
    //wavevector=1/wavelength
    double wvi=1.0/wi;
    // Now calculate the wavevector of the scattered neutron
    double wvf = 1.0 / this->getWavelength();
    // And Q in the lab frame 
    return beamDir*wvi-detDir*wvf;
  }

  //----------------------------------------------------------------------------------------------
  /** Return the Q change (of the lattice, k_i - k_f) for this peak.
   * The Q is in the Sample frame: the goniometer rotation WAS taken out. */
  Mantid::Geometry::V3D Peak::getQSampleFrame() const
  {
    V3D Qlab = this->getQLabFrame();
    // Multiply by the inverse of the goniometer matrix to get the sample frame
    V3D Qsample = m_InverseGoniometerMatrix * Qlab;
    return Qsample;
  }



  //----------------------------------------------------------------------------------------------
  /** Return a shared ptr to the detector at center of peak. */
  Mantid::Geometry::IDetector_sptr Peak::getDetector() const
  {    return m_det;  }

  /** Return a shared ptr to the instrument for this peak. */
  Mantid::Geometry::IInstrument_sptr Peak::getInstrument() const
  {    return m_inst;  }

  //----------------------------------------------------------------------------------------------
  /** Return the run number this peak was measured at. */
  int Peak::getRunNumber() const
  {    return m_RunNumber;  }

  /** Set the run number that measured this peak
   * @param m_RunNumber :: the run number   */
  void Peak::setRunNumber(int m_RunNumber)
  {    this->m_RunNumber = m_RunNumber;  }

  //----------------------------------------------------------------------------------------------
  /** Get the ID of the detector at the center of the peak  */
  int Peak::getDetectorID() const
  {    return m_DetectorID;  }

  //----------------------------------------------------------------------------------------------
  /** Get the final neutron energy */
  double Peak::getFinalEnergy() const
  {    return m_FinalEnergy;  }

  /** Get the initial (incident) neutron energy */
  double Peak::getInitialEnergy() const
  {    return m_InitialEnergy; }

  //----------------------------------------------------------------------------------------------
  /** Get the H index of the peak */
  double Peak::getH() const
  {    return m_H;  }

  /** Get the K index of the peak */
  double Peak::getK() const
  {    return m_K;  }

  /** Get the L index of the peak */
  double Peak::getL() const
  {    return m_L;  }

  /** Return the HKL vector */
  Mantid::Geometry::V3D Peak::getHKL()
  {
    return V3D(m_H, m_K, m_L);
  }

  //----------------------------------------------------------------------------------------------
  /** Set the H index of this peak
   * @param m_H :: index to set   */
  void Peak::setH(double m_H)
  {    this->m_H = m_H;  }

  /** Set the K index of this peak
   * @param m_K :: index to set   */
  void Peak::setK(double m_K)
  {    this->m_K = m_K;  }

  /** Set the L index of this peak
   * @param m_L :: index to set   */
  void Peak::setL(double m_L)
  {    this->m_L = m_L;  }

  /** Set all three H,K,L indices of the peak */
  void Peak::setHKL(double H, double K, double L)
  {
    m_H = H;
    m_K = K;
    m_L = L;
  }

  /** Set all HKL
   *
   * @param HKL :: vector with x,y,z -> h,k,l
   */
  void Peak::setHKL(Mantid::Geometry::V3D HKL)
  {
    m_H = HKL.X();
    m_K = HKL.Y();
    m_L = HKL.Z();
  }

  //----------------------------------------------------------------------------------------------
  /** Return the # of counts in the bin at its peak*/
  double Peak::getBinCount() const
  {    return m_BinCount;  }

  /** Return the integrated peak intensity */
  double Peak::getIntensity() const
  {    return m_Intensity;  }

  /** Return the error on the integrated peak intensity */
  double Peak::getSigmaIntensity() const
  {    return m_SigmaIntensity;  }

  /** Set the integrated peak intensity
   * @param m_Intensity :: intensity value   */
  void Peak::setIntensity(double m_Intensity)
  {    this->m_Intensity = m_Intensity;  }

  /** Set the # of counts in the bin at its peak
   * @param m_BinCount :: counts  */
  void Peak::setBinCount(double m_BinCount)
  {    this->m_BinCount = m_BinCount;  }

  /** Set the error on the integrated peak intensity
   * @param m_Intensity :: intensity error value   */
  void Peak::setSigmaIntensity(double m_SigmaIntensity)
  {    this->m_SigmaIntensity = m_SigmaIntensity;  }

  /** Set the final energy
   * @param m_FinalEnergy :: final energy in meV   */
  void Peak::setFinalEnergy(double m_FinalEnergy)
  {    this->m_FinalEnergy = m_FinalEnergy;  }

  /** Set the initial energy
   * @param m_InitialEnergy :: initial energy in meV   */
  void Peak::setInitialEnergy(double m_InitialEnergy)
  {    this->m_InitialEnergy = m_InitialEnergy;  }


  // -------------------------------------------------------------------------------------
  /** Get the goniometer rotation matrix at which this peak was measured. */
  Mantid::Geometry::Matrix<double> Peak::getGoniometerMatrix() const
  {
    return this->m_GoniometerMatrix;
  }

  /** Set the goniometer rotation matrix at which this peak was measured.
   * @throw std::invalid_argument if matrix is not 3x3*/
  void Peak::setGoniometerMatrix(Mantid::Geometry::Matrix<double> goniometerMatrix)
  {
    if ((goniometerMatrix.numCols() != 3) || (goniometerMatrix.numRows() != 3))
      throw std::invalid_argument("Goniometer matrix must be 3x3.");
    this->m_GoniometerMatrix = goniometerMatrix;
    // Calc the inverse rotation matrix
    m_InverseGoniometerMatrix = m_GoniometerMatrix;
    m_InverseGoniometerMatrix.Invert();
  }


  // -------------------------------------------------------------------------------------
  /** Find the name of the bank that is the parent of the detector. This works
   * best for RectangularDetector instruments (goes up two levels)
   * @return name of the bank.
   */
  std::string Peak::getBankName() const
  {
    return m_BankName;
  }

  // -------------------------------------------------------------------------------------
  /** For RectangularDetectors only, returns the row (y) of the pixel of the detector.
  * Returns -1 if it could not find it. */
  int Peak::getRow() const
  {   return m_Row;  }

  // -------------------------------------------------------------------------------------
  /** For RectangularDetectors only, returns the column (x) of the pixel of the detector.
   * Returns -1 if it could not find it. */
  int Peak::getCol() const
  {   return m_Col;  }

  // -------------------------------------------------------------------------------------
  /** Return the detector position vector */
  Mantid::Geometry::V3D Peak::getDetPos() const
  {
    return detPos;
  }

  // -------------------------------------------------------------------------------------
  /** Return the L1 flight path length (source to sample), in meters. */
  double Peak::getL1() const
  {
    return (samplePos - sourcePos).norm();
  }

  // -------------------------------------------------------------------------------------
  /** Return the L2 flight path length (sample to detector), in meters. */
  double Peak::getL2() const
  {
    return (detPos - samplePos).norm();
  }

} // namespace Mantid
} // namespace DataObjects

