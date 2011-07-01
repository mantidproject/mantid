//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Q1D2.h"
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
DECLARE_ALGORITHM(Q1D2)

/// Sets documentation strings for this algorithm
void Q1D2::initDocs()
{
  this->setWikiSummary("Part of the 1D data reduction chain for SANS instruments. ");
  this->setOptionalMessage("Part of the 1D data reduction chain for SANS instruments.");
}


using namespace Kernel;
using namespace API;
using namespace Geometry;

void Q1D2::init()
{
  CompositeValidator<> *dataVal = new CompositeValidator<>;
  dataVal->add(new WorkspaceUnitValidator<>("Wavelength"));
  dataVal->add(new HistogramValidator<>);
  dataVal->add(new InstrumentValidator<>);
  dataVal->add(new CommonBinsValidator<>);
  declareProperty(new WorkspaceProperty<>("DetBankWorkspace", "", Direction::Input, dataVal));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output));
  declareProperty(new ArrayProperty<double>("OutputBinning", new RebinParamsValidator));
  
  declareProperty(new WorkspaceProperty<>("PixelAdj","", Direction::Input, true));
  CompositeValidator<> *wavVal = new CompositeValidator<>;
  wavVal->add(new WorkspaceUnitValidator<>("Wavelength"));
  wavVal->add(new HistogramValidator<>);
  declareProperty(new WorkspaceProperty<>("WavelengthAdj", "", Direction::Input, true, wavVal));
  
  declareProperty("AccountForGravity",false);
  
  BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
  mustBePositive->setLower(0.0);
  declareProperty("RadiusCut", 0.0, mustBePositive);
  declareProperty("WaveCut", 0.0, mustBePositive->clone());
}
/**
  @ throw invalid_argument if the workspaces are not mututially compatible
*/
void Q1D2::exec()
{
  m_dataWS = getProperty("DetBankWorkspace");
  MatrixWorkspace_const_sptr waveAdj = getProperty("WavelengthAdj");
  // this pointer could be NULL as PixelAdj is an optional property
  MatrixWorkspace_const_sptr pixelAdj = getProperty("PixelAdj");
  const bool doGravity = getProperty("AccountForGravity");
  initizeCutOffs(getProperty("RadiusCut"), getProperty("WaveCut"));

  //throws if we don't have common binning or another incompatibility
  examineInput(waveAdj, pixelAdj);
  // normalization as a function of wavelength (i.e. centers of x-value bins)
  double const * const binNorms = waveAdj ? &(waveAdj->readY(0)[0]) : NULL;
  // error on the wavelength normalization
  double const * const binNormEs = waveAdj ? &(waveAdj->readE(0)[0]) : NULL;

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
  MantidVec normError2(YOut.size(), 0.0);

  const Geometry::ISpectraDetectorMap & inSpecMap = m_dataWS->spectraMap();

  const int numSpec = static_cast<int>(m_dataWS->getNumberHistograms());
  Progress progress(this, 0.05, 1.0, numSpec+1);

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

    //get the bins that are included inside the RadiusCut/WaveCutcut off, those to calculate for
    const size_t wavStart = waveLengthCutOff(i);
    if (wavStart >=  m_dataWS->readY(i).size())
    {
      // all the spectra in this detector are out of range
      continue;
    }
    
    const size_t numWavbins = m_dataWS->readY(i).size()-wavStart;
    //make just one call to new to reduce CPU overhead on each thread, access is these three arrays is via iterators
    MantidVec _noDirectUseStorage_(3*numWavbins);
    //normalization term
    MantidVec::iterator norms = _noDirectUseStorage_.begin();
    // the error on these weights, it contributes to the error calculation on the output workspace
    MantidVec::iterator normETo2s = norms + numWavbins;
    // the Q values calculated from input wavelength workspace
    MantidVec::iterator QIn = normETo2s + numWavbins;

    // the weighting for this input spectrum that is added to the normalization
    calculateNormalization(wavStart, i, pixelAdj, binNorms, binNormEs, norms, normETo2s);

    // now read the data from the input workspace, calculate Q for each bin
    convertWavetoQ(i, doGravity, wavStart, QIn);
    // Pointers to the counts data and it's error
    MantidVec::const_iterator YIn = m_dataWS->readY(i).begin()+wavStart;
    MantidVec::const_iterator EIn = m_dataWS->readE(i).begin()+wavStart;

    //when finding the output Q bin remember that the input Q bins (from the convert to wavelength) start high and reduce
    MantidVec::const_iterator loc = QOut.end();
    // sum the Q contributions from each individual spectrum into the output array
    const MantidVec::const_iterator end = m_dataWS->readY(i).end();
    for( ; YIn != end; ++YIn, ++EIn, ++QIn, ++norms, ++normETo2s)
    {
      //find the output bin that each input y-value will fall into, remembering there is one more bin boundary than bins
      getQBinPlus1(QOut, *QIn, loc);
      // ignore counts that are out of the output range
      if ( (loc != QOut.begin()) && (loc != QOut.end()) )
      {
        const size_t bin = loc - QOut.begin() - 1;
        PARALLEL_CRITICAL(q1d_counts_sum)
        {
          YOut[bin] += *YIn;
          normSum[bin] += *norms;
          //these are the errors squared which will be summed and square rooted at the end
          EOutTo2[bin] += (*EIn)*(*EIn);
          normError2[bin] += *normETo2s;
        }
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

  progress.report("Normalizing I(Q)");
  //finally divide the number of counts in each output Q bin by its weighting
  normalize(normSum, normError2, YOut, EOutTo2);

  setProperty("OutputWorkspace",outputWS);
}
/** If the distribution/raw counts status and binning on all the input workspaces
*  is the same and this reads some workspace description but throws if not
  @param binWS workpace that will be checked to see if it has one spectrum and the same number of bins as dataWS
  @param detectWS passing NULL for this wont raise an error, if set it will be checked this workspace has as many histograms as dataWS each with one bin
  @throw invalid_argument if the workspaces are not mututially compatible
*/
void Q1D2::examineInput(API::MatrixWorkspace_const_sptr binAdj, API::MatrixWorkspace_const_sptr detectAdj)
{
  if ( m_dataWS->getNumberHistograms() < 1 )
  {
    throw std::invalid_argument("Empty data workspace passed, can not continue");
  }

  //it is not an error for these workspaces not to exist
  if (binAdj)
  {
    if ( binAdj->getNumberHistograms() != 1 )
    {
      throw std::invalid_argument("The WavelengthAdj workspace must have one spectrum");
    }
    if ( binAdj->readY(0).size() != m_dataWS->readY(0).size() )
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
/* the distribution status of workspaces isn't getting propogated reliably enough for this work yet i.e. a distribution workspace x a raw counts = raw counts
if ( binAdj->isDistribution() != m_dataWS->isDistribution() )
    {
      throw std::invalid_argument("The distrbution/raw counts status of the wavelengthAdj and DetBankWorkspace must be the same");
    }*/
  }
  else if( ! m_dataWS->isDistribution() )
  {
    throw std::invalid_argument("The data workspace must be a distrbution if there is no Wavelength dependent adjustment");
  }
  
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
/** Detector independent parts of the wavelength cut off calculation
*  @param RCut the radius cut off, should be value of the property RadiusCut
*  @param WCut this wavelength cut off, should be equal to the value WaveCut
*/
void Q1D2::initizeCutOffs(const double RCut, const double WCut)
{
  if ( RCut > 0 && WCut > 0 )
  {
    m_WCutOver = WCut/RCut;
    m_RCut = RCut;
  }
}
/** Creates the output workspace, its size, units, etc.
*  @param binParams the bin boundary specification using the same same syntax as param the Rebin algorithm
*  @param specMap a spectra map that the new workspace should use and take owner ship of
*  @return A pointer to the newly-created workspace
*/
API::MatrixWorkspace_sptr Q1D2::setUpOutputWorkspace(const std::vector<double> & binParams,  const API::SpectraDetectorMap * const specMap) const
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
  outputWS->isDistribution(true);

  outputWS->replaceSpectraMap(specMap);
  return outputWS;
}
/** Finds the first index number of the first wavelength bin that should included based on the
*  the calculation: W = Wcut (Rcut-R)/Rcut
*  @param specInd spectrum that is being analysed
*  @return index number of the first bin to include in the calculation
*/
size_t Q1D2::waveLengthCutOff(const size_t specInd) const
{
  if ( !(m_RCut > 0) )
  {
    return 0;
  }
  //get the distance of between this detector and the origin, which should be the along the beam center
  const V3D posOnBank = m_dataWS->getDetector(specInd)->getPos();
  double R = (posOnBank.X()*posOnBank.X())+(posOnBank.Y()*posOnBank.Y());
  R = std::sqrt(R);

  const double WMin = m_WCutOver*(m_RCut-R);
  const MantidVec & Xs = m_dataWS->readX(specInd);
  return std::lower_bound(Xs.begin(), Xs.end(), WMin) - Xs.begin();
}
/** Calcualtes the normalization term for each output bin
*  @param[in] offSet the inex number of the first bin in the input wavelengths that is actually being used
*  @param[in] specInd the spectrum to calculate
*  @param[in] pixelAdj if not NULL this is workspace contains single bins with the adjustments, e.g. detector efficencies, for the given spectrum index
*  @param[in] binNorms pointer to a contigious array of doubles that are the wavelength correction from waveAdj workspace, can be NULL
*  @param[in] binNormEs pointer to a contigious array of doubles which corrospond to the corrections and are their errors, can be NULL
*  @param[out] norm normalization for each bin, including soild angle, pixel correction, the proportion that is not masked and the normalization workspace
*  @param[out] normETo2 this pointer must point to the end of the norm array, it will be filled with the total of the error on the normalization
*/
void Q1D2::calculateNormalization(const size_t wavStart, const size_t specInd, API::MatrixWorkspace_const_sptr pixelAdj, double const * const binNorms, double const * const binNormEs, const MantidVec::iterator norm, const MantidVec::iterator normETo2) const
{
  double detectorAdj, detAdjErr;
  pixelWeight(pixelAdj, specInd, detectorAdj, detAdjErr);

  //use that the normalization array ends at the start of the error array
  for(MantidVec::iterator n = norm, e = normETo2; n != normETo2; ++n, ++e)
  {
    *n = detectorAdj;
    *e = detAdjErr*detAdjErr;
  }

  if (binNorms && binNormEs)
  {
    addWaveAdj(binNorms+wavStart, binNormEs+wavStart, norm, normETo2);
  }
  normToBinWidth(wavStart, specInd, norm, normETo2);
}
/** Calculates the normalisation for the spectrum specified by the index number that was passed
*  as the solid anlge multiplied by the pixelAdj that was passed
*  @param[in] pixelAdj if not NULL this is workspace contains single bins with the adjustments, e.g. detector efficencies, for the given spectrum index
*  @param[in] specIndex the spectrum index to return the data from
*  @param[out] weight the solid angle or if pixelAdj the solid anlge times the pixel adjustment for this spectrum
*  @param[out] error the error on the weight, only non-zero if pixelAdj
*  @throw LogicError if the solid angle is tiny or negative
*/
void Q1D2::pixelWeight(API::MatrixWorkspace_const_sptr pixelAdj,  const size_t specIndex, double & weight, double & error) const
{
  const V3D samplePos = m_dataWS->getInstrument()->getSample()->getPos();

  weight = m_dataWS->getDetector(specIndex)->solidAngle(samplePos);
  if ( weight < 1e-200 )
  {
    throw std::logic_error("Invalid (zero or negative) solid angle for one detector");
  }
  // this input multiplies up the adjustment if it exists
  if (pixelAdj)
  {
    weight *= pixelAdj->readY(specIndex)[0];
    error = weight*pixelAdj->readE(specIndex)[0];
  }
  else
  {
    error = 0.0;
  }
}
/** Calculates the contribution to the normalization terms from each bin in a spectrum
*  @param[in] c pointer to the start of a contigious array of wavelength dependent normalization terms
*  @param[in] Dc pointer to the start of a contigious array that corrosponds to wavelength dependent term, having its error
*  @param[in,out] bInOut normalization for each bin, this method multiplise this by the proportion that is not masked and the normalization workspace
*  @param[in, out] e2InOut this array must follow straight after the normalization array and will contain the error on the normalisation term before the WavelengthAdj term
*/
void Q1D2::addWaveAdj(const double * c, const double * Dc, MantidVec::iterator bInOut, MantidVec::iterator e2InOut) const
{
  // normalize by the wavelength dependent correction, keeping the percentage errors the same
  // the error when a = b*c, the formula for Da, the error on a, in terms of Db, etc. is (Da/a)^2 = (Db/b)^2 + (Dc/c)^2
  //(Da)^2 = ((Db*a/b)^2 + (Dc*a/c)^2) = (Db*c)^2 + (Dc*b)^2
  // the variable names relate to those above as: existing values (b=bInOut) multiplied by the additional errors (Dc=binNormEs), existing errors (Db=sqrt(e2InOut)) times new factor (c=binNorms)

  //use the fact that errors array follows straight after the normalization array
  const MantidVec::const_iterator end = e2InOut;
  for( ; bInOut != end; ++e2InOut, ++c, ++Dc, ++bInOut)
  {
    //first the error
    *e2InOut += ( (*e2InOut)*(*c)*(*c) )+( (*Dc)*(*Dc)*(*bInOut)*(*bInOut) );
    // now the actual calculation a = b*c
    *bInOut = (*bInOut)*(*c);
  }
}
/** Add the bin widths, scaled to bin masking, to the normalization
*  @param[in] offSet the inex number of the first bin in the input wavelengths that is actually being used
*  @param[in] specIndex the spectrum to calculate
*  @param[in,out] theNorms normalization for each bin, this is multiplied by the proportion that is not masked and the normalization workspace
*  @param[in,out] errorSquared the running total of the square of the uncertainty in the normalization
*/
void Q1D2::normToBinWidth(const size_t offSet, const size_t specIndex, const MantidVec::iterator theNorms, const MantidVec::iterator errorSquared) const
{
/*  //normally this is false but handling this would mean more combinations of distribution/raw counts workspaces could be accepted
  if (m_convToDistr)
  {
    for(int i = 0; i < theNorms.size(); ++i)
    {
      const double width = ???;
      *(theNorms+i) *= width;
      *(errorSquared+i) *= width*width;
    }
  }*/
  
  // if any bins are masked it is normally a small proportion
  if ( m_dataWS->hasMaskedBins(specIndex) )
  {
    // Get a reference to the list of masked bins
    const MatrixWorkspace::MaskList & mask = m_dataWS->maskedBins(specIndex);
    // Now iterate over the list, adjusting the weights for the affected bins
    MatrixWorkspace::MaskList::const_iterator it;
    for (it = mask.begin(); it != mask.end(); ++it)
    {
      size_t outBin = it->first;
      if ( outBin <  offSet )
      {
        // this masked bin wasn't in the range being delt with anyway
        continue;
      }
      outBin -= offSet;
      // The weight for this masked bin is 1 - the degree to which this bin is masked
      const double factor = 1.0-(it->second);
      *(theNorms+outBin) *= factor;
      *(errorSquared+outBin) *= factor*factor;
    }
  }
}
/** Fills a vector with the Q values calculated from the wavelength bin centers from the input workspace and
*  the workspace geometry as Q = 4*pi*sin(theta)/lambda
*  @param[in] specInd the spectrum to calculate
*  @param[in] doGravity if to include gravity in the calculation of Q
*  @param[in] offset index number of the first input bin to use
*  @param[out] Qs points to a preallocated array that is large enough to contain all the calculated Q values
*  @throw NotFoundError if the detector associated with the spectrum is not found in the instrument definition
*/
void Q1D2::convertWavetoQ(const size_t specInd, const bool doGravity, const size_t offset, MantidVec::iterator Qs) const
{
  static const double FOUR_PI=4.0*M_PI;
  
  IDetector_const_sptr det = m_dataWS->getDetector(specInd);

  // wavelengths (lamda) to be converted to Q
  MantidVec::const_iterator waves = m_dataWS->readX(specInd).begin() + offset;
  // going from bin boundaries to bin centered x-values the size goes down one
  const MantidVec::const_iterator end = m_dataWS->readX(specInd).end() - 1;
  if (doGravity)
  {
    GravitySANSHelper grav(m_dataWS, det);
    for( ; waves !=end; ++Qs, ++waves)
    {
      // the HistogramValidator at the start should ensure that we have one more bin on the input wavelengths
      const double lambda = 0.5*(*(waves+1)+(*waves));
      // as the fall under gravity is wavelength dependent sin theta is now different for each bin with each detector 
      const double sinTheta = grav.calcSinTheta(lambda);
      // Now we're ready to go to Q
      *Qs = FOUR_PI*sinTheta/lambda;
    }
  }
  else
  {
    // Calculate the Q values for the current spectrum, using Q = 4*pi*sin(theta)/lambda
    const double factor = 2.0* FOUR_PI*sin( m_dataWS->detectorTwoTheta(det)/2.0 );
    for( ; waves !=end; ++Qs, ++waves)
    {
      // the HistogramValidator at the start should ensure that we have one more bin on the input wavelengths
      *Qs = factor/(*(waves+1)+(*waves));
    }
  }
}
/** This is a slightly "clever" method as it makes some guesses about where is best
*  to look for the right Q bin based on the fact that the input Qs (calcualted from wavelengths) tend
*  to go down while the output Qs are always in accending order
*  @param[in] OutQs the array of output Q bin boundaries, this finds the bin that contains the QIn value
*  @param[in] QToFind the Q value to find the correct bin for
*  @param[in, out] loc points to the bin boundary (in the OutQs array) whos Q is higher than QToFind and higher by the smallest amount. Algorithm starts by checking the value of loc passed and then all the bins _downwards_ through the array
*/
void Q1D2::getQBinPlus1(const MantidVec & OutQs, const double QToFind, MantidVec::const_iterator & loc) const
{
  if ( loc != OutQs.end() )
  {
    while ( loc != OutQs.begin() )
    {
      if ( (QToFind >= *(loc-1)) && (QToFind < *loc) )
      {
        return;
      }
      --loc;
    }
    if ( QToFind < *loc )
    {
      //QToFind is outside the array leave loc == OutQs.begin()
      return;
    }
  }
  else //loc == OutQs.end()
  {
    if ( OutQs.empty() || QToFind > *(loc-1) )
    {
      //outside the array leave loc == OutQs.end()
      return;
    }
  }

  // we are lost, normally the order of the Q values means we only get here on the first iteration. It's slow
  loc = std::lower_bound(OutQs.begin(), OutQs.end(), QToFind);
}
/** Map all the detectors onto the spectrum of the output
*  @param[in] specIndex the spectrum to add
*  @param[out] specMap the map in the output workspace to write to
*  @param[in] inSpecMap spectrum data
*  @param[out] outputWS the workspace with the spectra axis
*/
void Q1D2::updateSpecMap(const size_t specIndex, API::SpectraDetectorMap * const specMap, const Geometry::ISpectraDetectorMap & inSpecMap, API::MatrixWorkspace_sptr outputWS) const
{
  Axis* const spectraAxis = m_dataWS->getAxis(1);
  if (spectraAxis->isSpectra())
  {
    specid_t newSpectrumNo = outputWS->getAxis(1)->spectraNo(0) = spectraAxis->spectraNo(specIndex);
    specMap->addSpectrumEntries(newSpectrumNo,inSpecMap.getDetectors(spectraAxis->spectraNo(specIndex)));
  }
}
/** Divides the number of counts in each output Q bin by the wrighting ("number that would expected to arrive")
*  The errors are propogated using the uncorrolated error estimate for multiplication/division
*  @param[in] normSum the weighting for each bin
*  @param[in] normError2 square of the error on the normalization
*  @param[in, out] counts counts in each bin
*  @param[in, out] errors input the _square_ of the error on each bin, output the total error (unsquared)
*/
void Q1D2::normalize(const MantidVec & normSum, const MantidVec & normError2, MantidVec & counts, MantidVec & errors) const
{
  for (size_t k = 0; k < counts.size(); ++k)
  {
    // the normalisation is a = b/c where b = counts c =normalistion term
    const double c = normSum[k];
    const double a = counts[k] /= c;
    // when a = b/c, the formula for Da, the error on a, in terms of Db, etc. is (Da/a)^2 = (Db/b)^2 + (Dc/c)^2
    //(Da)^2 = ((Db/b)^2 + (Dc/c)^2)*(b^2/c^2) = ((Db/c)^2 + (b*Dc/c^2)^2) = (Db^2 + (b*Dc/c)^2)/c^2 = (Db^2 + (Dc*a)^2)/c^2
    //this will work as long as c>0, but then the above formula above can't deal with 0 either
    const double aOverc = a/c;
    errors[k] = std::sqrt(errors[k]/(c*c) + normError2[k]*aOverc*aOverc);
  }
}

} // namespace Algorithms
} // namespace Mantid

