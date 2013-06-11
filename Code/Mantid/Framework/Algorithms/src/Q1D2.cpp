/*WIKI* 

===Q Unit Conversion===
The equation for <math>Q</math> as function of wavelength, <math>\lambda</math>, and neglecting gravity, is 
:<math>Q = \frac{4\pi}{\lambda} sin(\theta)</math>
where <math>2 \theta</math> is the particle's angle of deflection. If a particle's measured deflection over the sample to the detector (pixel) distance, <math>L_2</math>, is <math>x</math> along the x-axis and <math>y</math> along the y-axis then <math>\theta</math> is 

:<math>\theta = \frac{1}{2} arcsin\left (\frac{\sqrt{x^2+y^2}}{L_2} \right )</math>

Including gravity adds another term to this equation which becomes: 
:<math>\theta = \frac{1}{2} arcsin\left (\frac{ \sqrt{x^2+\left (y+\frac{gm^2}{2h^2} \lambda^2 L_2^2 \right)^2}}{L_2} \right )</math>
where <math>m</math> is the particle's mass, <math>g</math> is the acceleration due to gravity and <math>h</math> is [http://en.wikipedia.org/wiki/Planks_constant plank's constant] (this assumes neutrons are all travelling in horizontal at sample, and that <math>x=y=0</math> would be beam centre at <math>\lambda = 0</math>).

===Normalized Intensity===
This [[Algorithm|algorithm]] takes a workspace of number of neutron counts against [[Units|wavelength]] and creates a workspace of cross section against Q. The output Q bins boundaries are defined by setting the property OutputBinning.

Below is the formula used to calculate the cross section, <math>P_I(Q)</math>, for one bin in the output workspace whose bin number is denoted by I, when the input workspace has just one detector. Each bin is calculated from the sum of all input wavelength bins, n, that evaluate to the same Q using the formula for Q at the top of this page. In equations this relationship between the input bins and the output bins is represented by <math>n \supset I</math> and an example of a set of two bins is shown diagrammatically below.
[[File:Wav_Q_bins.png|Each Q bin contains the sum of many, one, or no wavelength bins|centre]]
 
In the equation the number of counts in the input spectrum number is denoted by <math>S(n)</math>, <math>N(n)</math> is the wavelength dependent correction and <math>\Omega</math> is the [[SolidAngle|solid angle]] of the detector

:<math>P_I(Q) = \frac{ \sum_{n \supset I} S(n)}{\Omega\sum_{n \supset I}N(n)}</math>

The wavelength dependent correction is supplied to the algorithm through the WavelengthAdj property and this workspace must have the same wavelength binning as the input workspace and should be equal to the following:

:<math>N(n) = M(n)\eta(n)T(n)</math>

where <math>M</math>, <math>\eta</math> and <math>T</math> are the monitor counts, detector efficiency and transmission fraction respectively.

Normally there will be many spectra each from a different pixel with a row number <math>i</math> and column number <math>j</math>. Because the value of <math>\theta</math> varies between pixels corresponding input bins (n) from different input spectra can contribute to different output bins (I) i.e. <math>n \supset I</math> will be different for different pixels. For multiple spectra the sum for each output bin will be over the set of input bins in each pixel that have the correct Q, that is <math>\{i, j, n\} \supset \{I\}</math> while  <math>\Omega_{i j}</math> is detector dependent:

:<math>P_I(Q) = \frac{\sum_{\{i, j, n\} \supset \{I\}} S(i,j,n)}{\sum_{\{i, j, n\} \supset \{I\}}M(n)\eta(n)T(n)\Omega_{i j}F_{i j}}</math>

where <math>F</math> is the detector dependent (e.g. flood) scaling specified by the PixelAdj property, and where a <math>\lambda</math> bin <math>n</math> spans more than one <math>Q</math> bin <math>I</math>, it is split assuming a uniform distribution of the counts in <math>\lambda</math>. The normalization takes any [[MaskBins|bin masking]] into account.

Although the units on the y-axis of the output workspace space are quoted in 1/cm note that conversion to a cross section requires scaling by an [[instrument]] dependent absolute units constant.

===Resolution and Cutoffs===
There are two sources of uncertainty in the intensity: the statistical (counting) error and the finite size of the bins, i.e. both time bins and the spatial extent of the detectors (pixels). The first error is reducible by increasing the length of the experiment or bin sizes while the second reduces with smaller bin sizes. The first is represented by the errors on the output workspace but the second is not included in the error calculation although it increases uncertainties and degrades the effective resolution of the data none the less. This algorithm allows the resolution to be improved by removing the bins with the worst resolution.

Normally the bins that give the worst resolution are those near the beam center and with short wavelengths. When the optional properties <math>RadiusCut</math> and <math>WaveCut</math> are set bins from this region of the input workspace are removed from the intensity calculation (both from the numerator and denominator). For a pixel at distance R from the beam center the wavelength cutoff, <math>W_{low}</math>, is defined by the input properties <math>RadiusCut</math> and <math>WaveCut</math> as:

:<math>W_{low} = \frac{WaveCut (RadiusCut-R)}{RadiusCut}</math>

The bin that contains the wavelength <math>W_{low}</math> and all lower indices are excluded from the summations for that detector pixel.

From the equation it is possible to see that for pixels in <math>R > RadiusCut</math> all (positive) wavelengths are included. Also substituting <math>WaveCut = W_{low}</math> we have that <math>R = 0</math> and hence all detectors contribute at wavelengths above <math>WaveCut</math>.

''Practically, it is more likely to be necessary to implement <math>RadiusCut</math> and <math>WaveCut</math> in situations where the scattering near to the beamstop is weak and 'contaminated' by short wavelength scatter. This might arise, for example, when running at long sample-detector distances, or at short sample-detector distances with large diameter beams, or where the sample generates Bragg peaks at low-Q. The best recourse is to check the wavelength overlap. If it is not too bad it may be possible to improve the data presentation simply by altering <math>Q{min}</math> and the binning scheme.''

'''References'''

[http://scripts.iucr.org/cgi-bin/paper?gk0158 R.P. Hjelm Jr. ''J. Appl. Cryst.'' (1988), 21, 618-628].

[http://scripts.iucr.org/cgi-bin/paper?gk0573 P.A. Seeger & R.P. Hjelm Jr. ''J. Appl. Cryst.'' (1991), 24, 467-478].

===Variations on applying the normalization===
It is possible to divide the input workspace by the WavelenghAdj and PixelAdj workspaces prior to calling this algorithm. The results will be same as if these workspaces were passed to Q1D instead when there are high numbers of particle counts. However, in this scheme the probabilities tend to converge on the true high count probabablities more slowly with increasing number of counts and so the result is less accuate.

Depending on the input and output bins there could be a significant difference in CPU time required by these two methods.

===References===
Calculation of Q is from Seeger, P. A. and Hjelm, R. P. Jr, "Small-Angle Neutron Scattering at Pulsed Spallation Sources" (1991) J. Appl '''24''' 467-478

==Previous Versions==

===Version 1===
Before July 2011 the intensity was calculated with an equation like the following:
:<math>P_I(Q) = \frac{ \sum_{\{i, j, n\} \supset \{I\}}G(i,j,n) }{ \sum_{\{i, j, n\} \supset \{I\}} \Omega_{i j} }</math>
where G is the input workspace normally related to the raw counts workspace as:
:<math>G(i,j,n) = S(i,j,n)/(M(n)\eta(n)T(n)F_{i j})</math>
That is the normalization was performed before the Q calculation which gives the same probilities at high numbers of particles counts but weighted noisy, low count data too highly, giving more noise in <math>P_I(Q)</math>.

The error was calculation did not include the errors due the normalization or any corrections.

==== Properties ====

{| border="1" cellpadding="5" cellspacing="0"
!Order
!Name
!Direction
!Type
!Default
!Description
|-
|1
|InputWorkspace
|Input
|MatrixWorkspace
|Mandatory
|The (partly) corrected data in units of wavelength.
|-
|2
|InputForErrors
|Input
|MatrixWorkspace
|Mandatory
|The workspace containing the counts to use for the error calculation. Must also be in units of wavelength and have matching bins to the InputWorkspace.
|-
|3
|OutputWorkspace
|Output
|MatrixWorkspace
|Mandatory
|The workspace name under which to store the result histogram.
|-
|4
|OutputBinning
|Input
|String
|Mandatory
|The bin parameters to use for the final result (in the format used by the [[Rebin]] algorithm).
|-
|5
|AccountForGravity
|Input
|Boolean
|False
|Whether to correct for the effects of gravity.
|}



*WIKI*/
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
#include "MantidDataObjects/Histogram1D.h"
#include "MantidAlgorithms/Qhelper.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/ISpectrum.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Q1D2)

