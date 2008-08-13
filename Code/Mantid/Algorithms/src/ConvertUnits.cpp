//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/UnitFactory.h"

namespace Mantid
{
namespace Algorithms
{

// Register with the algorithm factory
DECLARE_NAMESPACED_ALGORITHM(Mantid::Algorithms,ConvertUnits)

using namespace Kernel;
using namespace API;

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
  declareProperty(new WorkspaceProperty<API::Workspace>("InputWorkspace","",Direction::Input));
  declareProperty(new WorkspaceProperty<API::Workspace>("OutputWorkspace","",Direction::Output));

  // Extract the current contents of the UnitFactory to be the allowed values of the Target property
  declareProperty("Target","",new ListValidator(UnitFactory::Instance().getKeys()) );
  declareProperty("Emode",0);
  declareProperty("Efixed",0.0);
}

/** Executes the algorithm
 *  @throw std::runtime_error If the input workspace has not had its unit set
 *  @throw NotImplementedError If the input workspace contains point (not histogram) data
 *  @throw InstrumentDefinitionError If unable to calculate source-sample distance
 */
void ConvertUnits::exec()
{
  // Get the workspace
  API::Workspace_const_sptr inputWS = getProperty("InputWorkspace");

  // Check that the workspace is histogram data
  if (inputWS->dataX(0).size() == inputWS->dataY(0).size())
  {
    g_log.error("Conversion of units for point data is not yet implemented");
    throw Exception::NotImplementedError("Conversion of units for point data is not yet implemented");
  }

  // Check that the input workspace has had its unit set
  boost::shared_ptr<Unit> inputUnit = inputWS->getAxis(0)->unit();
  if ( ! inputUnit )
  {
    g_log.error("Input workspace has not had its unit set");
    throw std::runtime_error("Input workspace has not had its unit set");
  }

  API::Workspace_sptr outputWS = getProperty("OutputWorkspace");
  // If input and output workspaces are not the same, create a new workspace for the output
  if (outputWS != inputWS )
  {
    outputWS = WorkspaceFactory::Instance().create(inputWS);
    setProperty("OutputWorkspace",outputWS);
  }

  // Check that the input workspace doesn't already have the desired unit. If it does, just copy data.
  const std::string targetUnit = getPropertyValue("Target");
  if ( inputUnit->unitID() == targetUnit )
  {
    g_log.information() << "Input workspace already has target unit " << targetUnit
                        << ", so just copying the data unchanged." << std::endl;
    this->copyDataUnchanged(inputWS, outputWS);
  }

  // Set the final unit that our output workspace will have
  boost::shared_ptr<Unit> outputUnit = outputWS->getAxis(0)->unit() = UnitFactory::Instance().create(targetUnit);

  // Check whether the Y data of the input WS is dimensioned and set output WS flag to be same
  const bool distribution = outputWS->isDistribution(inputWS->isDistribution());
  const unsigned int size = inputWS->blocksize();
  // Calculate the number of spectra in this workspace
  const int numberOfSpectra = inputWS->size() / size;

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
    convertViaTOF(numberOfSpectra,inputWS,outputWS);
  }

  // If appropriate, put back the bin width division into Y/E.
  if (distribution)
  {
    for (int i = 0; i < numberOfSpectra; ++i) {
      // There must be good case for having a 'divideByBinWidth'/'normalise' algorithm...
      for (unsigned int j = 0; j < size; ++j)
      {
        const double width = std::abs( outputWS->dataX(i)[j+1] - outputWS->dataX(i)[j] );
        outputWS->dataY(i)[j] = inputWS->dataY(i)[j]/width;
        outputWS->dataE(i)[j] = inputWS->dataE(i)[j]/width;
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
void ConvertUnits::convertQuickly(const int& numberOfSpectra, API::Workspace_sptr outputWS, const double& factor, const double& power)
{
  // Loop over the histograms (detector spectra)
  for (int i = 0; i < numberOfSpectra; ++i) {
    std::vector<double>::iterator it;
    for (it = outputWS->dataX(i).begin(); it != outputWS->dataX(i).end(); ++it)
    {
      *it = factor * std::pow(*it,power);
    }
  }
}

/** Convert the workspace units using TOF as an intermediate step in the conversion
* @param numberOfSpectra The number of Spectra
* @param inputWS the input workspace
* @param outputWS the output workspace
*/
void ConvertUnits::convertViaTOF(const int& numberOfSpectra, API::Workspace_const_sptr inputWS, API::Workspace_sptr outputWS)
{
  // Get a pointer to the instrument contained in the workspace
  boost::shared_ptr<API::Instrument> instrument = inputWS->getInstrument();
  // And one to the SpectraDetectorMap
  boost::shared_ptr<API::SpectraDetectorMap> specMap = inputWS->getSpectraMap();

  // Get the unit object for each workspace
  boost::shared_ptr<Unit> inputUnit = inputWS->getAxis(0)->unit();
  boost::shared_ptr<Unit> outputUnit = outputWS->getAxis(0)->unit();

  // Get the distance between the source and the sample (assume in metres)
  Geometry::ObjComponent* sample = instrument->getSample();
  double l1;
  try
  {
    l1 = instrument->getSource()->getDistance(*sample);
    g_log.debug() << "Source-sample distance: " << l1 << std::endl;
  }
  catch (Exception::NotFoundError e)
  {
    g_log.error("Unable to calculate source-sample distance");
    throw Exception::InstrumentDefinitionError("Unable to calculate source-sample distance", inputWS->getTitle());
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
      const int spec = inputWS->getAxis(1)->spectraNo(i);
      // Now get the detector to which this relates
      Geometry::V3D detPos = specMap->getDetector(spec)->getPos();
      // Get the sample-detector distance for this detector (in metres)
      const double l2 = detPos.distance(samplePos);
      // The scattering angle for this detector (in radians).
      //     - this assumes the incident beam comes in along the z axis
      const double twoTheta = detPos.zenith(samplePos);
      if (failedDetectorIndex != notFailed)
      {
        g_log.information() << "Unable to calculate sample-detector[" << failedDetectorIndex << "-" << i-1 << "] distance. Zeroing spectrum." << std::endl;
        failedDetectorIndex = notFailed;
      }

      // Convert the input unit to time-of-flight
      inputUnit->toTOF(outputWS->dataX(i),emptyVec,l1,l2,twoTheta,emode,efixed,delta);
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

  } // loop over spectra

  if (failedDetectorIndex != notFailed)
  {
    g_log.information() << "Unable to calculate sample-detector[" << failedDetectorIndex << "-" << numberOfSpectra-1 << "] distance. Zeroing spectrum." << std::endl;
  }

}

/// Copies over the workspace data from the input to the output workspace
void ConvertUnits::copyDataUnchanged(const API::Workspace_const_sptr inputWS, const API::Workspace_sptr outputWS)
{
  const int numberOfSpectra = inputWS->size() / inputWS->blocksize();
  for (int i = 0; i < numberOfSpectra; ++i)
  {
    outputWS->dataX(i) = inputWS->dataX(i);
    outputWS->dataY(i) = inputWS->dataY(i);
    outputWS->dataE(i) = inputWS->dataE(i);
  }
}


} // namespace Algorithm
} // namespace Mantid
