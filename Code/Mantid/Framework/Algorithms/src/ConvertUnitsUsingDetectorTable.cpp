//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ConvertUnitsUsingDetectorTable.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <cfloat>
#include <iostream>
#include <limits>
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid
{
namespace Algorithms
{

// Register with the algorithm factory
DECLARE_ALGORITHM(ConvertUnitsUsingDetectorTable)

using namespace Kernel;
using namespace API;
using namespace DataObjects;
using boost::function;
using boost::bind;

/// Default constructor
ConvertUnitsUsingDetectorTable::ConvertUnitsUsingDetectorTable() : Algorithm(), m_numberOfSpectra(0), m_inputEvents(false)
{
}

/// Destructor
ConvertUnitsUsingDetectorTable::~ConvertUnitsUsingDetectorTable()
{
}

/// Initialisation method
void ConvertUnitsUsingDetectorTable::init()
{
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>();
  wsValidator->add<HistogramValidator>();
  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace","",Direction::Input,wsValidator),
    "Name of the input workspace");
  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace","",Direction::Output),
    "Name of the output workspace, can be the same as the input" );

  // Extract the current contents of the UnitFactory to be the allowed values of the Target property
  declareProperty("Target","",boost::make_shared<StringListValidator>(UnitFactory::Instance().getKeys()),
    "The name of the units to convert to (must be one of those registered in\n"
    "the Unit Factory)");
  std::vector<std::string> propOptions;
  propOptions.push_back("Elastic");
  propOptions.push_back("Direct");
  propOptions.push_back("Indirect");
  declareProperty("EMode","Elastic",boost::make_shared<StringListValidator>(propOptions),
    "The energy mode (default: elastic)");
  auto mustBePositive = boost::make_shared<BoundedValidator<double> >();
  mustBePositive->setLower(0.0);
  declareProperty("EFixed",EMPTY_DBL(),mustBePositive,
    "Value of fixed energy in meV : EI (EMode=Direct) or EF (EMode=Indirect) . Must be\n"
    "set if the target unit requires it (e.g. DeltaE)");

  declareProperty("AlignBins",false,
    "If true (default is false), rebins after conversion to ensure that all spectra in the output workspace\n"
    "have identical bin boundaries. This option is not recommended (see http://www.mantidproject.org/ConvertUnits).");

  declareProperty(new WorkspaceProperty<ITableWorkspace>("DetectorParameters", "", Direction::Input, PropertyMode::Optional),
    "Name of a TableWorkspace containing the detector parameters to use instead of the IDF.");
}

/** Executes the algorithm
 *  @throw std::runtime_error If the input workspace has not had its unit set
 *  @throw NotImplementedError If the input workspace contains point (not histogram) data
 *  @throw InstrumentDefinitionError If unable to calculate source-sample distance
 */
