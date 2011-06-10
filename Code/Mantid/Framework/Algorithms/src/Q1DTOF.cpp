//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Q1DTOF.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataObjects/Histogram1D.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
//this test algorithm isn't to be declared! DECLARE_ALGORITHM(Q1DTOF)

/// Sets documentation strings for this algorithm
void Q1DTOF::initDocs()
{
  this->setWikiSummary("Part of the 1D data reduction chain for SANS instruments. ");
  this->setOptionalMessage("Part of the 1D data reduction chain for SANS instruments.");
}


using namespace Kernel;
using namespace API;
using namespace Geometry;

void Q1DTOF::init()
{
  CompositeValidator<> *dataVal = new CompositeValidator<>;
  dataVal->add(new WorkspaceUnitValidator<>("Wavelength"));
  dataVal->add(new HistogramValidator<>);
  dataVal->add(new InstrumentValidator<>);
  dataVal->add(new CommonBinsValidator<>);
  declareProperty(new WorkspaceProperty<>("DetBankWorkspace", "", Direction::Input, dataVal));
  CompositeValidator<> *wavVal = new CompositeValidator<>;
  wavVal->add(new WorkspaceUnitValidator<>("Wavelength"));
  wavVal->add(new HistogramValidator<>);
  declareProperty(new WorkspaceProperty<>("WavelengthAdj", "", Direction::Input, wavVal));
  declareProperty(new WorkspaceProperty<>("PixelAdj","", Direction::Input, true));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output));
  declareProperty(new ArrayProperty<double>("OutputBinning", new RebinParamsValidator));
  declareProperty("AccountForGravity",false);
}
/**
  @ throw invalid_argument if the workspaces are not mututially compatible
*/
void Q1DTOF::exec()
{
  m_dataWS = getProperty("DetBankWorkspace");
  MatrixWorkspace_const_sptr waveAdj = getProperty("WavelengthAdj");
  // this pointer could be NULL as PixelAdj is an optional property
  MatrixWorkspace_const_sptr pixelAdj = getProperty("PixelAdj");
  const bool doGravity = getProperty("AccountForGravity");

  //throws if we don't have common binning or another incompatibility
  examineInput(waveAdj, pixelAdj);
  // normalization as a function of wavelength (i.e. center of bin x-value)
  const MantidVec & binNorms = waveAdj->readY(0);
  // error on the wavelength normalization
  const MantidVec & binNormEs = waveAdj->readE(0);

  //define the (large number of) data objects that are going to be used in all iterations of the loop below
    // Construct a new spectra map. This will be faster than remapping the old one
  API::SpectraDetectorMap *specMap = new SpectraDetectorMap;
  // this will become the output workspace from this algorithm
  MatrixWorkspace_sptr outputWS = setUpOutputWorkspace(getProperty("OutputBinning"), specMap);
  const MantidVec & QOut = outputWS->readX(0);
  MantidVec & YOut = outputWS->dataY(0);
  MantidVec & EOutTo2 = outputWS->dataE(0);
  // normalisation that is applied to counts in each Q bin
  MantidVec normSum(YOut.size(), 0.0);
  // the error on the normalisation
  MantidVec normError(YOut.size(), 0.0);

  const Geometry::ISpectraDetectorMap & inSpecMap = m_dataWS->spectraMap();
  const Axis* const spectraAxis = m_dataWS->getAxis(1);



  const size_t numSpec = m_dataWS->getNumberHistograms();
  Progress progress(this, 0.1, 1.0, numSpec+1);

  PARALLEL_FOR3(m_dataWS, outputWS, pixelAdj)
  for (int i = 0; i < numSpec; ++i)
  {
    PARALLEL_START_INTERUPT_REGION
    // Get the pixel relating to this spectrum
    IDetector_const_sptr det;
    try {
      det = m_dataWS->getDetector(i);
    } catch (Exception::NotFoundError&) {
      g_log.warning() << "Spectrum index " << i << " has no detector assigned to it - discarding" << std::endl;
      // Catch if no detector. Next line tests whether this happened - test placed
      // outside here because Mac Intel compiler doesn't like 'continue' in a catch
      // in an openmp block.
    }
    // If no detector found or if detector is masked shouldn't be included skip onto the next spectrum
    if ( !det || det->isMonitor() || det->isMasked() )
    {
      continue;
    }

    // A temporary vector to store the Q values for the input workspace before the rebin
    MantidVec QIn(m_numInBins);
    convertWavetoQ(i, doGravity, QIn);
    const MantidVec & YIn = m_dataWS->readY(i);
    const MantidVec & EIn = m_dataWS->readE(i);

    // the weighting for this input spectrum that is added to the normalization
    MantidVec norm(m_numInBins+1);
    // the error on these weights, it contributes to the error calculation on the output workspace
    MantidVec normEs(m_numInBins);
    getNormFromSpec(pixelAdj, i, binNorms, binNormEs, norm, normEs);
    
    //find the output bin that each input y-value will fall into, remembering there is one more bin boundary than bins
    MantidVec::const_iterator loc = QOut.end();
    for(size_t j=0; j < m_numInBins; ++j)
    {
      //Q goes from a high value to a low one in the QIn array (high Q particles arrive at low TOF) so we know loc will go downwards
      loc = std::upper_bound(QOut.begin(), loc, QIn[j]);//QOut.begin(), loc, QIn[j]);
      // ignore counts that are out of the output range
      if ( (loc != QOut.begin()) && (loc != QOut.end()) )
      {
        const size_t bin = loc - QOut.begin() - 1;
        PARALLEL_CRITICAL(q1d_counts_sum)
        {
          YOut[bin] += YIn[j];
          normSum[bin] += norm[j];
          //this is the error square
          EOutTo2[bin] += EIn[j]*EIn[j];
          normError[bin] += normEs[j]*normEs[j];
        }
        //this is used to restrict the search range above for a modest increase in speed
        ++loc;
      }
    }
    
    PARALLEL_CRITICAL(q1d_spectra_map)
    {
      updateSpecMap(i, specMap, inSpecMap, outputWS);
    }

    progress.report("Computing I(Q)");
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  for (int k = 0; k < YOut.size(); ++k)
  {
    // the normalisation is a = b/c where b = counts c =normalistion term
    const double c = normSum[k];
    const double a = YOut[k] /= c;
    // when a = b/c, the formula for Da, the error on a, in terms of Db, etc. is (Da/a)^2 = (Db/b)^2 + (Dc/c)^2
    //(Da)^2 = ((Db/b)^2 + (Dc/c)^2)*(b^2/c^2) = ((Db/c)^2 + (b*Dc/c^2)^2) = (Db^2 + (b*Dc/c)^2)/c^2 = Db^2 + (Dc*a/c)^2
    //this will work as long as c>0, but then the above formula above can't deal with 0 either
    const double aOverc = a/c;
    EOutTo2[k] =+ normError[k]*aOverc*aOverc;

    progress.report("Computing I(Q)");
  }
  // finish calculating the error that was added in quadrature
  std::transform(EOutTo2.begin(), EOutTo2.end(), EOutTo2.begin(), 
    std::pointer_to_unary_function<double,double>(std::sqrt));

  // the division above means the output workspace is now a distribution regardless of the input workspace
  outputWS->isDistribution(true);

  setProperty("OutputWorkspace",outputWS);
}
/** If the distribution/raw counts status and binning on all the input workspaces
*  is the same and this reads some workspace description but throws if not
  @param binAdj workpace that will be checked to see if it has one spectrum and the same number of bins as dataWS
  @param detectAdj passing NULL for this wont raise an error, if set it will be checked this workspace has as many histograms as dataWS each with one bin
  @throw invalid_argument if the workspaces are not mututially compatible
*/
void Q1DTOF::examineInput(API::MatrixWorkspace_const_sptr binAdj, API::MatrixWorkspace_const_sptr detectAdj)
{
  if ( m_dataWS->getNumberHistograms() < 1 )
  {
    throw std::invalid_argument("Empty data workspace passed, can not continue");
  }
  m_numInBins = m_dataWS->readY(0).size();
  m_distr = m_dataWS->isDistribution();

  if ( binAdj->isDistribution() != m_distr )
  {
    throw std::invalid_argument("The distribution/raw counts status of the WavelengthAdj workspace does match the data");
  }
  if ( binAdj->getNumberHistograms() != 1 )
  {
    throw std::invalid_argument("The WavelengthAdj workspace must have one spectrum");
  }
  if ( binAdj->readY(0).size() != m_numInBins )
  {
    throw std::invalid_argument("The WavelengthAdj workspace's bins must match those of the detector bank workspace");
  }
  MantidVec::const_iterator reqX = m_dataWS->readX(0).begin();
  MantidVec::const_iterator testX = binAdj->readX(0).begin();
  for ( ; reqX != m_dataWS->readX(0).end(); ++reqX, ++testX)
  {
    if ( *reqX != *testX )
    {
      throw std::invalid_argument("The WavelengthAdj workspace must have matching bins with the detector bank workspace");
    }
  }
  
  //it is not an error for this workspace not to exist
  if (detectAdj)
  {
    if ( detectAdj->blocksize() != 1 )
    {
      throw std::invalid_argument("The PixelAdj workspace must point to a workspace with single bin spectra, as only the first bin is used");
    }
    if ( detectAdj->getNumberHistograms() != m_dataWS->getNumberHistograms() )
    {
      throw std::invalid_argument("The PixelAdj workspace must have one spectrum for each spectrum in the detector bank workspace");
    }
    g_log.debug() << "Optional PixelAdj workspace " << detectAdj->getName() << " validated successfully\n";
  }

  g_log.debug() << "All input workspaces were found to be valid\n";
}
/** Creates the output workspace, its size, units, etc.
*  @param binParams the bin boundary specification using the same same syntax as param the Rebin algorithm
*  @param specMap a spectra map that the new workspace should use and take owner ship of
*  @return A pointer to the newly-created workspace
*/
API::MatrixWorkspace_sptr Q1DTOF::setUpOutputWorkspace(const std::vector<double> & binParams,  const API::SpectraDetectorMap * const specMap) const
{
  // Calculate the output binning
  MantidVecPtr XOut;
  size_t sizeOut = static_cast<size_t>(
    VectorHelper::createAxisFromRebinParams(binParams, XOut.access()));

  // Now create the output workspace
  MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(m_dataWS,1,sizeOut,sizeOut-1);
  outputWS->getAxis(0)->unit() = UnitFactory::Instance().create("MomentumTransfer");
  outputWS->setYUnitLabel("1/cm");

  // Set the X vector for the output workspace
  outputWS->setX(0, XOut);

  outputWS->replaceSpectraMap(specMap);
  return outputWS;
}
/** Fills a vector with the Q values calculated from the wavelengths in the input workspace and the workspace
*  geometry as Q = 4*pi*sin(theta)/lambda
*  @param[in] specIndex the spectrum to calculate
*  @param[in] doGravity if to include gravity in the calculation of Q
*  @param[out] Qs a preallocated array that is large enough to contain all the calculated Q values
*  @throw NotFoundError if the detector associated with the spectrum is not found in the instrument definition
*/
void Q1DTOF::convertWavetoQ(const size_t specIndex, const bool doGravity, MantidVec & Qs) const
{
  static const double FOUR_PI=4.0*M_PI;
  const MantidVec & XIn = m_dataWS->readX(specIndex);
  IDetector_const_sptr det = m_dataWS->getDetector(specIndex);

  if (doGravity)
  {
    GravitySANSHelper grav(m_dataWS, det);
    for ( size_t j = 0; j < m_numInBins; ++j)
    {
      // as the fall under gravity is wavelength dependent sin theta is now different for each bin with each detector 
      // the HistogramValidator at the start should ensure that we have one more bin on the readX()s
      const double lambda = (XIn[j]+XIn[j+1])/2.0;
      const double sinTheta = grav.calcSinTheta(lambda);
      // Now we're ready to go to Q
      Qs[j] = FOUR_PI*sinTheta/lambda;
    }
  }
  else
  {
    // Calculate the Q values for the current spectrum, using Q = 4*pi*sin(theta)/lambda
    const double factor = 2.0* FOUR_PI*sin( m_dataWS->detectorTwoTheta(det)/2.0 );
    for ( size_t j = 0; j < m_numInBins; ++j)
    {
      // the HistogramValidator at the start should ensure that we have one more bin on the readX()s
      Qs[j] = factor/(XIn[j]+XIn[j+1]);
    }
  }
}
/** Calculates the contribution to the normalization terms from each bin in a spectrum
*  @param[in] pixelAdj detector efficiency input workspace
*  @param[in] specIndex index number of the spectrum to calculate for
*  @param[in] binNorms the wavelength dependent normalization term
*  @param[in] binNormEs the wavelength dependent term's error
*  @param[out] outNorms a preallocated array big enough to contain the normalization terms
*  @param[out] outEs a preallocated array to store the errors
*/
void Q1DTOF::getNormFromSpec(API::MatrixWorkspace_const_sptr pixelAdj, const size_t specIndex, const MantidVec & binNorms, const MantidVec & binNormEs, MantidVec & outNorms, MantidVec & outEs) const
{
  // we normalize by the bin width in Q
  getUnmaskedWidths(specIndex, outNorms);

  // and we normalize by solidangle and pixel correction
  const double detEff = pixelWeight(pixelAdj, specIndex);
  std::transform(outNorms.begin(), outNorms.end(), outNorms.begin(),
    std::bind2nd(std::multiplies<double>(), detEff));

  // and lastly normalize by the wavelength dependent correction, keeping the percentage errors the same
  std::transform(binNormEs.begin(), binNormEs.end(), outNorms.begin(),
    outEs.begin(), std::multiplies<double>());
  std::transform(binNorms.begin(), binNorms.end(), outNorms.begin(),
    outNorms.begin(), std::multiplies<double>());
}
/** Calculates the normalisation for the spectrum specified by the index number that was passed
*  as the solid anlge multiplied by the pixelAdj that was passed
*  @param[in] pixelAdj if not NULL this is workspace contains single bins with the adjustments, e.g. detector efficencie, for hte given spectrum index
*  @param[in] specIndex the spectrum index to return the data from
*  @return the calculated normalisation
*/
double Q1DTOF::pixelWeight(API::MatrixWorkspace_const_sptr pixelAdj,  const size_t specIndex) const
{
  const V3D samplePos = m_dataWS->getInstrument()->getSample()->getPos();

  double weight = m_dataWS->getDetector(specIndex)->solidAngle(samplePos);
  return pixelAdj ? pixelAdj->readY(specIndex)[0]*weight : weight;
}
/** Calculates the bin widths scaled to bin masking
*  @param[in] specIndex the spectrum to calculate
*  @param[out] widths a preallocated array that is large enough to contain a scaled bin width for each bin
*/
void Q1DTOF::getUnmaskedWidths(const size_t specIndex, MantidVec & widths) const
{
  // At this point we check for masked bins in the input workspace and modify accordingly
  if ( ! m_dataWS->hasMaskedBins(specIndex) )
  {
    std::adjacent_difference(m_dataWS->readX(0).begin(), m_dataWS->readX(0).end(), widths.begin());
    widths.erase(widths.begin());
  }
  else // take masking into account
  {// I hope to change this in my Qxy changes, generalise and simplify and add it to workspace or vector helper
    MantidVec included_bins,solidAngleVec;
    const MantidVec & Qx = m_dataWS->readX(specIndex);
    included_bins.push_back(Qx.front());
    // Get a reference to the list of masked bins
    const MatrixWorkspace::MaskList & mask = m_dataWS->maskedBins(specIndex);
    // Now iterate over the list, adjusting the weights for the affected bins
    // Since we've reversed Qx, we need to reverse the list too
    MatrixWorkspace::MaskList::const_reverse_iterator it;
    for (it = mask.rbegin(); it != mask.rend(); ++it)
    {
      const double currentX = Qx[m_numInBins-2-(*it).first];
      // Add an intermediate bin with full weight if masked bins aren't consecutive
      if (included_bins.back() != currentX) 
      {
        solidAngleVec.push_back(1);
        included_bins.push_back(currentX);
      }
      // The weight for this masked bin is 1 - the degree to which this bin is masked
      solidAngleVec.push_back( 1 * (1.0-(*it).second) );
      included_bins.push_back( Qx[m_numInBins-1-(*it).first]);
    }
    // Add on a final bin with full weight if masking doesn't go up to the end
    if (included_bins.back() != Qx.back()) 
    {
      solidAngleVec.push_back(1);
      included_bins.push_back(Qx.back());
    }
      
    // Create a zero vector for the errors because we don't care about them here
    const MantidVec zeroes(solidAngleVec.size(),0.0);
    MantidVec tmp(widths.size());
    // Rebin the solid angles - note that this is a distribution
    VectorHelper::rebin(included_bins,solidAngleVec,zeroes,Qx, widths, tmp,true,true);
  }
}
/** Map all the detectors onto the spectrum of the output
*  @param specIndex the spectrum to add
*  @param specMap the map in the output workspace to write to
*  @param inSpecMap ?????????????
*  @param outputWS the workspace with the spectra axis
*/
void Q1DTOF::updateSpecMap(const size_t specIndex, API::SpectraDetectorMap * const specMap, const Geometry::ISpectraDetectorMap & inSpecMap, API::MatrixWorkspace_sptr outputWS) const
{
  Axis* const spectraAxis = m_dataWS->getAxis(1);
  if (spectraAxis->isSpectra())
  {
    specid_t newSpectrumNo = outputWS->getAxis(1)->spectraNo(0) = spectraAxis->spectraNo(specIndex);
    specMap->addSpectrumEntries(newSpectrumNo,inSpecMap.getDetectors(spectraAxis->spectraNo(specIndex)));
  }
}

} // namespace Algorithms
} // namespace Mantid

