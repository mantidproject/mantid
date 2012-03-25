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
[[File:wav_Q_bins.png|Each Q bin contains the sum of many, one, or no wavelength bins|centre]]
 
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

[http://scripts.iucr.org/cgi-bin/paper?gk0158/R.P. Hjelm Jr. ''J. Appl. Cryst.'' (1988), 21, 618-628].

[http://scripts.iucr.org/cgi-bin/paper?gk0573/P.A. Seeger & R.P. Hjelm Jr. ''J. Appl. Cryst.'' (1991), 24, 467-478].

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
#include "MantidAlgorithms/Q1D.h"
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
DECLARE_ALGORITHM(Q1D)

/// Sets documentation strings for this algorithm
void Q1D::initDocs()
{
  this->setWikiSummary("Part of the 1D data reduction chain for SANS instruments. ");
  this->setOptionalMessage("Part of the 1D data reduction chain for SANS instruments.");
}


using namespace Kernel;
using namespace API;
using namespace Geometry;

void Q1D::init()
{
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<HistogramValidator>();
  wsValidator->add<InstrumentValidator>();
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,wsValidator));
  declareProperty(new WorkspaceProperty<>("InputForErrors","",Direction::Input));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));

  declareProperty(new ArrayProperty<double>("OutputBinning", boost::make_shared<RebinParamsValidator>()));

  declareProperty("AccountForGravity",false);
}