void ConvertUnitsUsingDetectorTable::exec()
{
  // Get the workspaces
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  this->setupMemberVariables(inputWS);

  // Check that the input workspace doesn't already have the desired unit.
  if (m_inputUnit->unitID() == m_outputUnit->unitID())
  {
    const std::string outputWSName = getPropertyValue("OutputWorkspace");
    const std::string inputWSName = getPropertyValue("InputWorkspace");
    if (outputWSName == inputWSName)
    {
      // If it does, just set the output workspace to point to the input one and be done.
      g_log.information() << "Input workspace already has target unit (" << m_outputUnit->unitID() << "), so just pointing the output workspace property to the input workspace."<< std::endl;
      setProperty("OutputWorkspace", boost::const_pointer_cast<MatrixWorkspace>(inputWS));
      return;
    }
    else
    {
      // Clone the workspace.
      IAlgorithm_sptr duplicate = createChildAlgorithm("CloneWorkspace",0.0,0.6);
      duplicate->initialize();
      duplicate->setProperty("InputWorkspace",  inputWS);
      duplicate->execute();
      Workspace_sptr temp = duplicate->getProperty("OutputWorkspace");
      auto outputWs = boost::dynamic_pointer_cast<MatrixWorkspace>(temp);
      setProperty("OutputWorkspace", outputWs);
      return;
    }

  }

  if (inputWS->dataX(0).size() < 2)
  {
    std::stringstream msg;
    msg << "Input workspace has invalid X axis binning parameters. Should have at least 2 values. Found "
        << inputWS->dataX(0).size() << ".";
    throw std::runtime_error(msg.str());
  }
  if (   inputWS->dataX(0).front() > inputWS->dataX(0).back()
      || inputWS->dataX(m_numberOfSpectra/2).front() > inputWS->dataX(m_numberOfSpectra/2).back())
    throw std::runtime_error("Input workspace has invalid X axis binning parameters. X values should be increasing.");

  MatrixWorkspace_sptr outputWS = this->setupOutputWorkspace(inputWS);

  // Check whether there is a quick conversion available
  double factor, power;
  if ( m_inputUnit->quickConversion(*m_outputUnit,factor,power) )
  // If test fails, could also check whether a quick conversion in the opposite direction has been entered
  {
    this->convertQuickly(outputWS,factor,power);
  }
  else
  {
    this->convertViaTOF(m_inputUnit,outputWS);
  }

  // If the units conversion has flipped the ascending direction of X, reverse all the vectors
  if (outputWS->dataX(0).size() && ( outputWS->dataX(0).front() > outputWS->dataX(0).back()
        || outputWS->dataX(m_numberOfSpectra/2).front() > outputWS->dataX(m_numberOfSpectra/2).back() ) )
  {
    this->reverse(outputWS);
  }

  // Need to lop bins off if converting to energy transfer.
  // Don't do for EventWorkspaces, where you can easily rebin to recover the situation without losing information
  /* This is an ugly test - could be made more general by testing for DBL_MAX
     values at the ends of all spectra, but that would be less efficient */
  if ( m_outputUnit->unitID().find("Delta")==0 && !m_inputEvents ) outputWS = this->removeUnphysicalBins(outputWS);

  // Rebin the data to common bins if requested, and if necessary
  bool alignBins = getProperty("AlignBins");
  if (alignBins && !WorkspaceHelpers::commonBoundaries(outputWS)) 
    outputWS = this->alignBins(outputWS);

  // If appropriate, put back the bin width division into Y/E.
  if (m_distribution && !m_inputEvents)  // Never do this for event workspaces
  {
    this->putBackBinWidth(outputWS);
  }

  // Point the output property to the right place.
  // Do right at end (workspace could could change in removeUnphysicalBins or alignBins methods)
  setProperty("OutputWorkspace",outputWS);
  return;
}

/** Initialise the member variables
 *  @param inputWS The input workspace
 */
void ConvertUnitsUsingDetectorTable::setupMemberVariables(const API::MatrixWorkspace_const_sptr inputWS)
{
  m_numberOfSpectra = inputWS->getNumberHistograms();
  // In the context of this algorithm, we treat things as a distribution if the flag is set
  // AND the data are not dimensionless
  m_distribution = inputWS->isDistribution() && !inputWS->YUnit().empty();
  //Check if its an event workspace
  m_inputEvents = ( boost::dynamic_pointer_cast<const EventWorkspace>(inputWS) != NULL );

  m_inputUnit = inputWS->getAxis(0)->unit();
  const std::string targetUnit = getPropertyValue("Target");
  m_outputUnit = UnitFactory::Instance().create(targetUnit);
}

/** Create an output workspace of the appropriate (histogram or event) type and copy over the data
 *  @param inputWS The input workspace
 */
API::MatrixWorkspace_sptr ConvertUnitsUsingDetectorTable::setupOutputWorkspace(const API::MatrixWorkspace_const_sptr inputWS)
{
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");

  // If input and output workspaces are NOT the same, create a new workspace for the output
  if (outputWS != inputWS )
  {
    if ( m_inputEvents )
    {
      // Need to create by name as WorkspaceFactory otherwise spits out Workspace2D when EventWS passed in
      outputWS = WorkspaceFactory::Instance().create("EventWorkspace", inputWS->getNumberHistograms(), 2, 1);
      // Copy geometry etc. over
      WorkspaceFactory::Instance().initializeFromParent(inputWS, outputWS, false);
      // Need to copy over the data as well
      EventWorkspace_const_sptr inputEventWS = boost::dynamic_pointer_cast<const EventWorkspace>(inputWS);
      boost::dynamic_pointer_cast<EventWorkspace>(outputWS)->copyDataFrom( *inputEventWS );
    }
    else
    {
      // Create the output workspace
      outputWS = WorkspaceFactory::Instance().create(inputWS);
      // Copy the data over
      this->fillOutputHist(inputWS, outputWS);
    }
  }

  // Set the final unit that our output workspace will have
  outputWS->getAxis(0)->unit() = m_outputUnit;

  return outputWS;
}

