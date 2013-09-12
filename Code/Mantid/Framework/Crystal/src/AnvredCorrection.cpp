/*WIKI* 

Following A.J.Schultz's anvred, the weight factors should be:
sin^2(theta) / (lamda^4 * spec * eff * trans)
where theta = scattering_angle/2
lamda = wavelength (in angstroms?)
spec  = incident spectrum correction
eff   = pixel efficiency
trans = absorption correction
The quantity:
sin^2(theta) / eff
depends only on the pixel and can be pre-calculated
for each pixel.  It could be saved in array pix_weight[].
For now, pix_weight[] is calculated by the method:
BuildPixWeights() and just holds the sin^2(theta) values.
The wavelength dependent portion of the correction is saved in
the array lamda_weight[].
The time-of-flight is converted to wave length by multiplying
by tof_to_lamda[id], then (int)STEPS_PER_ANGSTROM * lamda
gives an index into the table lamda_weight[].
The lamda_weight[] array contains values like:
1/(lamda^power * spec(lamda))
which are pre-calculated for each lamda.  These values are
saved in the array lamda_weight[].  The optimal value to use
for the power should be determined when a good incident spectrum
has been determined.  Currently, power=3 when used with an
incident spectrum and power=2.4 when used without an incident
spectrum.
The pixel efficiency and incident spectrum correction are NOT CURRENTLY USED.
The absorption correction, trans, depends on both lamda and the pixel,
Which is a fairly expensive calulation when done for each event.
--


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCrystal/AnvredCorrection.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Fast_Exponential.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/V3D.h"
#include "MantidAPI/MemoryManager.h"

/*  Following A.J.Schultz's anvred, the weight factors should be:
 * 
 *    sin^2(theta) / (lamda^4 * spec * eff * trans)
 *
 *  where theta = scattering_angle/2
 *        lamda = wavelength (in angstroms?)
 *        spec  = incident spectrum correction
 *        eff   = pixel efficiency
 *        trans = absorption correction
 *  
 *  The quantity:
 *
 *    sin^2(theta) / eff 
 *
 *  depends only on the pixel and can be pre-calculated 
 *  for each pixel.  It could be saved in array pix_weight[].
 *  For now, pix_weight[] is calculated by the method:
 *  BuildPixWeights() and just holds the sin^2(theta) values.
 *
 *  The wavelength dependent portion of the correction is saved in
 *  the array lamda_weight[].
 *  The time-of-flight is converted to wave length by multiplying
 *  by tof_to_lamda[id], then (int)STEPS_PER_ANGSTROM * lamda
 *  gives an index into the table lamda_weight[].
 *
 *  The lamda_weight[] array contains values like:
 *
 *      1/(lamda^power * spec(lamda))
 *   
 *  which are pre-calculated for each lamda.  These values are
 *  saved in the array lamda_weight[].  The optimal value to use
 *  for the power should be determined when a good incident spectrum
 *  has been determined.  Currently, power=3 when used with an 
 *  incident spectrum and power=2.4 when used without an incident
 *  spectrum.
 *
 *  The pixel efficiency and incident spectrum correction are NOT CURRENTLY USED.
 *  The absorption correction, trans, depends on both lamda and the pixel,
 *  Which is a fairly expensive calulation when done for each event.
 */

