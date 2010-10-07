#include "MantidAlgorithms/He3TubeEfficiency.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/Exception.h"
//#include "MantidKernel/cow_ptr.h"
#include <algorithm>
#include <cmath>

// this should be a big number but not so big that there are rounding errors
const double DIST_TO_UNIVERSE_EDGE = 1e3;

// Scalar constant for exponential in units of K / (m Angstroms atm)
const double EXP_SCALAR_CONST = 2175.48686;

namespace Mantid
{
namespace Algorithms
{
// Register the class into the algorithm factory
DECLARE_ALGORITHM(He3TubeEfficiency)

/// Default constructor
He3TubeEfficiency::He3TubeEfficiency() : Algorithm(), inputWS(),
outputWS(), paraMap(NULL), spectraSkipped()
{

}

/// Destructor
He3TubeEfficiency::~He3TubeEfficiency()
{
}

/**
 * Declare algorithm properties
 */
void He3TubeEfficiency::init()
{
  API::CompositeValidator<> *wsValidator = new API::CompositeValidator<>;
  wsValidator->add(new API::WorkspaceUnitValidator<>("Wavelength"));
  wsValidator->add(new API::HistogramValidator<>);
  this->declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace",
      "", Kernel::Direction::Input, wsValidator), "Name of the input workspace");
  this->declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace",
      "", Kernel::Direction::Output),
      "Name of the output workspace, can be the same as the input" );
}

/**
 * Executes the algorithm
 *
 * @throw NotImplementedError if the input workspace is an EventWorkspace
 */
