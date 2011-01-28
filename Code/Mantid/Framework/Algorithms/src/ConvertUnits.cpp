//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include <cfloat>
#include <iostream>
#include <limits>

namespace Mantid
{
namespace Algorithms
{

// Register with the algorithm factory
DECLARE_ALGORITHM(ConvertUnits)

using namespace Kernel;
using namespace API;
using namespace DataObjects;

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
  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace","",Direction::Input,wsValidator),
    "Name of the input workspace");
  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace","",Direction::Output),
    "Name of the output workspace, can be the same as the input" );

  // Extract the current contents of the UnitFactory to be the allowed values of the Target property
  declareProperty("Target","",new ListValidator(UnitFactory::Instance().getKeys()),
    "The name of the units to convert to (must be one of those registered in\n"
    "the Unit Factory)");
  std::vector<std::string> propOptions;
  propOptions.push_back("Elastic");
  propOptions.push_back("Direct");
  propOptions.push_back("Indirect");
  declareProperty("EMode","Elastic",new ListValidator(propOptions),
    "The energy mode (default: elastic)");
  BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
  mustBePositive->setLower(0.0);
  declareProperty("EFixed",EMPTY_DBL(),mustBePositive,
    "Value of fixed energy in meV : EI (EMode=Direct) or EF (EMode=Indirect) . Must be\n"
    "set if the target unit requires it (e.g. DeltaE)");

  declareProperty("AlignBins",false,
    "If true (default is false), rebins after conversion to ensure that all spectra in the output workspace\n"
    "have identical bin boundaries. This option is not recommended (see http://www.mantidproject.org/ConvertUnits).");
}

/** Executes the algorithm
 *  @throw std::runtime_error If the input workspace has not had its unit set
 *  @throw NotImplementedError If the input workspace contains point (not histogram) data
 *  @throw InstrumentDefinitionError If unable to calculate source-sample distance
 */
void ConvertUnits::exec()
{
  // Get the workspaces
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

  //Check if its an event workspace
  EventWorkspace_const_sptr eventW = boost::dynamic_pointer_cast<const EventWorkspace>(inputWS);
  if (eventW != NULL)
  {
    //g_log.information() << "Executing ConvertUnits for Eventworkspace" << std::endl;
    this->execEvent();
    return;
  }

  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");

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
  if (outputWS != inputWS ) outputWS = WorkspaceFactory::Instance().create(inputWS);

  // Set the final unit that our output workspace will have
  Kernel::Unit_const_sptr outputUnit = outputWS->getAxis(0)->unit() = UnitFactory::Instance().create(targetUnit);

  const unsigned int size = inputWS->blocksize();
  // Calculate the number of spectra in this workspace
  const int numberOfSpectra = inputWS->size() / size;

  int iprogress_step = numberOfSpectra / 100;
  if (iprogress_step == 0) iprogress_step = 1;
  //input and output workspaces are checked to ensure they suitable for multithreaded access.

  // Check whether the Y data of the input WS is dimensioned and set output WS flag to be same
  bool distribution = outputWS->isDistribution(inputWS->isDistribution());
  // In the context of this algorithm, we treat things as a distribution if the flag is set
  // AND the data are not dimensionless
  distribution = distribution && !inputWS->YUnit().empty();

  // Loop over the histograms (detector spectra)
  Progress prog(this,0.0,0.5,numberOfSpectra);
  PARALLEL_FOR2(inputWS,outputWS)
  for (int i = 0; i < numberOfSpectra; ++i) 
  {
    PARALLEL_START_INTERUPT_REGION
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
    outputWS->setX( i, inputWS->refX(i) );
    
    prog.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

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
        || outputWS->dataX(numberOfSpectra/2).front() > outputWS->dataX(numberOfSpectra/2).back() ) )
  {
    this->reverse(outputWS);
  }

  // Need to lop bins off if converting to energy transfer
  /* This is an ugly test - could be made more general by testing for DBL_MAX
     values at the ends of all spectra, but that would be less efficient */
  if (targetUnit.find("Delta")==0) outputWS = this->removeUnphysicalBins(outputWS);

  // Rebin the data to common bins if requested, and if necessary
  bool alignBins = getProperty("AlignBins");
  if (alignBins && !WorkspaceHelpers::commonBoundaries(outputWS)) 
    outputWS = this->alignBins(outputWS);

  const unsigned int outSize = outputWS->blocksize();
  // If appropriate, put back the bin width division into Y/E.
  if (distribution)
  {
    for (int i = 0; i < numberOfSpectra; ++i) {
      // There must be good case for having a 'divideByBinWidth'/'normalise' algorithm...
      for (unsigned int j = 0; j < outSize; ++j)
      {
        const double width = std::abs( outputWS->dataX(i)[j+1] - outputWS->dataX(i)[j] );
        outputWS->dataY(i)[j] = outputWS->dataY(i)[j]/width;
        outputWS->dataE(i)[j] = outputWS->dataE(i)[j]/width;
      }
    }
  }

  setProperty("OutputWorkspace",outputWS);
  return;
}