/// Sets documentation strings for this algorithm
void Q1D2::initDocs()
{
  this->setWikiSummary("Converts a workspace of counts in wavelength bins into a workspace of counts verses momentum transfer, Q, assuming completely elastic scattering");
  this->setOptionalMessage("Converts a workspace of counts in wavelength bins into a workspace of counts verses momentum transfer, Q, assuming completely elastic scattering");
}


using namespace Kernel;
using namespace API;
using namespace Geometry;

void Q1D2::init()
{
  auto dataVal = boost::make_shared<CompositeValidator>();
  dataVal->add<WorkspaceUnitValidator>("Wavelength");
  dataVal->add<HistogramValidator>();
  dataVal->add<InstrumentValidator>();
  dataVal->add<CommonBinsValidator>();
  declareProperty(new WorkspaceProperty<>("DetBankWorkspace", "", Direction::Input, dataVal),
    "Particle counts as a function of wavelength");
  declareProperty(new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
    "Name of the workspace that will contain the result of the calculation");
  declareProperty(new ArrayProperty<double>("OutputBinning", boost::make_shared<RebinParamsValidator>()),
        "A comma separated list of first bin boundary, width, last bin boundary. Optionally\n"
        "this can be followed by a comma and more widths and last boundary pairs.\n"
        "Negative width values indicate logarithmic binning.");
  declareProperty(new WorkspaceProperty<>("PixelAdj","", Direction::Input, PropertyMode::Optional),
    "The scaling to apply to each spectrum e.g. for detector efficiency, must have\n"
    "the same number of spectra as the DetBankWorkspace");
  auto wavVal = boost::make_shared<CompositeValidator>();
  wavVal->add<WorkspaceUnitValidator>("Wavelength");
  wavVal->add<HistogramValidator>();
  declareProperty(new WorkspaceProperty<>("WavelengthAdj", "", Direction::Input, PropertyMode::Optional, wavVal),
    "The scaling to apply to each bin to account for monitor counts, transmission\n"
    "fraction, etc");
  declareProperty(new WorkspaceProperty<>("WavePixelAdj", "", Direction::Input, PropertyMode::Optional, dataVal), 
    "The scaling to apply to the normalization factor. It gathers the corrections that depend on pixel and wavelength together.\n"
     "Currently, it takes in account the angle transmission correction.");
  declareProperty("AccountForGravity",false,
    "Whether to correct for the effects of gravity");
  declareProperty("SolidAngleWeighting",true,
      "If true, pixels will be weighted by their solid angle.", Direction::Input);
  auto mustBePositive = boost::make_shared<BoundedValidator<double> >();
  mustBePositive->setLower(0.0);
  declareProperty("RadiusCut", 0.0, mustBePositive,
    "To increase resolution some wavelengths are excluded within this distance from\n"
    "the beam center (mm)");
  declareProperty("WaveCut", 0.0, mustBePositive,
    "To increase resolution by starting to remove some wavelengths below this"
    "freshold (angstrom)");
  declareProperty("OutputParts", false,
    "Set to true to output two additional workspaces which will have the names OutputWorkspace_sumOfCounts "
    "OutputWorkspace_sumOfNormFactors. The division of _sumOfCounts and _sumOfNormFactors equals the workspace"
    " returned by the property OutputWorkspace "
    "(default is false)." );
}
/**
  @ throw invalid_argument if the workspaces are not mututially compatible
*/
void Q1D2::exec()
{
  m_dataWS = getProperty("DetBankWorkspace");
  MatrixWorkspace_const_sptr waveAdj = getProperty("WavelengthAdj");
  MatrixWorkspace_const_sptr pixelAdj = getProperty("PixelAdj");
  MatrixWorkspace_const_sptr wavePixelAdj = getProperty("WavePixelAdj");
  const bool doGravity = getProperty("AccountForGravity");
  m_doSolidAngle = getProperty("SolidAngleWeighting");

  //throws if we don't have common binning or another incompatibility
  Qhelper helper;
  helper.examineInput(m_dataWS, waveAdj, pixelAdj);
  // FIXME: how to examine the wavePixelAdj? 
  g_log.debug() << "All input workspaces were found to be valid\n";
  // normalization as a function of wavelength (i.e. centers of x-value bins)
  double const * const binNorms = waveAdj ? &(waveAdj->readY(0)[0]) : NULL;
  // error on the wavelength normalization
  double const * const binNormEs = waveAdj ? &(waveAdj->readE(0)[0]) : NULL;

  //define the (large number of) data objects that are going to be used in all iterations of the loop below

  // this will become the output workspace from this algorithm
  MatrixWorkspace_sptr outputWS = setUpOutputWorkspace(getProperty("OutputBinning"));

  const MantidVec & QOut = outputWS->readX(0);
  MantidVec & YOut = outputWS->dataY(0);
  MantidVec & EOutTo2 = outputWS->dataE(0);
  // normalisation that is applied to counts in each Q bin
  MantidVec normSum(YOut.size(), 0.0);
  // the error on the normalisation
  MantidVec normError2(YOut.size(), 0.0);

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
      g_log.warning() << "Workspace index " << i << " (SpectrumIndex = " << m_dataWS->getSpectrum(i)->getSpectrumNo() << ") has no detector assigned to it - discarding" << std::endl;
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
    //const size_t wavStart = waveLengthCutOff(i);
    const size_t wavStart = helper.waveLengthCutOff(m_dataWS, getProperty("RadiusCut"), getProperty("WaveCut"), i);
    if (wavStart >=  m_dataWS->readY(i).size())
    {
      // all the spectra in this detector are out of range
      continue;
    }
    
    const size_t numWavbins = m_dataWS->readY(i).size()-wavStart;
    // make just one call to new to reduce CPU overhead on each thread, access to these 
    // three "arrays" is via iterators
    MantidVec _noDirectUseStorage_(3*numWavbins);
    //normalization term
    MantidVec::iterator norms = _noDirectUseStorage_.begin();
    // the error on these weights, it contributes to the error calculation on the output workspace
    MantidVec::iterator normETo2s = norms + numWavbins;
    // the Q values calculated from input wavelength workspace
    MantidVec::iterator QIn = normETo2s + numWavbins;

    // the weighting for this input spectrum that is added to the normalization
    calculateNormalization(wavStart, i, pixelAdj, wavePixelAdj, binNorms, binNormEs, norms, normETo2s);

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
        // the actual Q-bin to add something to
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
      progress.report("Computing I(Q)");

      // Add up the detector IDs in the output spectrum at workspace index 0
      const ISpectrum * inSpec = m_dataWS->getSpectrum(i);
      ISpectrum * outSpec = outputWS->getSpectrum(0);
      outSpec->addDetectorIDs( inSpec->getDetectorIDs() );
    }

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  bool doOutputParts = getProperty("OutputParts");
  if (doOutputParts)
  {
      MatrixWorkspace_sptr ws_sumOfCounts = WorkspaceFactory::Instance().create(outputWS);
      ws_sumOfCounts->dataX(0) = outputWS->dataX(0);
      ws_sumOfCounts->dataY(0) = outputWS->dataY(0);
      for (size_t i = 0; i < outputWS->dataE(0).size(); i++)
      {
        ws_sumOfCounts->dataE(0)[i] = sqrt(outputWS->dataE(0)[i]);
      }      

      MatrixWorkspace_sptr ws_sumOfNormFactors = WorkspaceFactory::Instance().create(outputWS);
      ws_sumOfNormFactors->dataX(0) = outputWS->dataX(0);
      for (size_t i = 0; i < ws_sumOfNormFactors->dataY(0).size(); i++)
      {
        ws_sumOfNormFactors->dataY(0)[i] = normSum[i];
        ws_sumOfNormFactors->dataE(0)[i] = sqrt(normError2[i]);
      }

      helper.outputParts(this, ws_sumOfCounts, ws_sumOfNormFactors);
  }


  progress.report("Normalizing I(Q)");
  //finally divide the number of counts in each output Q bin by its weighting
  normalize(normSum, normError2, YOut, EOutTo2);

  setProperty("OutputWorkspace",outputWS);
}


