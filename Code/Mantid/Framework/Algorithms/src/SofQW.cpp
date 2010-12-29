//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SofQW.h"
#include "MantidDataObjects/Histogram1D.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/UnitFactory.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SofQW)

using namespace Kernel;
using namespace API;

void SofQW::init()
{
  CompositeValidator<> *wsValidator = new CompositeValidator<>;
  wsValidator->add(new WorkspaceUnitValidator<>("DeltaE"));
  wsValidator->add(new CommonBinsValidator<>);
  wsValidator->add(new HistogramValidator<>);
  wsValidator->add(new InstrumentValidator<>);
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,wsValidator));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));

  declareProperty(new ArrayProperty<double>("QAxisBinning", new RebinParamsValidator));
  
  std::vector<std::string> propOptions;
  propOptions.push_back("Direct");
  propOptions.push_back("Indirect");
  declareProperty("EMode","",new ListValidator(propOptions),
    "The energy mode");
  BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
  mustBePositive->setLower(0.0);
  declareProperty("EFixed",0.0,mustBePositive,
    "Value of fixed energy in meV : EI (EMode=Direct) or EF (EMode=Indirect).");
}

void SofQW::exec()
{
  using namespace Geometry;
  
  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
  // Do the full check for common binning
  if ( ! WorkspaceHelpers::commonBoundaries(inputWorkspace) )
  {
    g_log.error("The input workspace must have common binning across all spectra");
    throw std::invalid_argument("The input workspace must have common binning across all spectra");
  }
  
  std::vector<double> verticalAxis;
  MatrixWorkspace_sptr outputWorkspace = this->setUpOutputWorkspace(inputWorkspace, verticalAxis);

  // Get hold of the unit objects in the output workspace
  const Unit_const_sptr wUnit = outputWorkspace->getAxis(0)->unit();
  const Unit_const_sptr qUnit = outputWorkspace->getAxis(1)->unit();
  
  // Retrieve the emode & efixed properties
  const std::string emodeStr = getProperty("EMode");
  // Convert back to an integer representation
  int emode = 0;
  if (emodeStr == "Direct") emode=1;
  else if (emodeStr == "Indirect") emode=2;
  
  // Get a pointer to the instrument contained in the workspace
  IInstrument_const_sptr instrument = inputWorkspace->getInstrument();
  // Get the parameter map
  const ParameterMap& pmap = inputWorkspace->constInstrumentParameters();
  // Get the distance between the source and the sample (assume in metres)
  IObjComponent_const_sptr source = instrument->getSource();
  IObjComponent_const_sptr sample = instrument->getSample();
  double l1;
  try
  {
    l1 = source->getDistance(*sample);
    g_log.debug() << "Source-sample distance: " << l1 << std::endl;
  }
  catch (Exception::NotFoundError e)
  {
    g_log.error("Unable to calculate source-sample distance");
    throw Exception::InstrumentDefinitionError("Unable to calculate source-sample distance", inputWorkspace->getTitle());
  }
  
  // Loop over input workspace bins, reassigning data to correct bin in output qw workspace
  const int numHists = inputWorkspace->getNumberHistograms();
  const int numBins = inputWorkspace->blocksize();
  for (int i = 0; i < numHists; ++i)
  {
    try {
      // Now get the detector object for this histogram
      IDetector_sptr det = inputWorkspace->getDetector(i);
      // Get the sample-detector distance for this detector (in metres)
      const double l2 = det->getDistance(*sample);
      // The scattering angle for this detector (in radians).
      const double twoTheta = inputWorkspace->detectorTwoTheta(det);
      // If an indirect instrument, try getting Efixed from the geometry
      double efixed = getProperty("EFixed");
      if (emode==2)
      {
        try {
          Parameter_sptr par = pmap.get(det->getComponent(),"Efixed");
          if (par) 
          {
            efixed = par->value<double>();
          }
        } catch (std::runtime_error) { /* Throws if a DetectorGroup, use single provided value */ }
      }
      // Get a copy of the X vector for the current histogram (although all X vectors are the same,
      // they will be changed by the calls to the Unit methods so this must be done each time)
      MantidVec currentX = inputWorkspace->readX(i);
      MantidVec emptyVec;
      // Now convert it to Q
      wUnit->toTOF(currentX,emptyVec,l1,l2,twoTheta,emode,efixed,0.0);
      qUnit->fromTOF(currentX,emptyVec,l1,l2,twoTheta,emode,efixed,0.0);
    
      // Now need to loop over unit-converted vector, looking up the Q value of each point and
      // putting it in the right place in the q-w grid
      const MantidVec& Y = inputWorkspace->readY(i);
      const MantidVec& E = inputWorkspace->readE(i);
      for (int j = 0; j < numBins; ++j)
      {
        // Calculate the Q value as the centre of the bin
        const double q = 0.5 * (currentX[j] + currentX[j+1]);
        // Test whether it's in range of the Q axis
        if ( q < verticalAxis.front() || q > verticalAxis.back() ) continue;
        // Find which q bin this point lies in
        const MantidVec::difference_type qIndex = std::upper_bound(verticalAxis.begin(),verticalAxis.end(),q) - verticalAxis.begin() - 1;
        // And add the data and it's error to that bin
        outputWorkspace->dataY(qIndex)[j] += Y[j];
        outputWorkspace->dataE(qIndex)[j] = sqrt( pow(outputWorkspace->readE(qIndex)[j],2) + pow(E[j],2) );
      }
    } catch (Exception::NotFoundError e) {
       // Get to here if exception thrown when calculating distance to detector
       // Presumably, if we get to here the spectrum will be all zeroes anyway (from conversion to E)
       continue;
    }
  }

  // If the input workspace was a distribution, need to divide by q bin width
  if (inputWorkspace->isDistribution()) this->makeDistribution(outputWorkspace,verticalAxis);
}