/**
 * Execute ConvertUnits for event workspaces
 *
 */
void ConvertUnits::execEvent()
{
  //g_log.information("Processing event workspace");

  const MatrixWorkspace_const_sptr matrixInputWS = this->getProperty("InputWorkspace");
  EventWorkspace_const_sptr inputWS
                 = boost::dynamic_pointer_cast<const EventWorkspace>(matrixInputWS);

  // generate the output workspace pointer
  API::MatrixWorkspace_sptr matrixOutputWS = this->getProperty("OutputWorkspace");
  EventWorkspace_sptr outputWS;
  if (matrixOutputWS == matrixInputWS)
    outputWS = boost::dynamic_pointer_cast<EventWorkspace>(matrixOutputWS);
  else
  {
    //Make a brand new EventWorkspace
    outputWS = boost::dynamic_pointer_cast<EventWorkspace>(
        API::WorkspaceFactory::Instance().create("EventWorkspace", inputWS->getNumberHistograms(), 2, 1));
    //Copy geometry over.
    API::WorkspaceFactory::Instance().initializeFromParent(inputWS, outputWS, false);
    //outputWS->mutableSpectraMap().clear();
    //You need to copy over the data as well.
    outputWS->copyDataFrom( (*inputWS) );

    //Cast to the matrixOutputWS and save it
    matrixOutputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS);
    this->setProperty("OutputWorkspace", matrixOutputWS);
  }

  Kernel::Unit_sptr inputUnit = inputWS->getAxis(0)->unit();
  const std::string targetUnit = getPropertyValue("Target");

  // Set the final unit that our output workspace will have
  Kernel::Unit_const_sptr outputUnit = outputWS->getAxis(0)->unit() = UnitFactory::Instance().create(targetUnit);

  const int numberOfSpectra = inputWS->getNumberHistograms();
  this->convertViaEventsTOF(numberOfSpectra, inputUnit, outputWS);
}

/** Convert the workspace units using TOF as an intermediate step in the conversion
 * @param numberOfSpectra :: The number of Spectra
 * @param fromUnit :: The unit of the input workspace
 * @param outputWS :: The output workspace
 */