/** Do the initial copy of the data from the input to the output workspace for histogram workspaces.
 *  Takes out the bin width if necessary.
 *  @param inputWS  The input workspace
 *  @param outputWS The output workspace
 */
void ConvertUnitsUsingDetectorTable::fillOutputHist(const API::MatrixWorkspace_const_sptr inputWS, const API::MatrixWorkspace_sptr outputWS)
{
  const int size = static_cast<int>(inputWS->blocksize());

  // Loop over the histograms (detector spectra)
  Progress prog(this,0.0,0.2,m_numberOfSpectra);
  int64_t numberOfSpectra_i = static_cast<int64_t>(m_numberOfSpectra); // cast to make openmp happy
  PARALLEL_FOR2(inputWS,outputWS)
  for (int64_t i = 0; i < numberOfSpectra_i; ++i)
  {
    PARALLEL_START_INTERUPT_REGION
    // Take the bin width dependency out of the Y & E data
    if (m_distribution)
    {
      for (int j = 0; j < size; ++j)
      {
        const double width = std::abs( inputWS->dataX(i)[j+1] - inputWS->dataX(i)[j] );
        outputWS->dataY(i)[j] = inputWS->dataY(i)[j]*width;
        outputWS->dataE(i)[j] = inputWS->dataE(i)[j]*width;
      }
    }
    else
    {
      // Just copy over
      outputWS->dataY(i) = inputWS->readY(i);
      outputWS->dataE(i) = inputWS->readE(i);
    }
    // Copy over the X data
    outputWS->setX( i, inputWS->refX(i) );

    prog.report("Convert to " + m_outputUnit->unitID());
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}

/** Convert the workspace units according to a simple output = a * (input^b) relationship
 *  @param outputWS :: the output workspace
 *  @param factor :: the conversion factor a to apply
 *  @param power :: the Power b to apply to the conversion
 */
void ConvertUnitsUsingDetectorTable::convertQuickly(API::MatrixWorkspace_sptr outputWS, const double& factor, const double& power)
{
  Progress prog(this,0.2,1.0,m_numberOfSpectra);
  int64_t numberOfSpectra_i = static_cast<int64_t>(m_numberOfSpectra); // cast to make openmp happy

  // See if the workspace has common bins - if so the X vector can be common
  // First a quick check using the validator
  CommonBinsValidator sameBins;
  bool commonBoundaries = false;
  if ( sameBins.isValid(outputWS) == "" )
  {
    commonBoundaries =  WorkspaceHelpers::commonBoundaries(outputWS);
    // Only do the full check if the quick one passes
    if (commonBoundaries)
    {
      // Calculate the new (common) X values
      MantidVec::iterator iter;
      for (iter = outputWS->dataX(0).begin(); iter != outputWS->dataX(0).end(); ++iter)
      {
        *iter = factor * std::pow(*iter,power);
      }

      MantidVecPtr xVals;
      xVals.access() = outputWS->dataX(0);

      PARALLEL_FOR1(outputWS)
      for (int64_t j = 1; j < numberOfSpectra_i; ++j)
      {
        PARALLEL_START_INTERUPT_REGION
        outputWS->setX(j,xVals);
        prog.report("Convert to " + m_outputUnit->unitID());
        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION
      if (!m_inputEvents) // if in event mode the work is done
        return;
    }
  }

  EventWorkspace_sptr eventWS = boost::dynamic_pointer_cast<EventWorkspace>(outputWS);
  assert ( static_cast<bool>(eventWS) == m_inputEvents ); // Sanity check

  // If we get to here then the bins weren't aligned and each spectrum is unique
  // Loop over the histograms (detector spectra)
  PARALLEL_FOR1(outputWS)
  for (int64_t k = 0; k < numberOfSpectra_i; ++k) {
    PARALLEL_START_INTERUPT_REGION
    if (!commonBoundaries) {
      MantidVec::iterator it;
      for (it = outputWS->dataX(k).begin(); it != outputWS->dataX(k).end(); ++it)
      {
        *it = factor * std::pow(*it,power);
      }
    }
    // Convert the events themselves if necessary. Inefficiently.
    if ( m_inputEvents )
    {
      eventWS->getEventList(k).convertUnitsQuickly(factor, power);

//      std::vector<double> tofs;
//      eventWS->getEventList(k).getTofs(tofs);
//      std::vector<double>::iterator tofIt;
//      for (tofIt = tofs.begin(); tofIt != tofs.end(); ++tofIt)
//      {
//        *tofIt = factor * std::pow(*tofIt,power);
//      }
//      eventWS->getEventList(k).setTofs(tofs);
    }
    prog.report("Convert to " + m_outputUnit->unitID());
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  if (m_inputEvents)
    eventWS->clearMRU();
  return;
}

/** Convert the workspace units using TOF as an intermediate step in the conversion
 * @param fromUnit :: The unit of the input workspace
 * @param outputWS :: The output workspace
 */
void ConvertUnitsUsingDetectorTable::convertViaTOF(Kernel::Unit_const_sptr fromUnit, API::MatrixWorkspace_sptr outputWS)
{
  using namespace Geometry;

    // Let's see if we are using a TableWorkspace to override parameters
    ITableWorkspace_sptr paramWS = getProperty("DetectorParameters");

    // Some
    bool usingDetPars = false;
    bool usingDetParsL1 = false;
    Column_const_sptr l1Column;
    Column_const_sptr l2Column;
    Column_const_sptr spectraColumn;
    Column_const_sptr twoThetaColumn;
    Column_const_sptr efixedColumn;
    Column_const_sptr emodeColumn;

    // See if we have supplied a DetectorParameters Workspace
    if ( paramWS != NULL )
    {
        g_log.debug("Setting usingDetPars = true");
        usingDetPars = true;

        std::vector<std::string> columnNames = paramWS->getColumnNames();
        
        // First lets see if the table includes L1 ?
        
        if (std::find(columnNames.begin(), columnNames.end(), "l1") != columnNames.end())
        {
            try {
                l1Column = paramWS->getColumn("l1");
                usingDetParsL1 = true;
                g_log.debug() << "Overriding L1 from IDF with parameter table." << std::endl;
                
            } catch (std::runtime_error) {
                // make sure we know we are using L1 from the IDF
                usingDetParsL1 = false;
                g_log.debug() << "Could not find L1 in parameter table supplied - using values from IDF." << std::endl;
            }
        }
        else
        {
            usingDetParsL1 = false;
            g_log.debug() << "Could not find L1 in parameter table supplied - using values from IDF." << std::endl;;
            
        }

        // Now lets read the rest of the parameters
        try {
            l2Column = paramWS->getColumn("l2");
            spectraColumn = paramWS->getColumn("spectra");
            twoThetaColumn = paramWS->getColumn("twotheta");
            efixedColumn = paramWS->getColumn("efixed");
            emodeColumn = paramWS->getColumn("emode");
        } catch (...) {
            usingDetPars = false;
            throw Exception::InstrumentDefinitionError("DetectorParameter TableWorkspace is not defined correctly.");
        }

    }

  EventWorkspace_sptr eventWS = boost::dynamic_pointer_cast<EventWorkspace>(outputWS);
  assert ( static_cast<bool>(eventWS) == m_inputEvents ); // Sanity check

  Progress prog(this,0.2,1.0,m_numberOfSpectra);
  int64_t numberOfSpectra_i = static_cast<int64_t>(m_numberOfSpectra); // cast to make openmp happy

  // Get a pointer to the instrument contained in the workspace
  Instrument_const_sptr instrument = outputWS->getInstrument();
  // Get the parameter map
  const ParameterMap& pmap = outputWS->constInstrumentParameters();

  // Get the unit object for each workspace
  Kernel::Unit_const_sptr outputUnit = outputWS->getAxis(0)->unit();

  // Get the distance between the source and the sample (assume in metres)
  IComponent_const_sptr source = instrument->getSource();
  IComponent_const_sptr sample = instrument->getSample();
    
    if (!usingDetPars)
    {
        if ( source == NULL || sample == NULL )
        {
            
            throw Exception::InstrumentDefinitionError("Instrument not sufficiently defined: failed to get source and/or sample");
        }
    }
    
    double l1;
    int emode = 0;
    double l2, twoTheta, efixed;
    double efixedProp;
    
    std::vector<double> emptyVec;
    int failedDetectorCount = 0;

    if (!usingDetPars)
    {
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
        
        /// @todo No implementation for any of these in the geometry yet so using properties
        const std::string emodeStr = getProperty("EMode");
        // Convert back to an integer representation
        if (emodeStr == "Direct") emode=1;
        else if (emodeStr == "Indirect") emode=2;
        
        // Not doing anything with the Y vector in to/fromTOF yet, so just pass empty vector
        const bool needEfixed = ( outputUnit->unitID().find("DeltaE") != std::string::npos || outputUnit->unitID().find("Wave") != std::string::npos );
        efixedProp = getProperty("Efixed");
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
    }
    
        std::vector<std::string> parameters = outputWS->getInstrument()->getStringParameter("show-signed-theta");
        bool bUseSignedVersion = (!parameters.empty()) && find(parameters.begin(), parameters.end(), "Always") != parameters.end();
        function<double(IDetector_const_sptr)> thetaFunction = bUseSignedVersion ? bind(&MatrixWorkspace::detectorSignedTwoTheta, outputWS, _1) : bind(&MatrixWorkspace::detectorTwoTheta, outputWS, _1);
    
    
  // Loop over the histograms (detector spectra)
  PARALLEL_FOR1(outputWS)
    for (int64_t i = 0; i < numberOfSpectra_i; ++i)
  {
    PARALLEL_START_INTERUPT_REGION
    efixed = efixedProp;

    std::size_t wsid = i;

    try
    {
        // Are we using a Detector Parameter workspace to override values
        if (usingDetPars)
        {
            specid_t spectraNumber = static_cast<specid_t>(spectraColumn->toDouble(i));
            wsid = outputWS->getIndexFromSpectrumNumber(spectraNumber);
            g_log.debug() << "###### Spectra #" << spectraNumber << " ==> Workspace ID:" << wsid << std::endl;
            l2 = l2Column->toDouble(wsid);
            twoTheta = twoThetaColumn->toDouble(wsid);
            efixed = efixedColumn->toDouble(wsid);
            emode = static_cast<int>(emodeColumn->toDouble(wsid));
            if (usingDetParsL1)
            {
                l1 = l1Column->toDouble(wsid);
            }
        }
        else
        {
            // Now get the detector object for this histogram
            IDetector_const_sptr det = outputWS->getDetector(i);
            // Get the sample-detector distance for this detector (in metres)
            if ( ! det->isMonitor() )
            {
                
                l2 = det->getDistance(*sample);
                // The scattering angle for this detector (in radians).
                twoTheta = thetaFunction(det);
                // If an indirect instrument, try getting Efixed from the geometry
                if (emode==2) // indirect
                {
                    if ( efixed == EMPTY_DBL() )
                    {
                        try
                        {
                            Parameter_sptr par = pmap.getRecursive(det.get(),"Efixed");
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
            else  // If this is a monitor then make l1+l2 = source-detector distance and twoTheta=0
            {
                l2 = det->getDistance(*source);
                l2 = l2-l1;
                twoTheta = 0.0;
                efixed = DBL_MIN;
                // Energy transfer is meaningless for a monitor, so set l2 to 0.
                if (outputUnit->unitID().find("DeltaE") != std::string::npos)
                {
                    l2 = 0.0;
                }
            }
        }

      // Make local copies of the units. This allows running the loop in parallel
      Unit * localFromUnit = fromUnit->clone();
      Unit * localOutputUnit = outputUnit->clone();

      /// @todo Don't yet consider hold-off (delta)
      const double delta = 0.0;
      // Convert the input unit to time-of-flight
      localFromUnit->toTOF(outputWS->dataX(wsid),emptyVec,l1,l2,twoTheta,emode,efixed,delta);
      // Convert from time-of-flight to the desired unit
      localOutputUnit->fromTOF(outputWS->dataX(wsid),emptyVec,l1,l2,twoTheta,emode,efixed,delta);

      // EventWorkspace part, modifying the EventLists.
      if ( m_inputEvents )
      {
        eventWS->getEventList(wsid).convertUnitsViaTof(localFromUnit, localOutputUnit);

//        std::vector<double> tofs;
//        eventWS->getEventList(i).getTofs(tofs);
//        localFromUnit->toTOF(tofs,emptyVec,l1,l2,twoTheta,emode,efixed,delta);
//        localOutputUnit->fromTOF(tofs,emptyVec,l1,l2,twoTheta,emode,efixed,delta);
//        eventWS->getEventList(i).setTofs(tofs);
      }
      // Clear unit memory
      delete localFromUnit;
      delete localOutputUnit;

    } catch (Exception::NotFoundError&) {
      // Get to here if exception thrown when calculating distance to detector
      failedDetectorCount++;
      // Since you usually (always?) get to here when there's no attached detectors, this call is
      // the same as just zeroing out the data (calling clearData on the spectrum)
      outputWS->maskWorkspaceIndex(i);
    }

    prog.report("Convert to " + m_outputUnit->unitID());
    PARALLEL_END_INTERUPT_REGION
  } // loop over spectra
  PARALLEL_CHECK_INTERUPT_REGION

  if (failedDetectorCount != 0)
  {
    g_log.information() << "Unable to calculate sample-detector distance for " << failedDetectorCount << " spectra. Masking spectrum." << std::endl;
  }
  if (m_inputEvents)
    eventWS->clearMRU();
}






/// Calls Rebin as a Child Algorithm to align the bins
API::MatrixWorkspace_sptr ConvertUnitsUsingDetectorTable::alignBins(API::MatrixWorkspace_sptr workspace)
{
  // Create a Rebin child algorithm
  IAlgorithm_sptr childAlg = createChildAlgorithm("Rebin");
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", workspace);
  // Next line for EventWorkspaces - needed for as long as in/out set same keeps as events.
  childAlg->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", workspace);
  childAlg->setProperty<std::vector<double> >("Params",this->calculateRebinParams(workspace));
  childAlg->executeAsChildAlg();
  return childAlg->getProperty("OutputWorkspace");
}



/// The Rebin parameters should cover the full range of the converted unit, with the same number of bins
const std::vector<double> ConvertUnitsUsingDetectorTable::calculateRebinParams(const API::MatrixWorkspace_const_sptr workspace) const
{
  // Need to loop round and find the full range
  double XMin = DBL_MAX, XMax = DBL_MIN;
  const size_t numSpec = workspace->getNumberHistograms();
  for (size_t i = 0; i < numSpec; ++i)
  {
    try {
      Geometry::IDetector_const_sptr det = workspace->getDetector(i);
      if ( !det->isMasked() )
      {
        const MantidVec & XData = workspace->readX(i);
        double xfront = XData.front();
        double xback = XData.back();
        if (boost::math::isfinite(xfront) && boost::math::isfinite(xback))
        {
          if ( xfront < XMin ) XMin = xfront;
          if ( xback > XMax )  XMax = xback;
        }
      }
    } catch (Exception::NotFoundError &) {} //Do nothing
  }
  const double step = ( XMax - XMin ) / static_cast<double>(workspace->blocksize());

  std::vector<double> retval;
  retval.push_back(XMin);
  retval.push_back(step);
  retval.push_back(XMax);

  return retval;
}

/** Reverses the workspace if X values are in descending order
 *  @param WS The workspace to operate on
 */
void ConvertUnitsUsingDetectorTable::reverse(API::MatrixWorkspace_sptr WS)
{
  if ( WorkspaceHelpers::commonBoundaries(WS) && !m_inputEvents )
  {
    std::reverse(WS->dataX(0).begin(),WS->dataX(0).end());
    std::reverse(WS->dataY(0).begin(),WS->dataY(0).end());
    std::reverse(WS->dataE(0).begin(),WS->dataE(0).end());

    MantidVecPtr xVals;
    xVals.access() = WS->dataX(0);
    for (size_t j = 1; j < m_numberOfSpectra; ++j)
    {
      WS->setX(j,xVals);
      std::reverse(WS->dataY(j).begin(),WS->dataY(j).end());
      std::reverse(WS->dataE(j).begin(),WS->dataE(j).end());
      if ( j % 100 == 0) interruption_point();
    }
  }
  else
  {
    EventWorkspace_sptr eventWS = boost::dynamic_pointer_cast<EventWorkspace>(WS);
    assert ( static_cast<bool>(eventWS) == m_inputEvents ); // Sanity check

    int m_numberOfSpectra_i = static_cast<int>(m_numberOfSpectra);
    PARALLEL_FOR1(WS)
    for (int j = 0; j < m_numberOfSpectra_i; ++j)
    {
      PARALLEL_START_INTERUPT_REGION
      if ( m_inputEvents )
      {
        eventWS->getEventList(j).reverse();
      }
      else
      {
        std::reverse(WS->dataX(j).begin(),WS->dataX(j).end());
        std::reverse(WS->dataY(j).begin(),WS->dataY(j).end());
        std::reverse(WS->dataE(j).begin(),WS->dataE(j).end());
      }
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
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
API::MatrixWorkspace_sptr ConvertUnitsUsingDetectorTable::removeUnphysicalBins(const Mantid::API::MatrixWorkspace_const_sptr workspace)
{
  MatrixWorkspace_sptr result;

  const size_t numSpec = workspace->getNumberHistograms();
  const std::string emode = getProperty("Emode");
  if (emode=="Direct")
  {
    // First the easy case of direct instruments, where all spectra will need the
    // same number of bins removed
    // Need to make sure we don't pick a monitor as the 'reference' X spectrum (X0)
    size_t i = 0;
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

    for (size_t i = 0; i < numSpec; ++i)
    {
      const MantidVec& X = workspace->readX(i);
      const MantidVec& Y = workspace->readY(i);
      const MantidVec& E = workspace->readE(i);
      result->dataX(i).assign(X.begin()+first,X.end());
      result->dataY(i).assign(Y.begin()+first,Y.end());
      result->dataE(i).assign(E.begin()+first,E.end());
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
    for (size_t i = 0; i < numSpec; ++i)
    {
      const MantidVec& X = workspace->readX(i);
      MantidVec::const_iterator end = std::lower_bound(X.begin(),X.end(),1.0e-10*DBL_MAX);
      MantidVec::difference_type bins = end - X.begin();
      lastBins[i] = bins;
      if (bins > maxBins) maxBins = static_cast<int>(bins);
    }
    g_log.debug() << maxBins << std::endl;
    // Now create an output workspace large enough for the longest 'good' range
    result = WorkspaceFactory::Instance().create(workspace,numSpec,maxBins,maxBins-1);
    // Next, loop again copying in the correct range for each spectrum
    for (int64_t j = 0; j < int64_t(numSpec); ++j)
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
    }
  }

  return result;
}

/** Divide by the bin width if workspace is a distribution
 *  @param outputWS The workspace to operate on
 */
void ConvertUnitsUsingDetectorTable::putBackBinWidth(const API::MatrixWorkspace_sptr outputWS)
{
  const size_t outSize = outputWS->blocksize();

  for (size_t i = 0; i < m_numberOfSpectra; ++i)
  {
    for (size_t j = 0; j < outSize; ++j)
    {
      const double width = std::abs( outputWS->dataX(i)[j+1] - outputWS->dataX(i)[j] );
      outputWS->dataY(i)[j] = outputWS->dataY(i)[j]/width;
      outputWS->dataE(i)[j] = outputWS->dataE(i)[j]/width;
    }
  }
}


} // namespace Algorithm
} // namespace Mantid
