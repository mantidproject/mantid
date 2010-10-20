#include "MantidAlgorithms/DetectorEfficiencyCor.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAlgorithms/InputWSDetectorInfo.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/Instrument/ParametrizedComponent.h"
#include "MantidKernel/PhysicalConstants.h"
#include <algorithm>
#include <functional>
#include <cmath>

namespace Mantid
{
namespace Algorithms
{
// Register the class into the algorithm factory
DECLARE_ALGORITHM(DetectorEfficiencyCor)

using namespace Kernel;
using namespace API;
using namespace Geometry;

namespace
{

// E = KSquaredToE*K^2    KSquaredToE = (hbar^2)/(2*NeutronMass) 
const double KSquaredToE = 2.07212466; // units of meV Angstrom^-2

const short  NUMCOEFS = 25;
// series expansion coefficients copied from a fortran source code file
const double c_eff_f[] =
  {0.7648360390553052,     -0.3700950778935237,
   0.1582704090813516,     -6.0170218669705407E-02,  2.0465515957968953E-02,
  -6.2690181465706840E-03,  1.7408667184745830E-03, -4.4101378999425122E-04,
   1.0252117967127217E-04, -2.1988904738111659E-05,  4.3729347905629990E-06,
  -8.0998753944849788E-07,  1.4031240949230472E-07, -2.2815971698619819E-08,
   3.4943984983382137E-09, -5.0562696807254781E-10,  6.9315483353094009E-11,
  -9.0261598195695569E-12,  1.1192324844699897E-12, -1.3204992654891612E-13,
   1.4100387524251801E-14, -8.6430862467068437E-16, -1.1129985821867194E-16,
  -4.5505266221823604E-16,  3.8885561437496108E-16};

const double c_eff_g[] ={2.033429926215546,
                  -2.3123407369310212E-02, 7.0671915734894875E-03,
                  -7.5970017538257162E-04, 7.4848652541832373E-05,
                   4.5642679186460588E-05,-2.3097291253000307E-05,
                   1.9697221715275770E-06, 2.4115259271262346E-06,
                  -7.1302220919333692E-07,-2.5124427621592282E-07,
                   1.3246884875139919E-07, 3.4364196805913849E-08,
                  -2.2891359549026546E-08,-6.7281240212491156E-09,
                   3.8292458615085678E-09, 1.6451021034313840E-09,
                  -5.5868962123284405E-10,-4.2052310689211225E-10,
                   4.3217612266666094E-11, 9.9547699528024225E-11,
                   1.2882834243832519E-11,-1.9103066351000564E-11,
                  -7.6805495297094239E-12, 1.8568853399347773E-12};

// constants from the fortran code multiplied together sigref=143.23d0, wref=3.49416d0, atmref=10.0d0 const = 2.0*sigref*wref/atmref
const double g_helium_prefactor = 2.0*143.23*3.49416/10.0;

// this should be a big number but not so big that there are rounding errors
const double DIST_TO_UNIVERSE_EDGE = 1e3;

}

// this default constructor calls default constructors and sets other member data to imposible (flag) values 
DetectorEfficiencyCor::DetectorEfficiencyCor() : 
  Algorithm(), m_inputWS(),m_outputWS(), m_paraMap(NULL), m_Ei(-1.0), m_ki(-1.0),
  m_shapeCache(), m_samplePos(), m_spectraSkipped()
  {
    m_shapeCache.clear();
  }

/**
 * Declare algorithm properties 
 */
void DetectorEfficiencyCor::init()
{
  CompositeValidator<> *val = new CompositeValidator<>;
  val->add(new WorkspaceUnitValidator<>("DeltaE"));
  val->add(new HistogramValidator<>);
  declareProperty(
    new WorkspaceProperty<>("InputWorkspace", "", Direction::Input, val),
    "The workspace to correct for detector efficiency");
  declareProperty(
    new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
    "The name of the workspace in which to store the result" );
  BoundedValidator<double> *checkEi = new BoundedValidator<double>();
  checkEi->setLower(0.0);
  checkEi->setUpper(1e4);
  declareProperty("IncidentEnergy", -1.0, checkEi,
    "The energy kinetic the neutrons have before they hit the sample (meV)" );
}

/** Executes the algorithm
*  @throw NullPointerException if a getDetector() returns NULL or pressure or wall thickness is not set
*  @throw invalid_argument if the shape of a detector is isn't a cylinder aligned on axis or there is no baseInstrument
*  @throw runtime_error if the SpectraDetectorMap had not been filled
*/
void DetectorEfficiencyCor::exec()
{
  //gets and checks the values passed to the algorithm
  retrieveProperties();

  // wave number that the neutrons originally had
  m_ki = std::sqrt(m_Ei/KSquaredToE);
  
  // Store some information about the instrument setup that will not change
  m_samplePos = m_inputWS->getInstrument()->getSample()->getPos();

  int numHists = m_inputWS->getNumberHistograms();
  const int progStep = static_cast<int>(ceil(numHists/100.0));

  PARALLEL_FOR2(m_inputWS,m_outputWS)
  for (int i = 0; i < numHists; ++i )
  {
    PARALLEL_START_INTERUPT_REGION
      
    m_outputWS->setX(i, m_inputWS->refX(i));
    try
    { 
      correctForEfficiency(i);
    }
    catch (Exception::NotFoundError &)
    {
      // if we don't have all the data there will be spectra we can't correct, avoid leaving the workspace part corrected 
      MantidVec& dud = m_outputWS->dataY(i);
      std::transform(dud.begin(),dud.end(),dud.begin(), std::bind2nd(std::multiplies<double>(),0));
      PARALLEL_CRITICAL(deteff_invalid)
      {
	      m_spectraSkipped.push_back(m_inputWS->getAxis(1)->spectraNo(i));
      }
    }      

    // make regular progress reports and check for cancelling the algorithm
    if ( i % progStep == 0 )
    {
      progress(static_cast<double>(i)/numHists);
      interruption_point();
    }

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  logErrors();
  setProperty("OutputWorkspace", m_outputWS);
}
/** Loads and checks the values passed to the algorithm
*
*  @throw invalid_argument if there is an incapatible property value so the algorithm can't continue
*/
void DetectorEfficiencyCor::retrieveProperties()
{
  // these first three properties are fully checked by validators
  m_inputWS = getProperty("InputWorkspace");
  m_paraMap = &(m_inputWS->instrumentParameters());

  m_Ei = getProperty("IncidentEnergy");

  m_outputWS = getProperty("OutputWorkspace");
  // If input and output workspaces are not the same, create a new workspace for the output
  if (m_outputWS != m_inputWS )
  {
    m_outputWS = WorkspaceFactory::Instance().create(m_inputWS);
  }
}

/** Corrects a spectra for the detector efficiency calculated from detector information
Gets the detector information and uses this to calculate its efficiency
*  @param spectraIn index of the spectrum to get the efficiency for
*  @throw invalid_argument if the shape of a detector is isn't a cylinder aligned along one axis
*  @throw runtime_error if the SpectraDetectorMap has not been filled
*  @throw NotFoundError if the detector or its gas pressure or wall thickness were not found
*/
void DetectorEfficiencyCor::correctForEfficiency(int spectraIn)
{
  IDetector_sptr det = m_inputWS->getDetector(spectraIn);
  if( det->isMonitor() || det->isMasked() )
  {
    return;
  }

  MantidVec & yout = m_outputWS->dataY(spectraIn);
  MantidVec & eout = m_outputWS->dataE(spectraIn);
  // Need the original values so this is not a reference
  const MantidVec yValues = m_inputWS->readY(spectraIn);
  const MantidVec eValues = m_inputWS->readE(spectraIn);

  // get a pointer to the detectors that created the spectrum
  const int specNum = m_inputWS->getAxis(1)->spectraNo(spectraIn);
  const std::vector<int> dets = m_inputWS->spectraMap().getDetectors(specNum);
  const double avgKi = m_ki/dets.size();

  std::vector<int>::const_iterator it = dets.begin();
  std::vector<int>::const_iterator iend = dets.end();
  if ( it == iend )
  {
    throw Exception::NotFoundError("No detectors found", spectraIn);
  }
  
  // Storage for the reciprocal wave vectors that are calculated as the 
  std::vector<double> oneOverWaveVectors(yValues.size());
  for( ; it != iend ; ++it )
  {
    IDetector_sptr det_member = m_inputWS->getInstrument()->getDetector(*it);
    
    Parameter_sptr par = m_paraMap->get(det_member.get(),"3He(atm)");
    if ( !par )
    {
      throw Exception::NotFoundError("3He(atm)", spectraIn);
    }
    const double atms = par->value<double>();
    par = m_paraMap->get(det_member.get(),"wallT(m)");
    if ( !par )
    {
      throw Exception::NotFoundError("wallT(m)", spectraIn);
    }
    const double wallThickness = par->value<double>();
    double detRadius(0.0);
    V3D detAxis;
    getDetectorGeometry(det_member, detRadius, detAxis);

   // now get the sin of the angle, it's the magnitude of the cross product of unit vector along the detector tube axis and a unit vector directed from the sample to the detector centre
    V3D vectorFromSample = det_member->getPos() - m_samplePos;
    vectorFromSample.normalize();
    Quat rot = det_member->getRotation();
    // rotate the original cylinder object axis to get the detector axis in the actual instrument
    rot.rotate(detAxis); 
    detAxis.normalize();
    // Scalar product is quicker than cross product
    double cosTheta = detAxis.scalar_prod(vectorFromSample);
    double sinTheta = std::sqrt(1.0 - cosTheta*cosTheta);
    // Detector constant
    const double det_const = g_helium_prefactor*(detRadius - wallThickness)*atms/sinTheta;

    std::vector<double>::const_iterator yinItr = yValues.begin();
    std::vector<double>::const_iterator einItr = eValues.begin();
    MantidVec::iterator youtItr = yout.begin();
    MantidVec::iterator eoutItr = eout.begin();
    MantidVec::const_iterator xItr = m_inputWS->readX(spectraIn).begin();
    std::vector<double>::iterator wavItr = oneOverWaveVectors.begin();

    for( ; youtItr != yout.end(); ++youtItr, ++eoutItr)
    {
      if( it == dets.begin() )
      {
	      *youtItr = 0.0;
	      *eoutItr = 0.0;
	      *wavItr = calculateOneOverK(*xItr, *(xItr + 1 ));
      }
      const double oneOverWave = *wavItr;
      const double correcti = avgKi*oneOverWave/detectorEfficiency(det_const*oneOverWave);
      *youtItr += (*yinItr)*correcti;
      *eoutItr += (*einItr)*correcti;
      ++yinItr; ++einItr;
      ++xItr; ++wavItr;
    }
  }
}

/**
 * Calculates one over the wave number of a neutron based on a lower and upper bin boundary
 * @param loBinBound A value interpreted as the lower bin bound of a histogram
 * @param uppBinBound A value interpreted as the upper bin bound of a histogram
 * @return The value of 1/K for this energy bin
 */
double DetectorEfficiencyCor::calculateOneOverK(double loBinBound, double uppBinBound) const
{
  double energy = m_Ei - 0.5*(uppBinBound + loBinBound);
  double oneOverKSquared = KSquaredToE/energy;
  return std::sqrt(oneOverKSquared);
}

/** Update the shape cache if necessary
* @param det a pointer to the detector to query
* @param detRadius An output paramater that contains the detector radius
* @param detAxis An output parameter that contains the detector axis vector
*/
void DetectorEfficiencyCor::getDetectorGeometry(boost::shared_ptr<Geometry::IDetector> det, double & detRadius, V3D & detAxis)
{
  boost::shared_ptr<const Object> shape_sptr = det->Shape();
  std::map<const Geometry::Object *, std::pair<double, Geometry::V3D> >::const_iterator it = 
    m_shapeCache.find(shape_sptr.get());
  if( it == m_shapeCache.end() )
  {
    double xDist = distToSurface( V3D(DIST_TO_UNIVERSE_EDGE, 0, 0), shape_sptr.get() );
    double zDist = distToSurface( V3D(0, 0, DIST_TO_UNIVERSE_EDGE), shape_sptr.get() );
    if ( std::abs(zDist - xDist) < 1e-8 )
    {
      detRadius = zDist/2.0;
      detAxis = V3D(0,1,0);
      // assume radi in z and x and the axis is in the y
      PARALLEL_CRITICAL(deteff_shapecachea)
      {
        m_shapeCache.insert(std::pair<const Object *,std::pair<double, V3D> >(shape_sptr.get(), std::pair<double, V3D>(detRadius, detAxis)));
      }
      return;
    }
    double yDist = distToSurface( V3D(0, DIST_TO_UNIVERSE_EDGE, 0), shape_sptr.get() );
    if ( std::abs(yDist - zDist) < 1e-8 )
    {
      detRadius = yDist/2.0;
      detAxis = V3D(1,0,0);
      // assume that y and z are radi of the cylinder's circular cross-section and the axis is perpendicular, in the x direction
      PARALLEL_CRITICAL(deteff_shapecacheb)
      {
        m_shapeCache.insert(std::pair<const Object *,std::pair<double, V3D> >(shape_sptr.get(), std::pair<double, V3D>(detRadius, detAxis)));
      }
      return;
    }
    
    if ( std::abs(xDist - yDist) < 1e-8 )
    {
      detRadius = xDist/2.0;
      detAxis = V3D(0,0,1);
      PARALLEL_CRITICAL(deteff_shapecachec)
      {
        m_shapeCache.insert(std::pair<const Object *,std::pair<double, V3D> >(shape_sptr.get(), std::pair<double, V3D>(detRadius, detAxis)));
      }
      return;
    }
  }
  else
  {
    std::pair<double, V3D> geometry = it->second;
    detRadius = it->second.first;
    detAxis = it->second.second;
  }
}

/** For basic shapes centred on the origin (0,0,0) this returns the distance to the surface in
 *  the direction of the point given
 *  @param start the distance calculated from origin to the surface in a line towards this point. It should be outside the shape
 *  @param shape the object to calculate for, should be centred on the origin
 *  @return the distance to the surface in the direction of the point given
 *  @throw invalid_argument if there is any error finding the distance
 * @returns The distance to the surface in metres
 */
double DetectorEfficiencyCor::distToSurface(const V3D start, const Object *shape) const
{  
  // get a vector from the point that was passed to the origin
  V3D direction = V3D(0.0, 0.0, 0.0)-start;
  // it needs to be a unit vector
  direction.normalize();
  // put the point and the vector (direction) together to get a line, here called a track
  Track track( start, direction );
  // split the track (line) up into the part that is inside the shape and the part that is outside
  shape->interceptSurface(track);
    
  if ( track.count() != 1 )
  {// the track missed the shape, probably the shape is not centred on the origin
    throw std::invalid_argument("Fatal error interpreting the shape of a detector");
  }
  // the first part of the track will be the part inside the shape, return its length
  return track.begin()->distInsideObject;
}

/** Calculates detector efficiency, copied from the fortran code in effic_3he_cylinder.for
 *  @param alpha From T.G.Perring's effic_3he_cylinder.for: alpha = const*rad*(1.0d0-t2rad)*atms/wvec
 *  @return detector efficiency
 */
double DetectorEfficiencyCor::detectorEfficiency(const double alpha) const
{
  if ( alpha < 9.0 )
  {
    return (M_PI/4.0)*alpha*chebevApprox(0.0, 10.0, c_eff_f, alpha);
  }
  if ( alpha > 10.0 )
  {
    double y = 1.0 - 18.0/alpha;
    return 1.0 - chebevApprox(-1.0, 1.0, c_eff_g, y)/(alpha*alpha);
  }
  double eff_f =(M_PI/4.0)*alpha*chebevApprox(0.0, 10.0, c_eff_f, alpha);
  double y=1.0 - 18.0/alpha;
  double eff_g =1.0 - chebevApprox(-1.0,1.0,c_eff_g, y)/(alpha*alpha);
  return (10.0-alpha)*eff_f  + (alpha-9.0)*eff_g;
}

/** Calculates an expansion similar to that in CHEBEV of "Numerical Recipes"
*  copied from the fortran code in effic_3he_cylinder.for
* @param a a fit parameter, only the difference between a and b enters this equation
* @param b a fit parameter, only the difference between a and b enters this equation 
* @param exspansionCoefs one of the 25 element constant arrays declared in this file
* @param x a fit parameter
* @return a numerical approximation provided by the expansion
*/
double DetectorEfficiencyCor::chebevApprox(double a, double b, const double exspansionCoefs[], double x) const
{
  double d=0.0;
  double dd=0.0;
  double y=(2.0*x-a-b)/(b-a);
  double y2=2.0*y;
  for ( int j=NUMCOEFS-1; j > 0; j-=1 )
  {
    double sv=d;
    d=y2*d-dd+exspansionCoefs[j];
    dd=sv;
  }
  return y*d-dd+0.5*exspansionCoefs[0];
}

/** 
 * Logs if there were any problems locating spectra.
*/
void DetectorEfficiencyCor::logErrors() const
{
  std::vector<int>::size_type nspecs = m_spectraSkipped.size();
  if( nspecs > 0 )
  {
    g_log.warning() << "There were " <<  nspecs << " spectra that could not be corrected. ";
    g_log.debug() << "Unaffected spectra numbers: ";
    for( size_t i = 0; i < nspecs; ++i )
    {
      g_log.debug() << m_spectraSkipped[i] << " ";
    }
    g_log.debug() << "\n";
  }
}

} // namespace Algorithm
} // namespace Mantid