/** Creates the output workspace, its size, units, etc.
*  @param binParams the bin boundary specification using the same same syntax as param the Rebin algorithm
*  @return A pointer to the newly-created workspace
*/
API::MatrixWorkspace_sptr Q1D2::setUpOutputWorkspace(const std::vector<double> & binParams) const
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

  outputWS->getSpectrum(0)->clearDetectorIDs();
  outputWS->getSpectrum(0)->setSpectrumNo(1);

  return outputWS;
}


/** Calculate the normalization term for each output bin
*  @param wavStart [in] the index number of the first bin in the input wavelengths that is actually being used
*  @param specInd [in] the spectrum to calculate
*  @param pixelAdj [in] if not NULL this is workspace contains single bins with the adjustments, e.g. detector efficencies, for the given spectrum index
*  @param wavePixelAdj [in] if not NULL this is workspace that contains the adjustments for the pixels and wavelenght dependend values. 
*  @param binNorms [in] pointer to a contigious array of doubles that are the wavelength correction from waveAdj workspace, can be NULL
*  @param binNormEs [in] pointer to a contigious array of doubles which corrospond to the corrections and are their errors, can be NULL
*  @param norm [out] normalization for each bin, including soild angle, pixel correction, the proportion that is not masked and the normalization workspace
*  @param normETo2 [out] this pointer must point to the end of the norm array, it will be filled with the total of the error on the normalization
*/
  void Q1D2::calculateNormalization(const size_t wavStart, const size_t specInd, API::MatrixWorkspace_const_sptr pixelAdj, API::MatrixWorkspace_const_sptr wavePixelAdj, double const * const binNorms, double const * const binNormEs, const MantidVec::iterator norm, const MantidVec::iterator normETo2) const
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
      if (wavePixelAdj)
        // pass the iterator for the wave pixel Adj dependent 
        addWaveAdj(binNorms+wavStart, binNormEs+wavStart, norm, normETo2,
                   wavePixelAdj->readY(specInd).begin() + wavStart, 
                   wavePixelAdj->readE(specInd).begin() + wavStart);
      else
        addWaveAdj(binNorms+wavStart, binNormEs+wavStart, norm, normETo2);
    }
  normToMask(wavStart, specInd, norm, normETo2);
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

  if (m_doSolidAngle)
    weight = m_dataWS->getDetector(specIndex)->solidAngle(samplePos);
  else
    weight = 1.0;

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
 
  //use the fact that error array follows straight after the normalization array
  const MantidVec::const_iterator end = e2InOut;
  for( ; bInOut != end; ++e2InOut, ++c, ++Dc, ++bInOut)
  {
    //first the error
    *e2InOut = ( (*e2InOut)*(*c)*(*c) )+( (*Dc)*(*Dc)*(*bInOut)*(*bInOut) );
    // now the actual calculation a = b*c
    *bInOut = (*bInOut)*(*c);
  }
}
/** Calculates the contribution to the normalization terms from each bin in a spectrum
*  @param[in] c pointer to the start of a contigious array of wavelength dependent normalization terms
*  @param[in] Dc pointer to the start of a contigious array that corrosponds to wavelength dependent term, having its error
*  @param[in,out] bInOut normalization for each bin, this method multiplise this by the proportion that is not masked and the normalization workspace
*  @param[in, out] e2InOut this array must follow straight after the normalization array and will contain the error on the normalisation term before the WavelengthAdj term
* @param[in] wavePixelAdjData normalization correction for each bin for each detector pixel. 
* @param[in] wavePixelAdjError normalization correction incertainty for each bin for each detector pixel.
*/
void Q1D2::addWaveAdj(const double * c, const double * Dc, MantidVec::iterator bInOut, MantidVec::iterator e2InOut, 
                      MantidVec::const_iterator wavePixelAdjData, MantidVec::const_iterator wavePixelAdjError) const
{
  // normalize by the wavelength dependent correction, keeping the percentage errors the same
  // the error when a = b*c*e, the formula for Da, the error on a, in terms of Db, etc. is 
  // (Da/a)^2 = (Db/b)^2 + (Dc/c)^2 + (De/e)^2  
  //(Da)^2 = ((Db*a/b)^2 + (Dc*a/c)^2) + (De * a/e)^2
  // But: a/b = c*e; a/c = b*e; a/e = b*c; 
  // So: 
  // (Da)^2 = (c*e*Db)^2 + (b*e*Dc)^2 + (b*c*De)^2
  // 
  // Consider: 
  // Da = Error (e2InOut)
  // Db^2 = PixelDependentError (e2InOut)
  // b  = PixelDependentValue (bInOut) 
  // c  = WaveDependentValue (c)
  // Dc = WaveDependentError (Dc)
  // e  = PixelWaveDependentValue (wavePixelAdjData)
  // De = PiexlWaveDependentError (wavePixelAdjError)

  //use the fact that error array follows straight after the normalization array
  const MantidVec::const_iterator end = e2InOut;
 for( ; bInOut != end; ++e2InOut, ++c, ++Dc, ++bInOut, ++wavePixelAdjData, ++wavePixelAdjError)
  {
    //first the error
    *e2InOut = ( (*e2InOut)*(*c)*(*c)*(*wavePixelAdjData)*(*wavePixelAdjData) ) + 
      ( (*Dc)*(*Dc)*(*bInOut)*(*bInOut)*(*wavePixelAdjData)*(*wavePixelAdjData) ) +
      ((*wavePixelAdjError)*(*wavePixelAdjError)*(*c)*(*c)*(*bInOut)*(*bInOut));
    // now the actual calculation a = b*c*e : Pixel * Wave * PixelWave
    *bInOut = (*bInOut)*(*c) * (*wavePixelAdjData);
  }
}

/** Scaled to bin masking, to the normalization
*  @param[in] offSet the inex number of the first bin in the input wavelengths that is actually being used
*  @param[in] specIndex the spectrum to calculate
*  @param[in,out] theNorms normalization for each bin, this is multiplied by the proportion that is not masked and the normalization workspace
*  @param[in,out] errorSquared the running total of the square of the uncertainty in the normalization
*/
void Q1D2::normToMask(const size_t offSet, const size_t specIndex, const MantidVec::iterator theNorms, const MantidVec::iterator errorSquared) const
{  
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

