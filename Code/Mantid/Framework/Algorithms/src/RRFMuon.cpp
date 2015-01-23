//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/RRFMuon.h"
#include "MantidKernel/ArrayProperty.h"
#include <boost/shared_array.hpp>

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
  m_freq = getProperty("Frequency");
  // Get phase
  m_phase = getProperty("Phase");
  // Get number of histograms
  m_nHisto = inputWs->getNumberHistograms();
  if ( m_nHisto != 2 )
  {
    throw std::runtime_error("Invalid number of spectra in input workspace");
  }
  // Set number of data points
  m_nData = inputWs->blocksize();

  // Compute the RRF polarization
  const double twoPiFreq = 2. * M_PI * m_freq;
  MantidVec time  = inputWs->readX(0); // X axis: time
  MantidVec labRe = inputWs->readY(0); // Lab-frame polarization (real part)
  MantidVec labIm = inputWs->readY(1); // Lab-frame polarization (imaginary part)
  MantidVec rrfRe(m_nData), rrfIm(m_nData); // Rotating Reference frame (RRF) polarizations
  for (size_t t=0; t<m_nData; t++)
  {
    rrfRe [t] =  labRe [t] * cos(twoPiFreq * time[t] + m_phase) + labIm [t] * sin(twoPiFreq * time[t] + m_phase);
    rrfIm [t] = -labRe [t] * sin(twoPiFreq * time[t] + m_phase) + labIm [t] * cos(twoPiFreq * time[t] + m_phase);
  }

  // Create output workspace to put results into
  API::MatrixWorkspace_sptr outputWs = boost::dynamic_pointer_cast<API::MatrixWorkspace>(
    API::WorkspaceFactory::Instance().create("Workspace2D", m_nHisto, m_nData+1, m_nData));
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


}
}