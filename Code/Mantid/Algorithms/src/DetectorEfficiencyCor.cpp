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
using namespace DataObjects;

// E = KSquaredToE*K^2    KSquaredToE = (hbar^2)/(2*NeutronMass) 
const double DetectorEfficiencyCor::KSquaredToE = 2.07212466;// units of meV Angsstrom^-2

const short  DetectorEfficiencyCor::NUMCOEFS = 25;
// series expansion coefficients copied from a fortran source code file
const double DetectorEfficiencyCor::c_eff_f[] =
  {0.7648360390553052,     -0.3700950778935237,
   0.1582704090813516,     -6.0170218669705407E-02,  2.0465515957968953E-02,
  -6.2690181465706840E-03,  1.7408667184745830E-03, -4.4101378999425122E-04,
   1.0252117967127217E-04, -2.1988904738111659E-05,  4.3729347905629990E-06,
  -8.0998753944849788E-07,  1.4031240949230472E-07, -2.2815971698619819E-08,
   3.4943984983382137E-09, -5.0562696807254781E-10,  6.9315483353094009E-11,
  -9.0261598195695569E-12,  1.1192324844699897E-12, -1.3204992654891612E-13,
   1.4100387524251801E-14, -8.6430862467068437E-16, -1.1129985821867194E-16,
  -4.5505266221823604E-16,  3.8885561437496108E-16};

