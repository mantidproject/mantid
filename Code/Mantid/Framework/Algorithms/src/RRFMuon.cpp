//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/RRFMuon.h"


#define REAL(z,i) ((z)[2*(i)])
#define IMAG(z,i) ((z)[2*(i)+1])

namespace Mantid
{
namespace Algorithms
{

using namespace Kernel;
using API::Progress;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(RRFMuon)

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void RRFMuon::init()
{

  declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace", "", Direction::Input), 
    "Name of the input workspace containing the spectra in the lab frame");

  declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace", "", Direction::Output), 
    "Name of the output workspace containing the spectra in the RRF" );

  declareProperty(new PropertyWithValue<double>("Frequency", 0, Direction::Input), 
    "Frequency of the oscillations");

  std::vector<std::string> unitOptions;
  unitOptions.push_back("MHz");
  unitOptions.push_back("Gauss");
  unitOptions.push_back("Mrad/s");
  declareProperty("Frequency units", "MHz",
                  boost::make_shared<StringListValidator>(unitOptions),
                  "The frequency units");

  declareProperty(new PropertyWithValue<double>("Phase", 0, Direction::Input), 
    "Phase accounting for any misalignment of the counters");

}

/** Executes the algorithm
 *
 */
void RRFMuon::exec()
{
	// Get input workspace containing polarization in the lab-frame
  API::MatrixWorkspace_sptr inputWs = getProperty("InputWorkspace");
  // Get frequency
  double freq = getProperty("Frequency");
  // Get units
  std::string units = getProperty("Frequency units");
  // Convert frequency to input workspace X units
  double factor = unitConversionFactor(inputWs->getAxis(0)->unit()->label().ascii(),units);
  // Get phase
  double phase = getProperty("Phase");
  // Get number of histograms
  size_t nHisto = inputWs->getNumberHistograms();
  if ( nHisto != 2 )
  {
    throw std::runtime_error("Invalid number of spectra in input workspace");
  }
  // Set number of data points
  size_t nData = inputWs->blocksize();

  // Compute the RRF polarization
  const double twoPiFreq = 2. * M_PI * freq * factor;
  MantidVec time  = inputWs->readX(0); // X axis: time
  MantidVec labRe = inputWs->readY(0); // Lab-frame polarization (real part)
  MantidVec labIm = inputWs->readY(1); // Lab-frame polarization (imaginary part)
  MantidVec rrfRe(nData), rrfIm(nData); // Rotating Reference frame (RRF) polarizations
  for (size_t t=0; t<nData; t++)
  {
    rrfRe [t] =  labRe [t] * cos(twoPiFreq * time[t] + phase) + labIm [t] * sin(twoPiFreq * time[t] + phase);
    rrfIm [t] = -labRe [t] * sin(twoPiFreq * time[t] + phase) + labIm [t] * cos(twoPiFreq * time[t] + phase);
  }

  // Create output workspace to put results into
  API::MatrixWorkspace_sptr outputWs = boost::dynamic_pointer_cast<API::MatrixWorkspace>(
    API::WorkspaceFactory::Instance().create("Workspace2D", nHisto, nData+1, nData));
  outputWs->getAxis(0)->unit() = inputWs->getAxis(0)->unit();

  // Put results into output workspace
  // Real RRF polarization
  outputWs->getSpectrum(0)->setSpectrumNo(1);
  outputWs->dataX(0) = inputWs->readX(0);
  outputWs->dataY(0) = rrfRe;
  // Imaginary RRF polarization
  outputWs->getSpectrum(1)->setSpectrumNo(2);
  outputWs->dataX(1) = inputWs->readX(1);
  outputWs->dataY(1) = rrfIm;

  // Set output workspace
  setProperty("OutputWorkspace", outputWs);

}

/** Gets factor to convert frequency units to input workspace units
 *  @param uin :: [input] input workspace units
 *  @param uuser :: [input] units selected by user
 */
double RRFMuon::unitConversionFactor(std::string uin, std::string uuser){

  if ( (uin == "microsecond" ) ){

    if ( uuser == "MHz" ) {
      return 1.0;
    } else if ( uuser == "Gauss" ) {
      std::cout << "FACTOR " << 2.0*M_PI*0.0001 << std::endl;
      return 2.0*M_PI*135.538817*0.0001;
    } else if ( uuser == "Mrad/s" ) {
      std::cout << "FACTOR " << 2.0*M_PI << std::endl;
      return 2.0*M_PI;
    } else {
      throw std::runtime_error("Could not find units");
    }

  } else {
    throw std::runtime_error("X units must be in microseconds");
  }

}

}
}