void Q1D::exec()
{
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_const_sptr errorsWS = getProperty("InputForErrors");

  // Check that the input workspaces match up
  if ( ! WorkspaceHelpers::matchingBins(inputWS,errorsWS) )
  {
    g_log.error("The input workspaces must have matching bins");
    throw std::runtime_error("The input workspaces must have matching bins");
  }

  // Calculate the output binning
  const std::vector<double> binParams = getProperty("OutputBinning");
  MantidVecPtr XOut;
  const int sizeOut = VectorHelper::createAxisFromRebinParams(binParams,XOut.access());

  // Now create the output workspace
  MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(inputWS,1,sizeOut,sizeOut-1);
  outputWS->getAxis(0)->unit() = UnitFactory::Instance().create("MomentumTransfer");
  outputWS->setYUnitLabel("1/cm");
  setProperty("OutputWorkspace",outputWS);
  // Set the X vector for the output workspace
  outputWS->setX(0,XOut);
  MantidVec& YOut = outputWS->dataY(0);
  // Don't care about errors here - create locals
  MantidVec EIn(inputWS->readE(0).size());
  MantidVec EOutDummy(sizeOut-1);
  MantidVec emptyVec(1);
  // Create temporary vectors for the summation & rebinning of the 'errors' workspace
  MantidVec errY(sizeOut-1),errE(sizeOut-1);
  // And one for the solid angles
  MantidVec anglesSum(sizeOut-1);

  const int numSpec = static_cast<int>(inputWS->getNumberHistograms());

  // Start with no detectors in the single spectrum of the output worskpace
  outputWS->getSpectrum(0)->clearDetectorIDs();
  specid_t newSpectrumNo = -1;

  // Set the progress bar (1 update for every one percent increase in progress)
  Progress progress(this, 0.0, 1.0, numSpec);

  const V3D samplePos = inputWS->getInstrument()->getSample()->getPos();

  const bool doGravity = getProperty("AccountForGravity");


  const int xLength = static_cast<int>(inputWS->readX(0).size());
  const double fmp=4.0*M_PI;

  PARALLEL_FOR3(inputWS,outputWS,errorsWS)
  for (int i = 0; i < numSpec; ++i)
  {
    PARALLEL_START_INTERUPT_REGION
    // Get the pixel relating to this spectrum
    IDetector_const_sptr det;
    try {
      det = inputWS->getDetector(i);
    } catch (Exception::NotFoundError&) {
      g_log.warning() << "Spectrum index " << i << " has no detector assigned to it - discarding" << std::endl;
      // Catch if no detector. Next line tests whether this happened - test placed
      // outside here because Mac Intel compiler doesn't like 'continue' in a catch
      // in an openmp block.
    }
    // If no detector found or if detector is masked, skip onto the next spectrum
    if ( !det || det->isMasked() ) continue;

    // Map all the detectors onto the spectrum of the output
    if (newSpectrumNo == -1)
    {
      PARALLEL_CRITICAL(q1d_a)
      {
        if( newSpectrumNo == -1 )
        {
          // The first spectrum in the input WS = the spectrum number in the output ws
          specid_t newSpectrumNo = inputWS->getSpectrum(i)->getSpectrumNo();
          outputWS->getSpectrum(0)->setSpectrumNo(newSpectrumNo);
        }
      }
    }
    PARALLEL_CRITICAL(q1d_b)
    {
      /* Write to shared memory - must protect */
      outputWS->getSpectrum(0)->addDetectorIDs( inputWS->getSpectrum(i)->getDetectorIDs() );
    }

    // Get the current spectrum for both input workspaces - not references, have to reverse below
    const MantidVec& XIn = inputWS->readX(i);
    MantidVec YIn = inputWS->readY(i);
    MantidVec errYIn = errorsWS->readY(i);
    MantidVec errEIn = errorsWS->readE(i);
    // A temporary vector to store intermediate Q values
    MantidVec Qx(xLength);

    if (doGravity)
    {
      // Get the vector to get at the 3 components of the pixel position
      GravitySANSHelper grav(inputWS, det);
      for ( int j = 0; j < xLength; ++j)
      {
        // as the fall under gravity is wavelength dependent sin theta is now different for each bin with each detector 
        const double sinTheta = grav.calcSinTheta(XIn[j]);
        // Now we're ready to go to Q
        Qx[xLength-j-1] = fmp*sinTheta/XIn[j];
      }
    }
    else
    {
      // Calculate the Q values for the current spectrum
      const double sinTheta = sin( inputWS->detectorTwoTheta(det)/2.0 );
      const double factor = fmp*sinTheta;
      for ( int j = 0; j < xLength; ++j)
      {
        Qx[xLength-j-1] = factor/XIn[j];
      }
    }

    // Unfortunately, have to reverse data vectors for rebin functions to work
    std::reverse(YIn.begin(),YIn.end());
    std::reverse(errYIn.begin(),errYIn.end());
    std::reverse(errEIn.begin(),errEIn.end());
    // Pass to rebin, flagging as a distribution. Need to protect from writing by more that one thread at once.
    PARALLEL_CRITICAL(q1d_c)
    {
      VectorHelper::rebin(Qx,YIn,EIn,*XOut,YOut,EOutDummy,true,true);
      VectorHelper::rebin(Qx,errYIn,errEIn,*XOut,errY,errE,true,true);
    }

    // Now summing the solid angle across the appropriate range
    const double solidAngle = det->solidAngle(samplePos);
    // At this point we check for masked bins in the input workspace and modify accordingly
    if ( inputWS->hasMaskedBins(i) )
    {
      MantidVec included_bins,solidAngleVec;
      included_bins.push_back(Qx.front());
      // Get a reference to the list of masked bins
      const MatrixWorkspace::MaskList& mask = inputWS->maskedBins(i);
      // Now iterate over the list, adjusting the weights for the affected bins
      // Since we've reversed Qx, we need to reverse the list too
      MatrixWorkspace::MaskList::const_reverse_iterator it;
      for (it = mask.rbegin(); it != mask.rend(); ++it)
      {
        const double currentX = Qx[xLength-2-(*it).first];
        // Add an intermediate bin with full weight if masked bins aren't consecutive
        if (included_bins.back() != currentX) 
        {
          solidAngleVec.push_back(solidAngle);
          included_bins.push_back(currentX);
        }
        // The weight for this masked bin is 1 - the degree to which this bin is masked
        solidAngleVec.push_back( solidAngle * (1.0-(*it).second) );
        included_bins.push_back( Qx[xLength-1-(*it).first]);
      }
      // Add on a final bin with full weight if masking doesn't go up to the end
      if (included_bins.back() != Qx.back()) 
      {
        solidAngleVec.push_back(solidAngle);
        included_bins.push_back(Qx.back());
      }
      
      // Create a zero vector for the errors because we don't care about them here
      const MantidVec zeroes(solidAngleVec.size(),0.0);
      PARALLEL_CRITICAL(q1d_d)
      {
        // Rebin the solid angles - note that this is a distribution
        VectorHelper::rebin(included_bins,solidAngleVec,zeroes,*XOut,anglesSum,EOutDummy,true,true);
      }
    }
    else // No masked bins
    {
      // Two element vector to define the range
      MantidVec xRange(2);
      xRange[0] = Qx.front();
      xRange[1] = Qx.back();
      // Single element vector containing the solid angle
      MantidVec solidAngleVec(1, solidAngle);
      PARALLEL_CRITICAL(q1d_e)
     {
        // Rebin the solid angles - note that this is a distribution
        VectorHelper::rebin(xRange,solidAngleVec,emptyVec,*XOut,anglesSum,EOutDummy,true,true);
      }
    }

    progress.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // Now need to loop over resulting vectors dividing by solid angle
  // and setting fractional error to match that in the 'errors' workspace
  MantidVec& EOut = outputWS->dataE(0);
  for (int k = 0; k < sizeOut-1; ++k)
  {
    YOut[k] /= anglesSum[k];
    const double fractional = errY[k] ? std::sqrt(errE[k])/errY[k] : 0.0;
    EOut[k] = fractional*YOut[k];
  }

  outputWS->isDistribution(true);
}

} // namespace Algorithms
} // namespace Mantid