void He3TubeEfficiency::exec()
{
  // Get the workspaces
  this->inputWS = this->getProperty("InputWorkspace");

  // Check if its an event workspace
  DataObjects::EventWorkspace_const_sptr eventWS = \
      boost::dynamic_pointer_cast<const DataObjects::EventWorkspace>(this->inputWS);
  if (eventWS != NULL)
  {
    //g_log.error() << "EventWorkspaces are not supported!" << std::endl;
    throw Kernel::Exception::NotImplementedError("EventWorkspaces are not supported!");
  }

  // Get the detector parameters
  this->paraMap = &(this->inputWS->instrumentParameters());

  this->outputWS = this->getProperty("OutputWorkspace");

  if (this->outputWS != this->inputWS)
  {
    this->outputWS = API::WorkspaceFactory::Instance().create(this->inputWS);
  }

  // Store some information about the instrument setup that will not change
  this->samplePos = this->inputWS->getInstrument()->getSample()->getPos();

  int numHists = this->inputWS->getNumberHistograms();
  const int progStep = static_cast<int>(ceil(numHists/100.0));

  PARALLEL_FOR2(inputWS, outputWS)
  for (int i = 0; i < numHists; ++i )
  {
    PARALLEL_START_INTERUPT_REGION

    this->outputWS->setX(i, this->inputWS->refX(i));
    try
    {
      this->correctForEfficiency(i);
    }
    catch (Kernel::Exception::NotFoundError &)
    {
      // if we don't have all the data there will be spectra we can't correct,
      // avoid leaving the workspace part corrected
      Mantid::MantidVec& dud = this->outputWS->dataY(i);
      std::transform(dud.begin(), dud.end(), dud.begin(),
          std::bind2nd(std::multiplies<double>(), 0));
      PARALLEL_CRITICAL(deteff_invalid)
      {
        this->spectraSkipped.push_back(this->inputWS->getAxis(1)->spectraNo(i));
      }
    }

    // make regular progress reports and check for canceling the algorithm
    if ( i % progStep == 0 )
    {
      progress(static_cast<double>(i)/numHists);
      interruption_point();
    }

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  this->logErrors();
  this->setProperty("OutputWorkspace", this->outputWS);
}

/**
 * Corrects a spectra for the detector efficiency calculated from detector
 * information. Gets the detector information and uses this to calculate its
 * efficiency
 *  @param spectraIndex index of the spectrum to get the efficiency for
 *  @throw invalid_argument if the shape of a detector is isn't a cylinder
 *  aligned along one axis
 *  @throw runtime_error if the SpectraDetectorMap has not been filled
 *  @throw NotFoundError if the detector or its gas pressure or wall thickness
 *  were not found
 */
void He3TubeEfficiency::correctForEfficiency(int spectraIndex)
{
  Geometry::IDetector_sptr det = this->inputWS->getDetector(spectraIndex);
  if( det->isMonitor() || det->isMasked() )
  {
    return;
  }

  Mantid::MantidVec &yout = this->outputWS->dataY(spectraIndex);
  Mantid::MantidVec &eout = this->outputWS->dataE(spectraIndex);
  // Need the original values so this is not a reference
  const Mantid::MantidVec yValues = this->inputWS->readY(spectraIndex);
  const Mantid::MantidVec eValues = this->inputWS->readE(spectraIndex);

  Geometry::Parameter_sptr par = this->paraMap->get(det.get(), "tube_pressure");
  if ( !par )
  {
    throw Kernel::Exception::NotFoundError("tube_pressure", spectraIndex);
  }
  g_log.information() << "Detector " << spectraIndex << ", Tube Pressure "
      << par->value<double>() << std::endl;
  double pressure = par->value<double>();

  par = this->paraMap->get(det.get(), "tube_temperature");
  if ( !par )
  {
    throw Kernel::Exception::NotFoundError("tube_temperature", spectraIndex);
  }
  double temperature = par->value<double>();

  double detRadius(0.0);
  Geometry::V3D detAxis;
  this->getDetectorGeometry(det, detRadius, detAxis);

  // now get the sin of the angle, it's the magnitude of the cross product of
  // unit vector along the detector tube axis and a unit vector directed from
  // the sample to the detector center
  Geometry::V3D vectorFromSample = det->getPos() - this->samplePos;
  vectorFromSample.normalize();
  Geometry::Quat rot = det->getRotation();
  // rotate the original cylinder object axis to get the detector axis in the
  // actual instrument
  rot.rotate(detAxis);
  detAxis.normalize();
  // Scalar product is quicker than cross product
  double cosTheta = detAxis.scalar_prod(vectorFromSample);
  double sinTheta = std::sqrt(1.0 - cosTheta*cosTheta);

  g_log.information() << "Detector " << spectraIndex << ", Tube Radius "
      << detRadius << std::endl;

  std::vector<double>::const_iterator yinItr = yValues.begin();
  std::vector<double>::const_iterator einItr = eValues.begin();
  Mantid::MantidVec::iterator youtItr = yout.begin();
  Mantid::MantidVec::iterator eoutItr = eout.begin();

  for( ; youtItr != yout.end(); ++youtItr, ++eoutItr)
  {
    const double effcorr = detectorEfficiency();
    *youtItr = (*yinItr) * effcorr;
    *eoutItr = (*einItr) * effcorr;
    ++yinItr; ++einItr;
  }

  return;
}

/**
 * Update the shape cache if necessary
 * @param det a pointer to the detector to query
 * @param detRadius An output paramater that contains the detector radius
 * @param detAxis An output parameter that contains the detector axis vector
 */
void He3TubeEfficiency::getDetectorGeometry(\
    boost::shared_ptr<Geometry::IDetector> det,
    double & detRadius, Geometry::V3D & detAxis)
{
  boost::shared_ptr<const Geometry::Object> shape_sptr = det->Shape();
  std::map<const Geometry::Object *, std::pair<double, Geometry::V3D> >::const_iterator it =
    this->shapeCache.find(shape_sptr.get());
  if( it == this->shapeCache.end() )
  {
    double xDist = distToSurface( Geometry::V3D(DIST_TO_UNIVERSE_EDGE, 0, 0),
        shape_sptr.get() );
    double zDist = distToSurface( Geometry::V3D(0, 0, DIST_TO_UNIVERSE_EDGE),
        shape_sptr.get() );
    if ( std::abs(zDist - xDist) < 1e-8 )
    {
      detRadius = zDist / 2.0;
      detAxis = Geometry::V3D(0, 1, 0);
      // assume radi in z and x and the axis is in the y
      PARALLEL_CRITICAL(deteff_shapecachea)
      {
        this->shapeCache.insert(std::pair<const Geometry::Object *,
            std::pair<double, Geometry::V3D> >(shape_sptr.get(),
                std::pair<double, Geometry::V3D>(detRadius, detAxis)));
      }
      return;
    }
    double yDist = distToSurface( Geometry::V3D(0, DIST_TO_UNIVERSE_EDGE, 0),
        shape_sptr.get() );
    if ( std::abs(yDist - zDist) < 1e-8 )
    {
      detRadius = yDist / 2.0;
      detAxis = Geometry::V3D(1, 0, 0);
      // assume that y and z are radi of the cylinder's circular cross-section
      // and the axis is perpendicular, in the x direction
      PARALLEL_CRITICAL(deteff_shapecacheb)
      {
        this->shapeCache.insert(std::pair<const Geometry::Object *,
            std::pair<double, Geometry::V3D> >(shape_sptr.get(),
                std::pair<double, Geometry::V3D>(detRadius, detAxis)));
      }
      return;
    }

    if ( std::abs(xDist - yDist) < 1e-8 )
    {
      detRadius = xDist / 2.0;
      detAxis = Geometry::V3D(0, 0, 1);
      PARALLEL_CRITICAL(deteff_shapecachec)
      {
        this->shapeCache.insert(std::pair<const Geometry::Object *,
            std::pair<double, Geometry::V3D> >(shape_sptr.get(),
                std::pair<double, Geometry::V3D>(detRadius, detAxis)));
      }
      return;
    }
  }
  else
  {
    std::pair<double, Geometry::V3D> geometry = it->second;
    detRadius = it->second.first;
    detAxis = it->second.second;
  }
}

/**
 * For basic shapes centered on the origin (0,0,0) this returns the distance to
 * the surface in the direction of the point given
 *  @param start the distance calculated from origin to the surface in a line
 *  towards this point. It should be outside the shape
 *  @param shape the object to calculate for, should be centered on the origin
 *  @return the distance to the surface in the direction of the point given
 *  @throw invalid_argument if there is any error finding the distance
 * @returns The distance to the surface in metres
 */
double He3TubeEfficiency::distToSurface(const Geometry::V3D start,
    const Geometry::Object *shape) const
{
  // get a vector from the point that was passed to the origin
  Geometry::V3D direction = Geometry::V3D(0.0, 0.0, 0.0) - start;
  // it needs to be a unit vector
  direction.normalize();
  // put the point and the vector (direction) together to get a line,
  // here called a track
  Geometry::Track track( start, direction );
  // split the track (line) up into the part that is inside the shape and the
  // part that is outside
  shape->interceptSurface(track);

  if ( track.count() != 1 )
  {
    // the track missed the shape, probably the shape is not centered on
    // the origin
    throw std::invalid_argument("Fatal error interpreting the shape of a detector");
  }
  // the first part of the track will be the part inside the shape,
  // return its length
  return track.begin()->Length;
}

/**
 * Calculate the detector efficiency from the detector parameters and the
 * spectrum's x-axis.
 */
double He3TubeEfficiency::detectorEfficiency() const
{
  double x = 1.0;
  return x;
}

/**
 * Logs if there were any problems locating spectra.
 */
void He3TubeEfficiency::logErrors() const
{
  std::vector<int>::size_type nspecs = this->spectraSkipped.size();
  if( nspecs > 0 )
  {
    g_log.warning() << "There were " <<  nspecs
        << " spectra that could not be corrected. ";
    g_log.debug() << "Unaffected spectra numbers: ";
    for( size_t i = 0; i < nspecs; ++i )
    {
      g_log.debug() << this->spectraSkipped[i] << " ";
    }
    g_log.debug() << std::endl;
  }
}
} // namespace Algorithms
} // namespace Mantid
