#include "MantidAlgorithms/He3TubeEfficiency.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/cow_ptr.h"
#include <algorithm>

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

  this->outputWS = this->getProperty("OutputWorkspace");

  if (this->outputWS != this->inputWS)
  {
    this->outputWS = API::WorkspaceFactory::Instance().create(this->inputWS);
  }

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
 * Corrects a spectra for the detector efficiency calculated from detector information
 * Gets the detector information and uses this to calculate its efficiency
*  @param spectraIndex index of the spectrum to get the efficiency for
*  @throw invalid_argument if the shape of a detector is isn't a cylinder aligned along one axis
*  @throw runtime_error if the SpectraDetectorMap has not been filled
*  @throw NotFoundError if the detector or its gas pressure or wall thickness were not found
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