void ConvertUnits::convertViaEventsTOF(const int& numberOfSpectra, Kernel::Unit_const_sptr fromUnit, DataObjects::EventWorkspace_sptr outputWS)
{
  using namespace Geometry;
  
  Progress prog(this,0.0,1.0,numberOfSpectra);

  // Get a pointer to the instrument contained in the workspace
  IInstrument_const_sptr instrument = outputWS->getInstrument();
  // Get the parameter map
  const ParameterMap& pmap = outputWS->constInstrumentParameters();

  // Get the unit object for each workspace
  Kernel::Unit_const_sptr outputUnit = outputWS->getAxis(0)->unit();

  // Get the distance between the source and the sample (assume in metres)
  IObjComponent_const_sptr source = instrument->getSource();
  IObjComponent_const_sptr sample = instrument->getSample();
  if ( source == NULL || sample == NULL )
  {
    throw Exception::InstrumentDefinitionError("Instrument not sufficiently defined: failed to get source and/or sample");
  }
  double l1;
  try
  {
    l1 = source->getDistance(*sample);
    g_log.debug() << "Source-sample distance: " << l1 << std::endl;
  }
  catch (Exception::NotFoundError &)
  {
    g_log.error("Unable to calculate source-sample distance");
    throw Exception::InstrumentDefinitionError("Unable to calculate source-sample distance", outputWS->getTitle());
  }

  int failedDetectorCount = 0;

  /// @todo No implementation for any of these in the geometry yet so using properties
  const std::string emodeStr = getProperty("EMode");
  // Convert back to an integer representation
  int emode = 0;
  if (emodeStr == "Direct") emode=1;
  else if (emodeStr == "Indirect") emode=2;

  // Not doing anything with the Y vector in to/fromTOF yet, so just pass empty vector
  std::vector<double> emptyVec;
  const bool needEfixed = ( outputUnit->unitID().find("DeltaE") != std::string::npos );
  double efixedProp = getProperty("Efixed");
  if ( emode == 1 )
  {
    //... direct efixed gather
    if ( efixedProp == EMPTY_DBL() )
    {
      // try and get the value from the run parameters
      const API::Run & run = outputWS->run();
      if ( run.hasProperty("Ei") )
      {
        Kernel::Property* prop = run.getProperty("Ei");
        efixedProp = boost::lexical_cast<double,std::string>(prop->value());
      }
      else
      {
        if ( needEfixed )
        {
          throw std::invalid_argument("Could not retrieve incident energy from run object");
        }
        else
        {
          efixedProp = 0.0;
        }
      }
    }
    else
    {
      // set the Ei value in the run parameters
      API::Run & run = outputWS->mutableRun();
      run.addProperty<double>("Ei", efixedProp, true);
    }
  }
  else if ( emode == 0 && efixedProp == EMPTY_DBL() ) // Elastic
  {
    efixedProp = 0.0;
  }

  // Loop over the histograms (detector spectra)
  PARALLEL_FOR1(outputWS)
  for (int i = 0; i < numberOfSpectra; ++i)
  {
    PARALLEL_START_INTERUPT_REGION
    double efixed = efixedProp;
    /// @todo Don't yet consider hold-off (delta)
    const double delta = 0.0;

    try
    {
      // Now get the detector object for this histogram
      IDetector_sptr det = outputWS->getDetector(i);
      // Get the sample-detector distance for this detector (in metres)
      double l2, twoTheta;
      if ( ! det->isMonitor() )
      {
        l2 = det->getDistance(*sample);
        // The scattering angle for this detector (in radians).
        twoTheta = outputWS->detectorTwoTheta(det);
        // If an indirect instrument, try getting Efixed from the geometry
        if (emode==2)
        {
          if ( efixed == EMPTY_DBL() )
          {
          try
          {
            Parameter_sptr par = pmap.getRecursive(det->getComponent(),"Efixed");
            if (par)
            {
              efixed = par->value<double>();
              g_log.debug() << "Detector: " << det->getID() << " EFixed: " << efixed << "\n";
            }
          } 
          catch (std::runtime_error&) { /* Throws if a DetectorGroup, use single provided value */ }
          }
        }
      }
      else  // If this is a monitor then make l2 = source-detector distance, l1=0 and twoTheta=0
      {
        l2 = det->getDistance(*source);
        l2 = l2-l1;
        twoTheta = 0.0;
        // Energy transfer is meaningless for a monitor, so set l2 to 0.
        if (needEfixed)
        {
          l2 = 0.0;
          efixed = DBL_MIN;
        }
      }

      // Convert from time-of-flight to the desired unit
      std::vector<double> tofs;
      outputWS->getEventList(i).getTofs(tofs);
      fromUnit->toTOF(tofs,emptyVec,l1,l2,twoTheta,emode,efixed,delta);
      outputUnit->fromTOF(tofs,emptyVec,l1,l2,twoTheta,emode,efixed,delta);
      outputWS->getEventList(i).setTofs(tofs);

      // convert the cached x-values to the desired unit
      MantidVecPtr x = outputWS->refX(i);
      fromUnit->toTOF(x.access(),emptyVec,l1,l2,twoTheta,emode,efixed,delta);
      outputUnit->fromTOF(x.access(),emptyVec,l1,l2,twoTheta,emode,efixed,delta);
      outputWS->setX(i,x);

      // reverse the data if appropriate
      if ( !x->empty() && x->front() > x->back() )
      {
        outputWS->getEventList(i).reverse();
      }

    }
    catch (Exception::NotFoundError&)
    {
      // Get to here if exception thrown when calculating distance to detector
      failedDetectorCount++;
      outputWS->getEventList(i).clear();
    }

    prog.report();
    PARALLEL_END_INTERUPT_REGION
  } // loop over spectra
  PARALLEL_CHECK_INTERUPT_REGION

  outputWS->clearMRU();

  if (failedDetectorCount != 0)
  {
    g_log.information() << "Unable to calculate sample-detector distance for " << failedDetectorCount << " spectra. Zeroing spectrum." << std::endl;
  }

}