/** Creates the output workspace, setting the axes according to the input binning parameters
 *  @param[in]  inputWorkspace The input workspace
 *  @param[out] newAxis        The 'vertical' axis defined by the given parameters
 *  @return A pointer to the newly-created workspace
 */
API::MatrixWorkspace_sptr SofQW::setUpOutputWorkspace(API::MatrixWorkspace_const_sptr inputWorkspace, std::vector<double>& newAxis)
{
  // Create vector to hold the new X axis values
  MantidVecPtr xAxis;
  xAxis.access() = inputWorkspace->readX(0);
  const int xLength = xAxis->size();
  // Create a vector to temporarily hold the vertical ('y') axis and populate that
  const int yLength = VectorHelper::createAxisFromRebinParams(getProperty("QAxisBinning"),newAxis);
  
  // Create the output workspace
  MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create(inputWorkspace,yLength-1,xLength,xLength-1);
  // Create a numeric axis to replace the default vertical one
  Axis* const verticalAxis = new NumericAxis(yLength);
  outputWorkspace->replaceAxis(1,verticalAxis);
  
  // Now set the axis values
  for (int i=0; i < yLength-1; ++i)
  {
    outputWorkspace->setX(i,xAxis);
    verticalAxis->setValue(i,newAxis[i]);
  }
  // One more to set on the 'y' axis
  verticalAxis->setValue(yLength-1,newAxis[yLength-1]);
  
  // Set the axis units
  verticalAxis->unit() = UnitFactory::Instance().create("MomentumTransfer");
  
  setProperty("OutputWorkspace",outputWorkspace);
  return outputWorkspace;
}

/** Divide each bin by the width of its q bin.
 *  @param outputWS The output workspace
 *  @param qAxis    A vector of the q bin boundaries
 */
void SofQW::makeDistribution(API::MatrixWorkspace_sptr outputWS, const std::vector<double> qAxis)
{
  std::vector<double> widths(qAxis.size());
  std::adjacent_difference(qAxis.begin(),qAxis.end(),widths.begin());

  const int numQBins = outputWS->getNumberHistograms();
  for (int i=0; i < numQBins; ++i)
  {
    MantidVec& Y = outputWS->dataY(i);
    MantidVec& E = outputWS->dataE(i);
    std::transform(Y.begin(),Y.end(),Y.begin(),std::bind2nd(std::divides<double>(),widths[i+1]));
    std::transform(E.begin(),E.end(),E.begin(),std::bind2nd(std::divides<double>(),widths[i+1]));
  }
}

} // namespace Algorithms
} // namespace Mantid
