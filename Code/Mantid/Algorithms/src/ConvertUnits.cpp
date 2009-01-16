//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include <cfloat>
#include <iostream>

namespace Mantid
{
namespace Algorithms
{

// Register with the algorithm factory
DECLARE_ALGORITHM(ConvertUnits)

using namespace Kernel;
using namespace API;
using namespace DataObjects;

// Get a reference to the logger
Logger& ConvertUnits::g_log = Logger::get("ConvertUnits");

/// Default constructor
ConvertUnits::ConvertUnits() : Algorithm()
{
}

/// Destructor
ConvertUnits::~ConvertUnits()
{
}

/// Initialisation method
void ConvertUnits::init()
{
  CompositeValidator<> *wsValidator = new CompositeValidator<>;
  wsValidator->add(new WorkspaceUnitValidator<>);
  wsValidator->add(new HistogramValidator<>);
  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace","",Direction::Input,wsValidator));
  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace","",Direction::Output));

  // Extract the current contents of the UnitFactory to be the allowed values of the Target property
  declareProperty("Target","",new ListValidator(UnitFactory::Instance().getKeys()) );
  declareProperty("Emode",0,new BoundedValidator<int>(0,2));
  BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
  mustBePositive->setLower(0.0);
  declareProperty("Efixed",0.0,mustBePositive);

  declareProperty("AlignBins",false);
}

/** Executes the algorithm
 *  @throw std::runtime_error If the input workspace has not had its unit set
 *  @throw NotImplementedError If the input workspace contains point (not histogram) data
 *  @throw InstrumentDefinitionError If unable to calculate source-sample distance
 */
void ConvertUnits::exec()
{
  // Get the workspaces
  API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  API::MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");

  // Check that the input workspace doesn't already have the desired unit.
  // If it does, just set the output workspace to point to the input one.
  Kernel::Unit_sptr inputUnit = inputWS->getAxis(0)->unit();
  const std::string targetUnit = getPropertyValue("Target");
  if ( inputUnit->unitID() == targetUnit )
  {
    g_log.information() << "Input workspace already has target unit (" << targetUnit
                        << "), so just pointing the output workspace property to the input workspace." << std::endl;
    setProperty("OutputWorkspace",boost::const_pointer_cast<MatrixWorkspace>(inputWS));
    return;
  }

  // If input and output workspaces are not the same, create a new workspace for the output
  if (outputWS != inputWS )
  {
    outputWS = WorkspaceFactory::Instance().create(inputWS);
    setProperty("OutputWorkspace",outputWS);
  }

  // Set the final unit that our output workspace will have
  Kernel::Unit_const_sptr outputUnit = outputWS->getAxis(0)->unit() = UnitFactory::Instance().create(targetUnit);

  // Check whether the Y data of the input WS is dimensioned and set output WS flag to be same
  const bool distribution = outputWS->isDistribution(inputWS->isDistribution());
  const unsigned int size = inputWS->blocksize();
  // Calculate the number of spectra in this workspace
  const int numberOfSpectra = inputWS->size() / size;

  int iprogress_step = numberOfSpectra / 100;
  if (iprogress_step == 0) iprogress_step = 1;
  // Loop over the histograms (detector spectra)
  for (int i = 0; i < numberOfSpectra; ++i) {

    // Take the bin width dependency out of the Y & E data
    if (distribution)
    {
      for (unsigned int j = 0; j < size; ++j)
      {
        const double width = std::abs( inputWS->dataX(i)[j+1] - inputWS->dataX(i)[j] );
        outputWS->dataY(i)[j] = inputWS->dataY(i)[j]*width;
        outputWS->dataE(i)[j] = inputWS->dataE(i)[j]*width;
      }
    }
    else
    {
      // Just copy over
      outputWS->dataY(i) = inputWS->dataY(i);
      outputWS->dataE(i) = inputWS->dataE(i);
    }
    // Copy over the X data (no copying will happen if the two workspaces are the same)
    outputWS->dataX(i) = inputWS->dataX(i);
    if ( i % iprogress_step == 0)
    {
        progress( double(i)/numberOfSpectra/2 );
        interruption_point();
    }

  }

  // Check whether there is a quick conversion available
  double factor, power;
  if ( inputUnit->quickConversion(*outputUnit,factor,power) )
  // If test fails, could also check whether a quick conversion in the opposite direction has been entered
  {
    convertQuickly(numberOfSpectra,outputWS,factor,power);
  }
  else
  {
    convertViaTOF(numberOfSpectra,inputUnit,outputWS);
  }

  // If the units conversion has flipped the ascending direction of X, reverse all the vectors
  if (outputWS->dataX(0).size() && ( outputWS->dataX(0).front() > outputWS->dataX(0).back()
        || outputWS->dataX(numberOfSpectra-1).front() > outputWS->dataX(numberOfSpectra-1).back() ) )
  {
    this->reverse(outputWS);
  }

  // Rebin the data to common bins if requested, and if necessary
  bool alignBins = getProperty("AlignBins");
  if (alignBins && !WorkspaceHelpers::commonBoundaries(outputWS))
  {
    outputWS = this->alignBins(outputWS);
    setProperty("OutputWorkspace",outputWS);
  }

  // If appropriate, put back the bin width division into Y/E.
  if (distribution)
  {
    for (int i = 0; i < numberOfSpectra; ++i) {
      // There must be good case for having a 'divideByBinWidth'/'normalise' algorithm...
      for (unsigned int j = 0; j < size; ++j)
      {
        const double width = std::abs( outputWS->dataX(i)[j+1] - outputWS->dataX(i)[j] );
        outputWS->dataY(i)[j] = outputWS->dataY(i)[j]/width;
        outputWS->dataE(i)[j] = outputWS->dataE(i)[j]/width;
      }
    }
  }

}

/** Convert the workspace units according to a simple output = a * (input^b) relationship
 * @param numberOfSpectra The number of Spectra
 * @param outputWS the output workspace
 * @param factor the conversion factor a to apply
 * @param power the Power b to apply to the conversion
 */
void ConvertUnits::convertQuickly(const int& numberOfSpectra, API::MatrixWorkspace_sptr outputWS, const double& factor, const double& power)
{
  // See if the workspace has common bins - if so the X vector can be common
  // First a quick check using the validator
  CommonBinsValidator<> sameBins;
  if ( sameBins.isValid(outputWS) )
  {
    // Only do the full check if the quick one passes
    if ( WorkspaceHelpers::commonBoundaries(outputWS) )
    {
      // Calculate the new (common) X values
      std::vector<double>::iterator iter;
      for (iter = outputWS->dataX(0).begin(); iter != outputWS->dataX(0).end(); ++iter)
      {
        *iter = factor * std::pow(*iter,power);
      }
      // If this is a Workspace2D then loop over the other spectra passing in the pointer
      Workspace2D_sptr WS2D = boost::dynamic_pointer_cast<Workspace2D>(outputWS);
      if (WS2D)
      {
        Histogram1D::RCtype xVals;
        xVals.access() = outputWS->dataX(0);
        for (int j = 1; j < numberOfSpectra; ++j)
        {
          WS2D->setX(j,xVals);
          if ( j % 100 == 0)
          {
              progress( 0.5 + double(j)/numberOfSpectra/2 );
              interruption_point();
          }
        }
      }
      return;
    }
  }
  // If we get to here then the bins weren't aligned and each spectrum is unique
  // Loop over the histograms (detector spectra)
  for (int k = 0; k < numberOfSpectra; ++k) {
    std::vector<double>::iterator it;
    for (it = outputWS->dataX(k).begin(); it != outputWS->dataX(k).end(); ++it)
    {
      *it = factor * std::pow(*it,power);
    }
  }
  return;
}

/** Convert the workspace units using TOF as an intermediate step in the conversion
 * @param numberOfSpectra The number of Spectra
 * @param fromUnit The unit of the input workspace
 * @param outputWS The output workspace
 */
void ConvertUnits::convertViaTOF(const int& numberOfSpectra, Kernel::Unit_const_sptr fromUnit, API::MatrixWorkspace_sptr outputWS)
{
  // Get a pointer to the instrument contained in the workspace
  IInstrument_const_sptr instrument = outputWS->getInstrument();
  // And one to the SpectraDetectorMap
  SpectraMap_const_sptr specMap = outputWS->getSpectraMap();

  // Get the unit object for each workspace
  //boost::shared_ptr<Unit> inputUnit = inputWS->getAxis(0)->unit();
  Kernel::Unit_const_sptr outputUnit = outputWS->getAxis(0)->unit();

  // Get the distance between the source and the sample (assume in metres)
  Geometry::IObjComponent_const_sptr sample = instrument->getSample();
  double l1;
  try
  {
    l1 = instrument->getSource()->getDistance(*sample);
    g_log.debug() << "Source-sample distance: " << l1 << std::endl;
  }
  catch (Exception::NotFoundError e)
  {
    g_log.error("Unable to calculate source-sample distance");
    throw Exception::InstrumentDefinitionError("Unable to calculate source-sample distance", outputWS->getTitle());
  }
  Geometry::V3D samplePos = sample->getPos();

  const int notFailed = -99;
  int failedDetectorIndex = notFailed;

  // Not doing anything with the Y vector in to/fromTOF yet, so just pass empty vector
  std::vector<double> emptyVec;

  // Loop over the histograms (detector spectra)
  for (int i = 0; i < numberOfSpectra; ++i) {

    /// @todo No implementation for any of these in the geometry yet so using properties
    const int emode = getProperty("Emode");
    const double efixed = getProperty("Efixed");
    /// @todo Don't yet consider hold-off (delta)
    const double delta = 0.0;

    try {
      // Get the spectrum number for this histogram
      const int spec = outputWS->getAxis(1)->spectraNo(i);
      // Now get the detector to which this relates
      Geometry::IDetector_const_sptr det = specMap->getDetector(spec);
      Geometry::V3D detPos = det->getPos();
      // Get the sample-detector distance for this detector (in metres)
      double l2, twoTheta;
      if ( ! det->isMonitor() )
      {
        l2 = detPos.distance(samplePos);
        // The scattering angle for this detector (in radians).
        twoTheta = instrument->detectorTwoTheta(det);
      }
      else  // If this is a monitor then make l1+l2 = source-detector distance and twoTheta=0
      {
        l2 = detPos.distance(instrument->getSource()->getPos());
        l2 = l2 - l1;
        twoTheta = 0.0;
      }
      if (failedDetectorIndex != notFailed)
      {
        g_log.information() << "Unable to calculate sample-detector[" << failedDetectorIndex << "-" << i-1 << "] distance. Zeroing spectrum." << std::endl;
        failedDetectorIndex = notFailed;
      }

      // Convert the input unit to time-of-flight
      fromUnit->toTOF(outputWS->dataX(i),emptyVec,l1,l2,twoTheta,emode,efixed,delta);
      // Convert from time-of-flight to the desired unit
      outputUnit->fromTOF(outputWS->dataX(i),emptyVec,l1,l2,twoTheta,emode,efixed,delta);

   } catch (Exception::NotFoundError e) {
      // Get to here if exception thrown when calculating distance to detector
      if (failedDetectorIndex == notFailed)
      {
        failedDetectorIndex = i;
      }
      outputWS->dataX(i).assign(outputWS->dataX(i).size(),0.0);
      outputWS->dataY(i).assign(outputWS->dataY(i).size(),0.0);
      outputWS->dataE(i).assign(outputWS->dataE(i).size(),0.0);
    }

    if ( i % 100 == 0)
    {
        progress( 0.5 + double(i)/numberOfSpectra/2 );
        interruption_point();
    }
  } // loop over spectra

  if (failedDetectorIndex != notFailed)
  {
    g_log.information() << "Unable to calculate sample-detector[" << failedDetectorIndex << "-" << numberOfSpectra-1 << "] distance. Zeroing spectrum." << std::endl;
  }

}

/// Calls Rebin as a sub-algorithm to align the bins
API::MatrixWorkspace_sptr ConvertUnits::alignBins(API::MatrixWorkspace_sptr workspace)
{
  // Create a Rebin child algorithm
  Algorithm_sptr childAlg = createSubAlgorithm("Rebin");
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", workspace);
  childAlg->setProperty<std::vector<double> >("params",this->calculateRebinParams(workspace));

  // Now execute the sub-algorithm. Catch and log any error
  try
  {
    childAlg->execute();
  }
  catch (std::runtime_error& err)
  {
    g_log.error("Unable to successfully run Rebinning sub-algorithm");
    throw;
  }

  if ( ! childAlg->isExecuted() )
  {
    g_log.error("Unable to successfully run Rebinning sub-algorithm");
    throw std::runtime_error("Unable to successfully run Rebinning sub-algorithm");
  }
  else
  {
    return childAlg->getProperty("OutputWorkspace");
  }
}

/// The Rebin parameters should cover the full range of the converted unit, with the same number of bins
const std::vector<double> ConvertUnits::calculateRebinParams(const API::MatrixWorkspace_const_sptr workspace) const
{
  // Need to loop round and find the full range
  double XMin = DBL_MAX, XMax = DBL_MIN;
  SpectraMap_const_sptr specMap = workspace->getSpectraMap();
  Axis* specAxis = workspace->getAxis(1);
  const int numSpec = workspace->getNumberHistograms();
  for (int i = 0; i < numSpec; ++i)
  {
    try {
      Geometry::IDetector_const_sptr det = specMap->getDetector(specAxis->spectraNo(i));
      if ( !det->isMonitor() && !det->isDead() )
      {
        const std::vector<double> XData = workspace->readX(i);
        if ( XData.front() < XMin ) XMin = XData.front();
        if ( XData.back() > XMax )  XMax = XData.back();
      }
    } catch (Exception::NotFoundError) {} //Do nothing
  }
  const double step = ( XMax - XMin ) / workspace->blocksize();

  std::vector<double> retval;
  retval.push_back(XMin);
  retval.push_back(step);
  retval.push_back(XMax);

  return retval;
}

void ConvertUnits::reverse(API::MatrixWorkspace_sptr WS)
{
  const int numberOfSpectra = WS->getNumberHistograms();

  // Only do the full check if the quick one passes
  if ( WorkspaceHelpers::commonBoundaries(WS) )
  {

      std::reverse(WS->dataX(0).begin(),WS->dataX(0).end());
      std::reverse(WS->dataY(0).begin(),WS->dataY(0).end());
      std::reverse(WS->dataE(0).begin(),WS->dataE(0).end());

      // If this is a Workspace2D then loop over the other spectra passing in the pointer
      Workspace2D_sptr WS2D = boost::dynamic_pointer_cast<Workspace2D>(WS);
      if (WS2D)
      {
        Histogram1D::RCtype xVals;
        xVals.access() = WS->dataX(0);
        for (int j = 1; j < numberOfSpectra; ++j)
        {
          WS2D->setX(j,xVals);
          std::reverse(WS->dataY(j).begin(),WS->dataY(j).end());
          std::reverse(WS->dataE(j).begin(),WS->dataE(j).end());
          if ( j % 100 == 0) interruption_point();
        }
      }
  }
  else
  {
    for (int j = 0; j < numberOfSpectra; ++j)
    {
      std::reverse(WS->dataX(j).begin(),WS->dataX(j).end());
      std::reverse(WS->dataY(j).begin(),WS->dataY(j).end());
      std::reverse(WS->dataE(j).begin(),WS->dataE(j).end());
      if ( j % 100 == 0) interruption_point();
    }
  }
}

} // namespace Algorithm
} // namespace Mantid
