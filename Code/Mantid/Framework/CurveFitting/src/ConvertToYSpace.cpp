/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidCurveFitting/ConvertToYSpace.h"
#include "MantidCurveFitting/ComptonProfile.h"

#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/BoundedValidator.h"

#include <boost/make_shared.hpp>

namespace Mantid
{
namespace CurveFitting
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(ConvertToYSpace);
  
  using namespace API;
  using namespace Kernel;

  namespace
  {
    /// Conversion constant
    const double MASS_TO_MEV = 0.5*PhysicalConstants::NeutronMass/PhysicalConstants::meV;
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ConvertToYSpace::ConvertToYSpace()
    : Algorithm(), m_inputWS(), m_l1(0.0), m_samplePos(),m_outputWS()
  {
  }
    

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string ConvertToYSpace::name() const { return "ConvertToYSpace";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int ConvertToYSpace::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string ConvertToYSpace::category() const { return "Transforms\\Units";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void ConvertToYSpace::initDocs()
  {
    this->setWikiSummary("Converts in workspace in units of TOF to Y-space as defined in Compton scattering field");
    this->setOptionalMessage("Converts in workspace in units of TOF to Y-space as defined in Compton scattering field");
  }

  //----------------------------------------------------------------------------------------------
  /**
   * @param yspace Output yspace value
   * @param qspace Output qspace value
   * @param ei Output incident energy value
   * @param mass Mass value for the transformation
   * @param tmicro Time-of-flight in microseconds
   * @param k1 Modulus of wavevector for final energy (sqrt(efixed/massToMeV)), avoids repeated calculation
   * @param v1 Velocity of neutron for final energy (sqrt(efixed/massToMeV)), avoids repeated calculation
   * @param detpar Struct defining Detector parameters @see ComptonProfile
   */
  void ConvertToYSpace::calculateY(double & yspace, double & qspace, double &ei,
                                   const double mass, const double tmicro,
                                   const double k1, const double v1,
                                   const DetectorParams & detpar)
  {

    const double tsec = tmicro*1e-06;
    const double v0 = detpar.l1/(tsec - detpar.t0 - (detpar.l2/v1));
    ei = MASS_TO_MEV*v0*v0;
    const double w = ei - detpar.efixed;
    const double k0 = std::sqrt(ei/PhysicalConstants::E_mev_toNeutronWavenumberSq);
    qspace = std::sqrt(k0*k0 + k1*k1 - 2.0*k0*k1*std::cos(detpar.theta));
    const double wreduced = PhysicalConstants::E_mev_toNeutronWavenumberSq*qspace*qspace/mass;
    yspace = 0.2393*(mass/qspace)*(w - wreduced);
  }

  //----------------------------------------------------------------------------------------------


  /** Initialize the algorithm's properties.
   */
  void ConvertToYSpace::init()
  {
    auto wsValidator = boost::make_shared<CompositeValidator>();
    wsValidator->add<WorkspaceUnitValidator>("TOF");
    wsValidator->add<InstrumentValidator>();
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,wsValidator),
                    "An input workspace.");

    auto mustBePositive = boost::make_shared<BoundedValidator<double> >();
    mustBePositive->setLower(0.0);
    mustBePositive->setLowerExclusive(true); //strictly greater than 0.0
    declareProperty("Mass",-1.0,mustBePositive,"The mass defining the recoil peak in AMU");

    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace.");

  }
  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void ConvertToYSpace::exec()
  {
    retrieveInputs();
    createOutputWorkspace();
  }

  /**
   * Caches input details for the peak information
   */
  void ConvertToYSpace::retrieveInputs()
  {
    m_inputWS = getProperty("InputWorkspace");
    cacheInstrumentGeometry();
  }


  /**
   * Create & cache output workspaces
   */
  void ConvertToYSpace::createOutputWorkspace()
  {
    m_outputWS = WorkspaceFactory::Instance().create(m_inputWS);
  }

  /**
   */
  void ConvertToYSpace::cacheInstrumentGeometry()
  {
    auto inst = m_inputWS->getInstrument();
    auto source = inst->getSource();
    auto sample = inst->getSample();
    m_samplePos = sample->getPos();
    m_l1 = m_samplePos.distance(source->getPos());
  }


} // namespace CurveFitting
} // namespace Mantid