/** Convert the workspace units according to a simple output = a * (input^b) relationship
 * @param numberOfSpectra :: The number of Spectra
 * @param outputWS :: the output workspace
 * @param factor :: the conversion factor a to apply
 * @param power :: the Power b to apply to the conversion
 */
void ConvertUnits::convertQuickly(const int& numberOfSpectra, API::MatrixWorkspace_sptr outputWS, const double& factor, const double& power)
{
  // See if the workspace has common bins - if so the X vector can be common
  // First a quick check using the validator
  CommonBinsValidator<> sameBins;
  if ( sameBins.isValid(outputWS) == "" )
  {
    // Only do the full check if the quick one passes
    if ( WorkspaceHelpers::commonBoundaries(outputWS) )
    {
      // Calculate the new (common) X values
      MantidVec::iterator iter;
      for (iter = outputWS->dataX(0).begin(); iter != outputWS->dataX(0).end(); ++iter)
      {
        *iter = factor * std::pow(*iter,power);
      }
      // If this is a Workspace2D then loop over the other spectra passing in the pointer
      Workspace2D_sptr WS2D = boost::dynamic_pointer_cast<Workspace2D>(outputWS);
      if (WS2D)
      {
        MantidVecPtr xVals;
        xVals.access() = outputWS->dataX(0);
        Progress prog(this,0.5,1.0,numberOfSpectra);

        PARALLEL_FOR1(outputWS)
        for (int j = 1; j < numberOfSpectra; ++j)
        {
          PARALLEL_START_INTERUPT_REGION
          WS2D->setX(j,xVals);

          prog.report();
          PARALLEL_END_INTERUPT_REGION
        }
        PARALLEL_CHECK_INTERUPT_REGION
      }
      return;
    }
  }
  // If we get to here then the bins weren't aligned and each spectrum is unique
  // Loop over the histograms (detector spectra)
  PARALLEL_FOR1(outputWS)
  for (int k = 0; k < numberOfSpectra; ++k) {
    PARALLEL_START_INTERUPT_REGION
    MantidVec::iterator it;
    for (it = outputWS->dataX(k).begin(); it != outputWS->dataX(k).end(); ++it)
    {
      *it = factor * std::pow(*it,power);
    }
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  return;
}

/** Convert the workspace units using TOF as an intermediate step in the conversion
 * @param numberOfSpectra :: The number of Spectra
 * @param fromUnit :: The unit of the input workspace
 * @param outputWS :: The output workspace
 */
void ConvertUnits::convertViaTOF(const int& numberOfSpectra, Kernel::Unit_const_sptr fromUnit, API::MatrixWorkspace_sptr outputWS)
{
  using namespace Geometry;
  
  Progress prog(this,0.5,1.0,numberOfSpectra);

  // Get a pointer to the instrument contained in the workspace
  IInstrument_const_sptr instrument = outputWS->getInstrument();
  // Get the parameter map
  const ParameterMap& pmap = outputWS->constInstrumentParameters();

  // Get the unit object for each workspace
  Kernel::Unit_const_sptr outputUnit = outputWS->getAxis(0)->unit();

  // Get the distance between the source and the sample (assume in metres)
  IObjComponent_const_sptr source = instrument->getSource();
  IObjComponent_const_sptr sample = instrument->getSample();
  if ( source == NULL || sample == NULL )
  {
    throw Exception::InstrumentDefinitionError("Instrument not sufficiently defined: failed to get source and/or sample");
  }
  double l1;
  try
  {
    l1 = source->getDistance(*sample);
    g_log.debug() << "Source-sample distance: " << l1 << std::endl;
  }
  catch (Exception::NotFoundError &)
  {
    g_log.error("Unable to calculate source-sample distance");
    throw Exception::InstrumentDefinitionError("Unable to calculate source-sample distance", outputWS->getTitle());
  }

  int failedDetectorCount = 0;

  /// @todo No implementation for any of these in the geometry yet so using properties
  const std::string emodeStr = getProperty("EMode");
  // Convert back to an integer representation
  int emode = 0;
  if (emodeStr == "Direct") emode=1;
  else if (emodeStr == "Indirect") emode=2;

  // Not doing anything with the Y vector in to/fromTOF yet, so just pass empty vector
  std::vector<double> emptyVec;
  const bool needEfixed = ( outputUnit->unitID().find("DeltaE") != std::string::npos );
  double efixedProp = getProperty("Efixed");
  if ( emode == 1 )
  {
    //... direct efixed gather
    if ( efixedProp == EMPTY_DBL() )
    {
      // try and get the value from the run parameters
      const API::Run & run = outputWS->run();
      if ( run.hasProperty("Ei") )
      {
        Kernel::Property* prop = run.getProperty("Ei");
        efixedProp = boost::lexical_cast<double,std::string>(prop->value());
      }
      else
      {
        if ( needEfixed )
        {
        throw std::invalid_argument("Could not retrieve incident energy from run object");
        }
        else
        {
          efixedProp = 0.0;
        }
      }
    }
    else
    {
      // set the Ei value in the run parameters
      API::Run & run = outputWS->mutableRun();
      run.addProperty<double>("Ei", efixedProp, true);
    }
  }
  else if ( emode == 0 && efixedProp == EMPTY_DBL() ) // Elastic
  {
    efixedProp = 0.0;
  }

  // Loop over the histograms (detector spectra)
  PARALLEL_FOR1(outputWS)
  for (int i = 0; i < numberOfSpectra; ++i) 
  {
    PARALLEL_START_INTERUPT_REGION
    double efixed = efixedProp;
    /// @todo Don't yet consider hold-off (delta)
    const double delta = 0.0;

    try
    {
      // Now get the detector object for this histogram
      IDetector_sptr det = outputWS->getDetector(i);
      // Get the sample-detector distance for this detector (in metres)
      double l2, twoTheta;
      if ( ! det->isMonitor() )
      {
        l2 = det->getDistance(*sample);
        // The scattering angle for this detector (in radians).
        twoTheta = outputWS->detectorTwoTheta(det);
        // If an indirect instrument, try getting Efixed from the geometry
        if (emode==2) // indirect
        {
          if ( efixed == EMPTY_DBL() )
          {
          try
          {
            Parameter_sptr par = pmap.getRecursive(det->getComponent(),"Efixed");
            if (par) 
            {
              efixed = par->value<double>();
              g_log.debug() << "Detector: " << det->getID() << " EFixed: " << efixed << "\n";
            }
          }
          catch (std::runtime_error&) { /* Throws if a DetectorGroup, use single provided value */ }
          }
        }
      }
      else  // If this is a monitor then make l2 = source-detector distance, l1=0 and twoTheta=0
      {
        l2 = det->getDistance(*source);
        l2 = l2-l1;
        twoTheta = 0.0;
        // Energy transfer is meaningless for a monitor, so set l2 to 0.
        if (needEfixed)
        {
          l2 = 0.0;
          efixed = DBL_MIN;
        }
      }

      // Convert the input unit to time-of-flight
      fromUnit->toTOF(outputWS->dataX(i),emptyVec,l1,l2,twoTheta,emode,efixed,delta);
      // Convert from time-of-flight to the desired unit
      outputUnit->fromTOF(outputWS->dataX(i),emptyVec,l1,l2,twoTheta,emode,efixed,delta);

   } catch (Exception::NotFoundError&) {
      // Get to here if exception thrown when calculating distance to detector
      failedDetectorCount++;
      outputWS->dataX(i).assign(outputWS->dataX(i).size(),0.0);
      outputWS->dataY(i).assign(outputWS->dataY(i).size(),0.0);
      outputWS->dataE(i).assign(outputWS->dataE(i).size(),0.0);
    }

    prog.report();
    PARALLEL_END_INTERUPT_REGION
  } // loop over spectra
  PARALLEL_CHECK_INTERUPT_REGION

  if (failedDetectorCount != 0)
  {
    g_log.information() << "Unable to calculate sample-detector distance for " << failedDetectorCount << " spectra. Zeroing spectrum." << std::endl;
  }

}

/// Calls Rebin as a sub-algorithm to align the bins
API::MatrixWorkspace_sptr ConvertUnits::alignBins(API::MatrixWorkspace_sptr workspace)
{
  // Create a Rebin child algorithm
  IAlgorithm_sptr childAlg = createSubAlgorithm("Rebin");
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", workspace);
  childAlg->setProperty<std::vector<double> >("Params",this->calculateRebinParams(workspace));

  // Now execute the sub-algorithm. Catch and log any error
  try
  {
    childAlg->execute();
  }
  catch (std::runtime_error&)
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

/// Checks if a double is not infinity or a NaN
bool isANumber(const double& d)
{
  return d == d && fabs(d) != std::numeric_limits<double>::infinity();
}

/// The Rebin parameters should cover the full range of the converted unit, with the same number of bins
const std::vector<double> ConvertUnits::calculateRebinParams(const API::MatrixWorkspace_const_sptr workspace) const
{
  // Need to loop round and find the full range
  double XMin = DBL_MAX, XMax = DBL_MIN;
  const int numSpec = workspace->getNumberHistograms();
  for (int i = 0; i < numSpec; ++i)
  {
    try {
      Geometry::IDetector_const_sptr det = workspace->getDetector(i);
      if ( !det->isMasked() )
      {
        const MantidVec & XData = workspace->readX(i);
        double xfront = XData.front();
        double xback = XData.back();
        if (isANumber(xfront) && isANumber(xback))
        {
          if ( xfront < XMin ) XMin = xfront;
          if ( xback > XMax )  XMax = xback;
        }
      }
    } catch (Exception::NotFoundError &) {} //Do nothing
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
        MantidVecPtr xVals;
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

/** Unwieldy method which removes bins which lie in a physically inaccessible region.
 *  This presently only occurs in conversions to energy transfer, where the initial
 *  unit conversion sets them to +/-DBL_MAX. This method removes those bins, leading
 *  to a workspace which is smaller than the input one.
 *  As presently implemented, it unfortunately requires testing for and knowledge of
 *  aspects of the particular units conversion instead of keeping all that in the
 *  units class. It could be made more general, but that would be less efficient.
 *  @param workspace :: The workspace after initial unit conversion
 *  @return The workspace after bins have been removed
 */
API::MatrixWorkspace_sptr ConvertUnits::removeUnphysicalBins(const Mantid::API::MatrixWorkspace_const_sptr workspace)
{
  MatrixWorkspace_sptr result;
  // If this is a Workspace2D, get the spectra axes for copying in the spectraNo later
  Axis *specAxis = NULL, *outAxis = NULL;
  if (workspace->axes() > 1) specAxis = workspace->getAxis(1);

  const int numSpec = workspace->getNumberHistograms();
  const std::string emode = getProperty("Emode");
  if (emode=="Direct")
  {
    // First the easy case of direct instruments, where all spectra will need the
    // same number of bins removed
    // Need to make sure we don't pick a monitor as the 'reference' X spectrum (X0)
    int i = 0;
    for ( ; i < numSpec; ++i )
    {
      try {
        Geometry::IDetector_const_sptr det = workspace->getDetector(i);
        if ( !det->isMonitor() ) break;
      } catch (Exception::NotFoundError &) { /* Do nothing */ }
    }
    // Get an X spectrum to search (they're all the same, monitors excepted)
    const MantidVec& X0 = workspace->readX(i);
    MantidVec::const_iterator start = std::lower_bound(X0.begin(),X0.end(),-1.0e-10*DBL_MAX);
    if ( start == X0.end() )
    {
      const std::string e("Check the input EFixed: the one given leads to all bins being in the physically inaccessible region.");
      g_log.error(e);
      throw std::invalid_argument(e);
    }
    MantidVec::difference_type bins = X0.end() - start;
    MantidVec::difference_type first = start - X0.begin();

    result = WorkspaceFactory::Instance().create(workspace,numSpec,bins,bins-1);
    if (specAxis) outAxis = result->getAxis(1);

    for (int i = 0; i < numSpec; ++i)
    {
      const MantidVec& X = workspace->readX(i);
      const MantidVec& Y = workspace->readY(i);
      const MantidVec& E = workspace->readE(i);
      result->dataX(i).assign(X.begin()+first,X.end());
      result->dataY(i).assign(Y.begin()+first,Y.end());
      result->dataE(i).assign(E.begin()+first,E.end());
      if (specAxis) outAxis->spectraNo(i) = specAxis->spectraNo(i);
    }
  }
  else if (emode=="Indirect") 
  {
    // Now the indirect instruments. In this case we could want to keep a different
    // number of bins in each spectrum because, in general L2 is different for each
    // one.
    // Thus, we first need to loop to find largest 'good' range
    std::vector<MantidVec::difference_type> lastBins(numSpec);
    int maxBins = 0;
    for (int i = 0; i < numSpec; ++i)
    {
      const MantidVec& X = workspace->readX(i);
      MantidVec::const_iterator end = std::lower_bound(X.begin(),X.end(),1.0e-10*DBL_MAX);
      MantidVec::difference_type bins = end - X.begin();
      lastBins[i] = bins;
      if (bins > maxBins) maxBins = bins;
    }
    g_log.debug() << maxBins << std::endl;
    // Now create an output workspace large enough for the longest 'good' range
    result = WorkspaceFactory::Instance().create(workspace,numSpec,maxBins,maxBins-1);
    if (specAxis) outAxis = result->getAxis(1);
    // Next, loop again copying in the correct range for each spectrum
    for (int j = 0; j < numSpec; ++j)
    {
      const MantidVec& X = workspace->readX(j);
      const MantidVec& Y = workspace->readY(j);
      const MantidVec& E = workspace->readE(j);
      MantidVec& Xnew = result->dataX(j);
      MantidVec& Ynew = result->dataY(j);
      MantidVec& Enew = result->dataE(j);
      int k;
      for (k = 0; k < lastBins[j]-1; ++k)
      {
        Xnew[k] = X[k];
        Ynew[k] = Y[k];
        Enew[k] = E[k];
      }
      Xnew[k] = X[k];
      ++k;
      // If necessary, add on some fake values to the end of the X array (Y&E will be zero)
      if (k < maxBins)
      {
        for (int l=k; l < maxBins; ++l)
        {
          Xnew[l] = X[k]+1+l-k;
        }
      }
      if (specAxis) outAxis->spectraNo(j) = specAxis->spectraNo(j);
    }
  }

  return result;
}

} // namespace Algorithm
} // namespace Mantid