const double DetectorEfficiencyCor::c_eff_g[] ={2.033429926215546,
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
const double DetectorEfficiencyCor::CONSTA = 2.0*143.23*3.49416/10.0;

// this should be a big number but not so big that there are rounding errors
const double DIST_TO_UNIVERSE_EDGE = 1e3;

// this default constructor calls default constructors and sets other member data to imposible (flag) values 
DetectorEfficiencyCor::DetectorEfficiencyCor() : Algorithm(), m_inputWS(),
  m_outputWS(), m_paraMap(NULL), m_detMasking(), m_Ei(-1.0), m_ki(-1.0),
  m_shapeCache(NULL), m_radCache(-1), m_sinThetaCache(-1e3), m_baseAxisCache(),
  m_1_t2rad(-1), m_CONST_rad_sintheta_1_t2rad_atms(-1), m_1_wvec(),
  m_XsCache(NULL)
  {
  }

void DetectorEfficiencyCor::init()
{// declare the input data this algorithm requires and declare what the pre-execute validation will be
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
  
  try
  {// load a helper object that will allow masking detectors and checking the masking status
    m_detMasking.reset( new InputWSDetectorInfo(m_inputWS) );
  }
  catch (std::invalid_argument &e)
  {
    g_log.debug() << "InputWSDetectorInfo::InputWSDetectorInfo() threw invalid_argument " << e.what();
    g_log.warning() << "Checking detector masking and masking detectors is disabled because of a problem loading the maksing map" << std::endl;
    m_detMasking.reset();
  }

  // save error logging information, we'll send it out as one block at the end because there could be a lottt
  std::vector<int> spuriousSpectra;
  std::vector<int> unsetParams;

  int numHists = m_inputWS->getNumberHistograms();
  const int progStep = static_cast<int>(ceil(numHists/30.0));
  for (int i = 0; i < numHists; ++i )
  {
    try
    {// check if a detector is masked, if so do nothing and the output workspace Y and E values will remain untouched as either zeros or the values in the input workspace
      if ( (! m_detMasking.get()) || (! m_detMasking->aDetecIsMaskedinSpec(i)))
      {
        efficiencyCorrect(i);
      }
    }
    catch (Exception::NotFoundError &excep)
    {// if we don't have all the data there will be spectra we can't correct, avoid leaving the workspace part corrected 
      MantidVec& dud = m_outputWS->dataY(i);
      std::transform(dud.begin(),dud.end(),dud.begin(), std::bind2nd(std::multiplies<double>(),0));

      //check on the reason for the error, mask the detectors if possible and log
      std::string error(excep.what());
      bool paramProb = error.find("3He(atm)") != std::string::npos;
      paramProb = paramProb || error.find("wallT(m)") != std::string::npos;
      if (paramProb)
      {// Couldn't get one of the properties for a detector, I don't why that would be but zero the spectra and tell the user
        unsetParams.push_back(i);
        // mark the detectors as masked
        m_detMasking->maskAllDetectorsInSpec(i);
      }
      else
      {// couldn't get a pointer to the detector, again blank the spectra and log
        spuriousSpectra.push_back(i);
      }// we can't mask the detector if we can't find it
    }
    
    // this algorithm doesn't change any of the bin boundaries
    if ( m_outputWS != m_inputWS )
    {
      m_outputWS->dataX(i) = m_inputWS->readX(i);
    }

    
    // make regular progress reports and check for cancelling the algorithm
    if ( i % progStep == 0 )
    {
      progress(static_cast<double>(i)/numHists);
      interruption_point();
    }
  }
  logErrors(spuriousSpectra, unsetParams);
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
*  @throw NullPointerException we can't get a pointer to a detector
*  @throw invalid_argument if the shape of a detector is isn't a cylinder aligned along one axis
*  @throw runtime_error if the SpectraDetectorMap has not been filled
*  @throw NotFoundError if the detector or its gas pressure or wall thickness were not found
*/
void DetectorEfficiencyCor::efficiencyCorrect(int spectraIn)
{
  // get a pointer to one of the detector that created the spectrum
  const int specNum = m_inputWS->getAxis(1)->spectraNo(spectraIn);
  const std::vector<int> dets = m_inputWS->spectraMap().getDetectors(specNum);
  // now test if we have a good detector or not
  // could get rid of this test as the function above should throw rather than return NULL
  if ( dets.size() < 1 )
  {
    throw Exception::NullPointerException("DetectorEfficiencyCor::detectorEfficiency()", "dets");
  }
  //??STEVES?? look at this again
  // for faster processing we are going to assume that all the detectors in a group are the same shape, so we can just examine the properties of the first
  const IDetector_sptr detector = m_inputWS->getInstrument()->getDetector(dets[0]);
  if ( ! detector )
  {
    throw Exception::NullPointerException("DetectorEfficiencyCor::detectorEfficiency()", "detector");
  }
  if (detector->isMonitor())
  {// the monitor spectra will be untouched by this algorithm
    return;
  }

  IComponent* detComp = NULL;
  try
  {// catch the common problem 0f the detector map and the detectors listed in the instrument file being slightly different
    // updates sin of the angle between the detector axis and a line to the sample, m_sinThetaCache, and, only if the shape is different, the radius, m_radCache
    getDetectorGeometry(detector);
    
    // the rest of the data we need is stored in a map of properties against the componants pointer
    detComp = detector->getComponent();
  }// deal with exceptions caused by detectors not being listed in the instrument file because this happens a lot
  catch( std::runtime_error &e )
  {// deal with exceptions thrown here separately from those thrown below
    throw Exception::NotFoundError(e.what(), spectraIn);
  }
  
  Parameter_sptr par = m_paraMap->get(detComp,"3He(atm)");
  if ( ! par )
  {
    throw Exception::NotFoundError("3He(atm)", spectraIn);
  }
  const double atms = par->value<double>();
    
  par = m_paraMap->get(detComp,"wallT(m)");
  if ( ! par )
  {
    throw Exception::NotFoundError("wallT(m)", spectraIn);
  }
  
  // 1 - (wallThickness/radius)
  m_1_t2rad = 1 - (par->value<double>()/m_radCache);
  
  // cache the values that we've calculated because the next part of the efficiency calculation will vary for each bin in the spectrum!
  m_CONST_rad_sintheta_1_t2rad_atms =
    CONSTA*(m_radCache/m_sinThetaCache)*(m_1_t2rad)*atms;

  if ( m_XsCache != &(m_inputWS->readX(spectraIn)) )
  {// either the X-values have changed or this is the first time this function has been run, either way the calculate wave vectors
    set1_wvec(spectraIn);
    m_XsCache = &(m_inputWS->readX(spectraIn));
  }

  // in this loop we go through all the bins in all the spectra and so it takes a lot of time
  for ( MantidVec::size_type i = 0; i < m_inputWS->readY(spectraIn).size(); ++i )
  {
    //          correction= (k_i/k_f)        / detector_efficiency
    const double correcti = m_ki*m_1_wvec[i] / EFF(m_1_wvec[i]);
    // an efficiency of zero shouldn't happen so, to save processor time, I don't check for divide by zero  if ( correction < 0 || correction == std::numeric_limits<double>::infinity() ) g_log.fatal() << "Neg E spec " << spectraIn << " index " << i << std::endl;
    m_outputWS->dataY(spectraIn)[i] = m_inputWS->readY(spectraIn)[i]*correcti;
    m_outputWS->dataE(spectraIn)[i] = m_inputWS->readE(spectraIn)[i]*correcti;
  }
}
/** Calculates the wave vectors from the X-values in the specified histogram
*  updating the value of m_1_wvec
*  @param spectraIn the index number of the histogram to do the calculation for
*/
void DetectorEfficiencyCor::set1_wvec(int spectraIn)
{
  // there is one more X bin boundary varible than the bins contained by them
  MantidVec::size_type numBins = m_inputWS->readX(spectraIn).size() - 1;
  m_1_wvec.resize( numBins );
  for ( MantidVec::size_type j = 0; j < numBins; ++j)
  {
    double firstBoundary = m_inputWS->readX(spectraIn)[j];
    double secondBoundary = m_inputWS->readX(spectraIn)[j+1];
    m_1_wvec[j] = get1OverK((firstBoundary + secondBoundary )/2);
  }
}
/** Calculates one over the wave number of a neutron based on deltaE
*  @param DeltaE the engery the neutron lost in the sample (meV)
*  @return one over the final neutron wavenumber in Angsstrom
*/
double DetectorEfficiencyCor::get1OverK(double DeltaE) const
{
  double E = m_Ei - DeltaE;
  double oneOverKSquared = KSquaredToE/E;
  return std::sqrt(oneOverKSquared);
}
/** Sets m_rad and m_sin to the detector radius and the sin of angle between its axis and a
*  line to the sample. Doesn't check if we're looking at a monitor
*  @param det a pointer to the detector to query
*  @throw invalid_argument when the shape of the detector is inconsistent with a cylinder aligned along one axis
*/
void DetectorEfficiencyCor::getDetectorGeometry(boost::shared_ptr<Geometry::IDetector> det)
{
  // get the shape information to find the radius and the base orientation of the cylinder
  const boost::shared_ptr< const Object > shape = det->Shape();
  // check if the shape if is different for this detector compared to the last, as most of the time the shapes are the same
  if ( shape.get() != m_shapeCache )
  {// it is different so we need to do the calculations
    m_shapeCache = shape.get();
    getCylinderAxis();
  }

  // now get the sin of the angle, it's the magnitude of the cross product of unit vector along the detector tube axis and a unit vector directed from the sample to the detector centre
  IObjComponent_sptr origin = m_inputWS->getInstrument()->getSample();
  V3D vectorFromSample = det->getPos() - origin->getPos();
  vectorFromSample.normalize();

  Quat rot = det->getRotation();
  // m_baseAxis was set by the last call to getCylinderAxis() above
  V3D detAxis = m_baseAxisCache;
  // rotate the original cylinder object axis to get the detector axis in the actual instrument
  rot.rotate(detAxis); 
  detAxis.normalize();

  // |A X B| = |A||B|sin(theta) i.e. |detAxis X vectorFromSample| = |detAxis||vectorFromSample|sin(theta)
  V3D vectorProd = vectorFromSample.cross_prod(detAxis);
  // |detAxis| = |vectorFromSample| = 1 (because normalize() was run on both these vectors before)
  // so      sin(theta) = |detAxis X vectorFromSample|
  m_sinThetaCache = vectorProd.norm();

//  g_log.debug() << "vectorProd = (" << vectorProd.X() << ", " << vectorProd.Y() << ", " << vectorProd.Z() << ")";
//  g_log.debug() << "   vectorFromSample = (" << vectorFromSample.X() << ", " << vectorFromSample.Y() << ", " << vectorFromSample.Z() << ")";
//  g_log.debug() << "   detAxis = (" << detAxis.X() << ", " << detAxis.Y() << ", " << detAxis.Z() << ")";
}

/** Calculates the radius of cylinderical detectors, function assumes that the detector's axis is aligned with
*  either the x-axis, the y-axis or the z-axis
*  @throw invalid_argument if we cannot get the radius or axis direction
*/
void DetectorEfficiencyCor::getCylinderAxis()
{
  double xDist=DistToSurface( V3D(DIST_TO_UNIVERSE_EDGE, 0, 0), m_shapeCache );
  double yDist=DistToSurface( V3D(0, DIST_TO_UNIVERSE_EDGE, 0), m_shapeCache );
  double zDist=DistToSurface( V3D(0, 0, DIST_TO_UNIVERSE_EDGE), m_shapeCache );
  
  if ( std::abs(yDist - zDist) < 1e-8 )
  {// assume that y and z are radi of the cylinder's circular cross-section and the axis is perpendicular, in the x direction
    m_radCache = yDist/2.0;
    m_baseAxisCache = V3D(1, 0, 0);
    return;
  }
  if ( std::abs(zDist - xDist) < 1e-8 )
  {// assume radi in z and x and the axis is in the y
    m_radCache = zDist/2.0;
    m_baseAxisCache = V3D(0, 1, 0);
    return;
  }
  if ( std::abs(xDist - yDist) < 1e-8 )
  {
    m_radCache = xDist/2.0;
    m_baseAxisCache = V3D(0, 0, 1);
    return;
  }
  
  g_log.debug() << "Found a detector that doesn't appear to be a cylinder on either the x-axis, the y-axis or the z-axis" << std::endl;
  throw std::invalid_argument("Fatal error while calculating the radius and axis of a detector, the detector shape is not as expected" );
}

/** For basic shapes centred on the origin (0,0,0) this returns the distance to the surface in
*  the direction of the point given
*  @param start the distance calculated from origin to the surface in a line towards this point. It should be outside the shape
*  @param shape the object to calculate for, should be centred on the origin
*  @throw invalid_argument if there is any error finding the distance
*/
double DetectorEfficiencyCor::DistToSurface(const V3D start, const Object *shape) const
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
  return track.begin()->Length;
}

/** Calculates detector efficiency, copied from the fortran code in effic_3he_cylinder.for
*  @param oneOverwvec Final neutron wavevector (Angsstrom^-1)
*  @return detector efficiency
*/
double DetectorEfficiencyCor::EFF(const double oneOverwvec) const
{
  //T.G.Perring Aug 2009: replace the following with the one after:  alf = const*rad*(1.0d0-t2rad)*atms/wvec
  // implements the equation with a large amount of caching of calculated values in member variables
  double alf = m_CONST_rad_sintheta_1_t2rad_atms  *  oneOverwvec;

  if ( alf < 9.0 )
  {
    return (M_PI/4.0)*alf*EFFCHB(0.0, 10.0, c_eff_f, alf);
  }
  if ( alf > 10.0 )
  {
    double y = 1.0 - 18.0/alf;
    return 1.0 - EFFCHB(-1.0, 1.0, c_eff_g, y)/(alf*alf);
  }
  double eff_f =(M_PI/4.0)*alf*EFFCHB(0.0, 10.0, c_eff_f, alf);
  double y=1.0 - 18.0/alf;
  double eff_g =1.0 - EFFCHB(-1.0,1.0,c_eff_g, y)/(alf*alf);
  return (10.0-alf)*eff_f  + (alf-9.0)*eff_g;
}
/** Calculates an expansion similar to that in CHEBEV of "Numerical Recipes"
*  copied from the fortran code in effic_3he_cylinder.for
* @param a a fit parameter, only the difference between a and b enters this equation
* @param b a fit parameter, only the difference between a and b enters this equation 
* @param exspansionCoefs one of the 25 element constant arrays declared in this file
* @param x a fit parameter
* @return a numerical approximation provided by the expansion
*/
double DetectorEfficiencyCor::EFFCHB(double a, double b, const double exspansionCoefs[], double x) const
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
/** If the missing detector information lists are not empty mask the listed detectors, if possible, and
*  log
*  @param spuriousSpectra the list of spectra indexes for histograms whoses detectors can't be found
*  @param unsetParams list of spectra indexes for histograms where detector information is incomplete, these spectra will be masked
*  @throw invalid_argument if there is no BaseInstrument in the workspace
*/
void DetectorEfficiencyCor::logErrors(std::vector<int> &spuriousSpectra, std::vector<int> &unsetParams) const
{
  if (spuriousSpectra.size() > 0)
  {
    g_log.information() << "Found " << spuriousSpectra.size() << " spectra without associated detectors, probably the detectors are not present in the instrument definition and this is not serious. The Y values for those spectra have been set to zero" << std::endl;
    g_log.information() << "The spectrum numbers that were set to zero are: " << m_inputWS->getAxis(1)->spectraNo(spuriousSpectra[0]);
    for ( std::vector<int>::size_type k = 1; k < spuriousSpectra.size(); k++ )
    {
      g_log.information() << ", " << m_inputWS->getAxis(1)->spectraNo(spuriousSpectra[k]);
    }
    g_log.information() << std::endl;
  }
  if (unsetParams.size() > 0)
  {
    g_log.warning() << "Found " << unsetParams.size() << " spectra where the first detector didn't contain gas pressure or wall thickness. Their detectors have been masked and Y values set to zero" << std::endl;
    g_log.warning() << "The spectrum numbers are: ";
    // try to mask the detector
    for ( std::vector<int>::size_type k = 0; k < unsetParams.size(); k++ )
    {
      g_log.warning() << " " << m_inputWS->getAxis(1)->spectraNo(unsetParams[k]);
    }
    g_log.warning() << std::endl;
  }
}

} // namespace Algorithm
} // namespace Mantid
