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
#include "MantidGeometry/Instrument/DetectorGroup.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SofQW)

/// Sets documentation strings for this algorithm
void SofQW::initDocs()
{
  this->setWikiSummary(" Converts a 2D workspace that has axes of <math>\\Delta E</math> against spectrum number to one that gives intensity as a function of momentum transfer against energy: <math>\\rm{S}\\left( q, \\omega \\right)</math>. ");
  this->setOptionalMessage("Converts a 2D workspace that has axes of <math>\\Delta E</math> against spectrum number to one that gives intensity as a function of momentum transfer against energy: <math>\\rm{S}\\left( q, \\omega \\right)</math>.");
}


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
  V3D beamDir = sample->getPos() - source->getPos();
  beamDir.normalize();

  double l1;
  try
  {
    l1 = source->getDistance(*sample);
    g_log.debug() << "Source-sample distance: " << l1 << std::endl;
  }
  catch (Exception::NotFoundError &)
  {
    g_log.error("Unable to calculate source-sample distance");
    throw Exception::InstrumentDefinitionError("Unable to calculate source-sample distance", inputWorkspace->getTitle());
  }

  // Conversion constant for E->k. k(A^-1) = sqrt(energyToK*E(meV))
  const double energyToK = 8.0*M_PI*M_PI*PhysicalConstants::NeutronMass*PhysicalConstants::meV*1e-20 / 
    (PhysicalConstants::h*PhysicalConstants::h);

  // Loop over input workspace bins, reassigning data to correct bin in output qw workspace
  const size_t numHists = inputWorkspace->getNumberHistograms();
  const size_t numBins = inputWorkspace->blocksize();
  Progress prog(this,0.0,1.0,numHists);
  for (int64_t i = 0; i < int64_t(numHists); ++i)
  {
    try {
      // Now get the detector object for this histogram
      IDetector_sptr spectrumDet = inputWorkspace->getDetector(i);
      if( spectrumDet->isMonitor() ) continue;
      // If an indirect instrument, try getting Efixed from the geometry
      double efixed = getProperty("EFixed");
      if (emode==2)
      {
        try {
          Parameter_sptr par = pmap.get(spectrumDet->getComponent(),"Efixed");
          if (par) 
          {
            efixed = par->value<double>();
          }
        } catch (std::runtime_error&) { /* Throws if a DetectorGroup, use single provided value */ }
      }

      // For inelastic scattering the simple relationship q=4*pi*sinTheta/lambda does not hold. In order to
      // be completely general wemust calculate the momentum transfer by calculating the incident and final
      // wave vectors and then use |q| = sqrt[(ki - kf)*(ki - kf)]
      DetectorGroup_sptr detGroup = boost::dynamic_pointer_cast<DetectorGroup>(spectrumDet);
      std::vector<IDetector_sptr> detectors;
      if( detGroup ) 
      {
	      detectors = detGroup->getDetectors();
      }
      else
      {
	      detectors.push_back(spectrumDet);
      }
      const size_t numDets = detectors.size();
      const MantidVec& Y = inputWorkspace->readY(i);
      const MantidVec& E = inputWorkspace->readE(i);
      const MantidVec& X = inputWorkspace->readX(i);

      // Loop over the detectors and for each bin calculate Q
      for( size_t idet = 0; idet < numDets; ++idet )
      {
	      IDetector_sptr det = detectors[idet];
	      // Calculate kf vector direction and then Q for each energy bin
	      V3D scatterDir = (det->getPos() - sample->getPos());
 	      scatterDir.normalize();
	      for (size_t j = 0; j < numBins; ++j)
	      {
 	        const double deltaE = 0.5*(X[j] + X[j+1]);
	        // Compute ki and kf wave vectors and therefore q = ki - kf
	        double ei(0.0),ef(0.0);
	        if( emode == 1 )
	        {
	          ei = efixed;
	          ef = efixed - deltaE;
	        }
	        else
	        {
	          ei = efixed + deltaE;
	          ef = efixed;
	        }
	        const V3D ki = beamDir*sqrt(energyToK*ei);
	        const V3D kf = scatterDir*(sqrt(energyToK*(ef)));
	        const double q = (ki - kf).norm();

	        // Test whether it's in range of the Q axis
	        if ( q < verticalAxis.front() || q > verticalAxis.back() ) continue;
	        // Find which q bin this point lies in
	        const MantidVec::difference_type qIndex =
	        std::upper_bound(verticalAxis.begin(),verticalAxis.end(),q) - verticalAxis.begin() - 1;
	  
	        // And add the data and it's error to that bin, taking into account the number of detectors contributing to this bin
	        outputWorkspace->dataY(qIndex)[j] += Y[j]/numDets;
	        // Standard error on the average
	        outputWorkspace->dataE(qIndex)[j] = sqrt( (pow(outputWorkspace->readE(qIndex)[j],2) + pow(E[j],2))/numDets );
	      }
      }

    } catch (Exception::NotFoundError &) {
      // Get to here if exception thrown when calculating distance to detector
      // Presumably, if we get to here the spectrum will be all zeroes anyway (from conversion to E)
      continue;
    }
    prog.report();
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
 *  @param outputWS :: The output workspace
 *  @param qAxis ::    A vector of the q bin boundaries
 */
void SofQW::makeDistribution(API::MatrixWorkspace_sptr outputWS, const std::vector<double> qAxis)
{
  std::vector<double> widths(qAxis.size());
  std::adjacent_difference(qAxis.begin(),qAxis.end(),widths.begin());

  const size_t numQBins = outputWS->getNumberHistograms();
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