namespace Mantid
{
namespace Crystal
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(AnvredCorrection)

using namespace Kernel;
using namespace Geometry;
using namespace API;
using namespace DataObjects;
using namespace Mantid::PhysicalConstants;

AnvredCorrection::AnvredCorrection() : API::Algorithm()
{}

void AnvredCorrection::init()
{
  this->setWikiSummary("Calculates anvred correction factors for attenuation due to absorption and scattering in a spherical sample");
  // The input workspace must have an instrument and units of wavelength
  boost::shared_ptr<CompositeValidator> wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add(boost::make_shared<InstrumentValidator>());

  declareProperty(new WorkspaceProperty<> ("InputWorkspace", "", Direction::Input,wsValidator),
    "The X values for the input workspace must be in units of wavelength or TOF");
  declareProperty(new WorkspaceProperty<> ("OutputWorkspace", "", Direction::Output),
    "Output workspace name");

  auto mustBePositive = boost::make_shared<BoundedValidator<double> >();
  mustBePositive->setLower(0.0);
  declareProperty("LinearScatteringCoef", EMPTY_DBL(), mustBePositive,
    "Linear scattering coefficient in 1/cm if not set with SetSampleMaterial");
  declareProperty("LinearAbsorptionCoef", EMPTY_DBL(), mustBePositive,
    "Linear absorption coefficient at 1.8 Angstroms in 1/cm if not set with SetSampleMaterial");
  declareProperty("Radius", EMPTY_DBL(), mustBePositive, "Radius of the sample in centimeters");
  declareProperty("PreserveEvents", true, "Keep the output workspace as an EventWorkspace, if the input has events (default).\n"
     "If false, then the workspace gets converted to a Workspace2D histogram.");
  declareProperty("OnlySphericalAbsorption", false, "All corrections done if false (default).\n"
     "If true, only the spherical absorption correction.");
  declareProperty("ReturnTransmissionOnly", false, "Corrections applied to data if false (default).\n"
     "If true, only return the transmission coefficient.");
  declareProperty("PowerLambda", 4.0,
    "Power of lamda ");


  defineProperties();
}

void AnvredCorrection::exec()
{
  // Retrieve the input workspace
  m_inputWS = getProperty("InputWorkspace");
  OnlySphericalAbsorption = getProperty("OnlySphericalAbsorption");
  ReturnTransmissionOnly = getProperty("ReturnTransmissionOnly");
  if (!OnlySphericalAbsorption)
  {
    const API::Run & run = m_inputWS->run();
    if ( run.hasProperty("LorentzCorrection") )
    {
      Kernel::Property* prop = run.getProperty("LorentzCorrection");
      bool lorentzDone = boost::lexical_cast<bool,std::string>(prop->value());
      if(lorentzDone)
      {
         OnlySphericalAbsorption = true;
         g_log.warning()<<"Lorentz Correction was already done for this workspace.  OnlySphericalAbsorption was changed to true." << std::endl;
      }
    }
  }

  std::string unitStr = m_inputWS->getAxis(0)->unit()->unitID();

  // Get the input parameters
  retrieveBaseProperties();

  BuildLamdaWeights();

  eventW = boost::dynamic_pointer_cast<EventWorkspace>( m_inputWS );
  if(eventW)eventW->sortAll(TOF_SORT, NULL);
  if ((getProperty("PreserveEvents")) && (eventW != NULL) && !ReturnTransmissionOnly)
  {
    //Input workspace is an event workspace. Use the other exec method
    this->execEvent();
    this->cleanup();
    return;
  }

  MatrixWorkspace_sptr correctionFactors = WorkspaceFactory::Instance().create(m_inputWS);

  const int64_t numHists = static_cast<int64_t>(m_inputWS->getNumberHistograms());
  const int64_t specSize = static_cast<int64_t>(m_inputWS->blocksize());
  if (specSize < 3) throw std::runtime_error("Problem in AnvredCorrection::events not binned");

  const bool isHist = m_inputWS->isHistogramData();

  // If sample not at origin, shift cached positions.
  const V3D samplePos = m_inputWS->getInstrument()->getSample()->getPos();
  const V3D pos = m_inputWS->getInstrument()->getSource()->getPos()-samplePos;
  double L1 = pos.norm();

  Progress prog(this,0.0,1.0,numHists);
  // Loop over the spectra
  PARALLEL_FOR2(m_inputWS,correctionFactors)
  for (int64_t i = 0; i < int64_t(numHists); ++i)
  {
    PARALLEL_START_INTERUPT_REGION

    // Get a reference to the Y's in the output WS for storing the factors
    MantidVec& Y = correctionFactors->dataY(i);
    MantidVec& E = correctionFactors->dataE(i);

    // Copy over bin boundaries
    const ISpectrum * inSpec = m_inputWS->getSpectrum(i);
    inSpec->lockData(); // for MRU-related thread safety

    const MantidVec& Xin = inSpec->readX();
    correctionFactors->dataX(i) = Xin;
    const MantidVec & Yin = inSpec->readY();
    const MantidVec & Ein = inSpec->readE();

    // Get detector position
    IDetector_const_sptr det;
    try
    {
      det = m_inputWS->getDetector(i);
    } catch (Exception::NotFoundError&)
    {
      // Catch if no detector. Next line tests whether this happened - test placed
      // outside here because Mac Intel compiler doesn't like 'continue' in a catch
      // in an openmp block.
    }
    // If no detector found, skip onto the next spectrum
    if ( !det ) continue;

    // This is the scattered beam direction
    Instrument_const_sptr inst = m_inputWS->getInstrument();
    V3D dir = det->getPos() - samplePos;
    double L2 = dir.norm();
    // Two-theta = polar angle = scattering angle = between +Z vector and the scattered beam
    double scattering = dir.angle( V3D(0.0, 0.0, 1.0) );

    Mantid::Kernel::Units::Wavelength wl;
    std::vector<double> timeflight;

    // Loop through the bins in the current spectrum
    for (int64_t j = 0; j < specSize; j++)
    {
      timeflight.push_back((isHist ? (0.5 * (Xin[j] + Xin[j + 1])) : Xin[j]));
      if (unitStr.compare("TOF") == 0)
        wl.fromTOF(timeflight, timeflight, L1, L2, scattering, 0, 0, 0);
      const double lambda = timeflight[0];
      timeflight.clear();

      if (ReturnTransmissionOnly)
      {
        Y[j] = 1.0 / this->getEventWeight(lambda, scattering);
      }
      else
      {
        double value = this->getEventWeight(lambda, scattering);
        Y[j] = Yin[j] * value;
        E[j] = Ein[j] * value;
      }

    }

    inSpec->unlockData();

    prog.report();

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // set the absorption correction values in the run parameters
  API::Run & run = correctionFactors->mutableRun();
  run.addProperty<double>("Radius", radius, true);
  if (!OnlySphericalAbsorption && !ReturnTransmissionOnly)
    run.addProperty<bool>("LorentzCorrection", 1, true);
  setProperty("OutputWorkspace", correctionFactors);

}

void AnvredCorrection::cleanup()
{
  //Clear vectors to free up memory.
  lamda_weight.clear();
}

void AnvredCorrection::execEvent()
{

  const int64_t numHists = static_cast<int64_t>(m_inputWS->getNumberHistograms());
  std::string unitStr = m_inputWS->getAxis(0)->unit()->unitID();
  //Create a new outputworkspace with not much in it
  DataObjects::EventWorkspace_sptr correctionFactors;
  correctionFactors = boost::dynamic_pointer_cast<EventWorkspace>(
      API::WorkspaceFactory::Instance().create("EventWorkspace",numHists,2,1) );
  correctionFactors->sortAll(TOF_SORT, NULL);
  //Copy required stuff from it
  API::WorkspaceFactory::Instance().initializeFromParent(m_inputWS, correctionFactors, true);
  bool inPlace = (this->getPropertyValue("InputWorkspace") == this->getPropertyValue("OutputWorkspace"));
  if (inPlace)
    g_log.debug("Correcting EventWorkspace in-place.");

  // If sample not at origin, shift cached positions.
  const V3D samplePos = m_inputWS->getInstrument()->getSample()->getPos();
  const V3D pos = m_inputWS->getInstrument()->getSource()->getPos()-samplePos;
  double L1 = pos.norm();

  Progress prog(this,0.0,1.0,numHists);
  // Loop over the spectra
  PARALLEL_FOR2(eventW,correctionFactors)
  for (int64_t i = 0; i < int64_t(numHists); ++i)
  {
    PARALLEL_START_INTERUPT_REGION

    // Copy over bin boundaries
    const MantidVec& X = eventW->readX(i);
    correctionFactors->dataX(i) = X;

    // Get detector position
    IDetector_const_sptr det;
    try
    {
      det = eventW->getDetector(i);
    } catch (Exception::NotFoundError&)
    {
      // Catch if no detector. Next line tests whether this happened - test placed
      // outside here because Mac Intel compiler doesn't like 'continue' in a catch
      // in an openmp block.
    }
    // If no detector found, skip onto the next spectrum
    if ( !det ) continue;

    // This is the scattered beam direction
    Instrument_const_sptr inst = eventW->getInstrument();
    V3D dir = det->getPos() - samplePos;
    double L2 = dir.norm();
    // Two-theta = polar angle = scattering angle = between +Z vector and the scattered beam
    double scattering = dir.angle( V3D(0.0, 0.0, 1.0) );

    EventList el = eventW->getEventList(i);
    el.switchTo(WEIGHTED_NOTIME);
    std::vector<WeightedEventNoTime> events = el.getWeightedEventsNoTime();

    std::vector<WeightedEventNoTime>::iterator itev;
    std::vector<WeightedEventNoTime>::iterator itev_end = events.end();

    Mantid::Kernel::Units::Wavelength wl;
    std::vector<double> timeflight;

    // multiplying an event list by a scalar value
    for (itev = events.begin(); itev != itev_end; ++itev)
    {
      timeflight.push_back(itev->tof());
      if (unitStr.compare("TOF") == 0)
        wl.fromTOF(timeflight, timeflight, L1, L2, scattering, 0, 0, 0);
      double value = this->getEventWeight(timeflight[0], scattering);
      timeflight.clear();
      itev->m_errorSquared = static_cast<float>(itev->m_errorSquared * value*value);
      itev->m_weight *= static_cast<float>(value);
    }
    correctionFactors->getOrAddEventList(i) +=events;
    
    std::set<detid_t>& dets = eventW->getEventList(i).getDetectorIDs();
    std::set<detid_t>::iterator j;
    for (j = dets.begin(); j != dets.end(); ++j)
      correctionFactors->getOrAddEventList(i).addDetectorID(*j);
    // When focussing in place, you can clear out old memory from the input one!
    if (inPlace)
    {
      eventW->getEventList(i).clear();
      Mantid::API::MemoryManager::Instance().releaseFreeMemory();
    }


    prog.report();

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // set the absorption correction values in the run parameters
  API::Run & run = correctionFactors->mutableRun();
  run.addProperty<double>("Radius", radius, true);
  if (!OnlySphericalAbsorption && !ReturnTransmissionOnly)
    run.addProperty<bool>("LorentzCorrection", 1, true);
  setProperty("OutputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(correctionFactors));

  // Now do some cleaning-up since destructor may not be called immediately
  this->cleanup();
}

/// Fetch the properties and set the appropriate member variables
void AnvredCorrection::retrieveBaseProperties()
{
  smu = getProperty("LinearScatteringCoef"); // in 1/cm
  amu = getProperty("LinearAbsorptionCoef"); // in 1/cm
  radius = getProperty("Radius"); // in cm
  power_th = getProperty("PowerLambda"); // in cm
  const Material *m_sampleMaterial = &(m_inputWS->sample().getMaterial());
  if( m_sampleMaterial->totalScatterXSection(NeutronAtom::ReferenceLambda) != 0.0)
  {
	double rho =  m_sampleMaterial->numberDensity();
	if(smu == EMPTY_DBL()) smu =  m_sampleMaterial->totalScatterXSection(NeutronAtom::ReferenceLambda) * rho;
	if(amu == EMPTY_DBL()) amu = m_sampleMaterial->absorbXSection(NeutronAtom::ReferenceLambda) * rho;
  }
  else  //Save input in Sample with wrong atomic number and name
  {
	NeutronAtom *neutron = new NeutronAtom(static_cast<uint16_t>(EMPTY_DBL()), static_cast<uint16_t>(0),
  			0.0, 0.0, smu, 0.0, smu, amu);
    Material *mat = new Material("SetInAnvredCorrection", *neutron, 1.0);
    m_inputWS->mutableSample().setMaterial(*mat);
  }
  // Call the virtual function for any further properties
  retrieveProperties();
}

 /**
  *  Get the weight factor that would be used for an event occuring 
  *  at the specified wavelength, with the specified two_theta value.
  *
  *  @param  lamda      The wavelength of an event.
  *  @param  two_theta  The scattering angle of the event.
  *
  *  @return The weight factor for the specified position and wavelength.
  */
double AnvredCorrection::getEventWeight( double lamda, double two_theta )
  {
    double transinv = 1;
    if ( radius > 0 )
       transinv = absor_sphere(two_theta, lamda);
    // Only Spherical absorption correction 
    if (OnlySphericalAbsorption || ReturnTransmissionOnly) return transinv;

    // Resolution of the lambda table
    size_t lamda_index = static_cast<size_t>( STEPS_PER_ANGSTROM * lamda );

    if ( lamda_index >= lamda_weight.size() )
      lamda_index = lamda_weight.size() - 1;

    double lamda_w     = lamda_weight[ lamda_index ];

    double sin_theta = std::sin( two_theta/2 );
    double pix_weight = sin_theta * sin_theta;

    double event_weight = pix_weight * lamda_w * transinv;

    return event_weight;
  }

 /**
  *       function to calculate a spherical absorption correction
  *       and tbar. based on values in:
  *
  *       c. w. dwiggins, jr., acta cryst. a31, 395 (1975).
  *
  *       in this paper, a is the transmission and a* = 1/a is
  *       the absorption correction.
  *
  *       input are the smu (scattering) and amu (absorption at 1.8 ang.)
  *       linear absorption coefficients, the radius r of the sample
  *       the theta angle and wavelength.
  *       the absorption (absn) and tbar are returned.
  *
  *       a. j. schultz, june, 2008
  *
  *       @param twoth scattering angle
  *       @param wl scattering wavelength
  *       @returns absorption
  */
double AnvredCorrection::absor_sphere(double& twoth, double& wl)
  {
    int i;
    double mu, mur;         //mu is the linear absorption coefficient,
                            //r is the radius of the spherical sample.
    double theta,astar1,astar2,frac,astar;
//  double trans;
//  double tbar;

//  For each of the 19 theta values in dwiggins (theta = 0.0 to 90.0
//  in steps of 5.0 deg.), the astar values vs.mur were fit to a third
//  order polynomial in excel. these values are given in the static array
//  pc[][]

    mu = smu + (amu/1.8f)*wl;

    mur = mu*radius;
    if (mur < 0. || mur > 2.5)
    {
      std::ostringstream s;
      s << mur;
      throw std::runtime_error("muR is not in range of Dwiggins' table :" + s.str());
    }

    theta = twoth*radtodeg_half;
    if (theta < 0. || theta > 90.)
    {
      std::ostringstream s;
      s << theta;
      throw std::runtime_error("theta is not in range of Dwiggins' table :" + s.str());
    }

//  using the polymial coefficients, calulate astar (= 1/transmission) at
//  theta values below and above the actual theta value.

    i = (int)(theta/5.);
    astar1 = pc[0][i] + mur * (pc[1][i] + mur * (pc[2][i] + pc[3][i] * mur));

    i = i+1;
    astar2 = pc[0][i] + mur * (pc[1][i] + mur * (pc[2][i] + pc[3][i] * mur));

//  do a linear interpolation between theta values.

    frac = theta - static_cast<double>(static_cast<int>( theta / 5. )) * 5.;//theta%5.
    frac = frac/5.;

    astar = astar1*(1-frac) + astar2*frac;       // astar is the correction
//  trans = 1.f/astar;                           // trans is the transmission
                                                 // trans = exp(-mu*tbar)

//  calculate tbar as defined by coppens.
//  tbar = -(double)Math.log(trans)/mu;

    return astar;
  }
  /**
   *  Build the list of weights corresponding to different wavelengths.
   *  Although the spectrum file need not have a fixed number of
   *  points, it MUST have the spectrum recorded as a histogram with one
   *  more bin boundary than the number of bins.
   *    The entries in the table produced are:
   *
   *     1/( lamda^power * spec(lamda) )
   *
   *  Where power was chosen to give a relatively uniform intensity display
   *  in 3D.  The power is currently 3 if an incident spectrum is present
   *  and 2.4 if no incident spectrum is used.
   */
  void AnvredCorrection::BuildLamdaWeights()
  {
                                             // Theoretically correct value 3.0;
                                             // if we have an incident spectrum
//  double power_ns = 2.4;                   // lower power needed to find
                                             // peaks in ARCS data with no
                                             // incident spectrum

    double power = power_th;

    //GetSpectrumWeights( spectrum_file_name, lamda_weight);

    if ( lamda_weight.size() == 0 )              // loading spectrum failed so use
    {                                        // array of 1's
//    power = power_ns;                      // This is commented out, so we
                                             // don't override user specified 
                                             // value.
      lamda_weight.reserve(NUM_WAVELENGTHS);
      for ( int i = 0; i < NUM_WAVELENGTHS; i++ )
        lamda_weight.push_back(1.);
    }

    for ( size_t i = 0; i < lamda_weight.size(); i++ )
    {
      double lamda = static_cast<double>(i) / STEPS_PER_ANGSTROM;
      lamda_weight[i] *= (double)(1/std::pow(lamda,power));
    }

  }


} // namespace Crystal
} // namespace Mantid
