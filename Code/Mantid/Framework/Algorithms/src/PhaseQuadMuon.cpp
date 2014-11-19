//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/ArrayProperty.h"
#include "MantidAlgorithms/PhaseQuadMuon.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/FrameworkManager.h"

namespace Mantid
{
namespace Algorithms
{

using namespace Kernel;
using API::Progress;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(PhaseQuadMuon)

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void PhaseQuadMuon::init()
{

  declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace", "", Direction::Input), 
    "Name of the input workspace");

  declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace", "", Direction::Output), 
    "Name of the output workspace" );

  declareProperty(new API::FileProperty("PhaseTable", "", API::FileProperty::Load, ".INF"),
    "The name of the list of phases for each spectrum");
}

/** Executes the algorithm
 *
 */
void PhaseQuadMuon::exec()
{
	// Get input workspace
  API::MatrixWorkspace_sptr inputWs = getProperty("InputWorkspace");

  // Get phase table
  std::string filename = getPropertyValue("PhaseTable");
  loadPhaseTable ( filename );

  // Set number of data points per histogram
  m_nData = inputWs->getSpectrum(0)->readY().size();

  // Check number of histograms in inputWs match number of detectors in phase table
  if (m_nHist != inputWs->getNumberHistograms())
  {
    throw std::runtime_error("PhaseQuad: Number of histograms in phase table does not match number of spectra in workspace");
  }

  // Create temporary workspace to perform operations on it
  API::MatrixWorkspace_sptr tempWs = boost::dynamic_pointer_cast<API::MatrixWorkspace>(
    API::WorkspaceFactory::Instance().create("Workspace2D", m_nHist, m_nData+1, m_nData));

  // Create output workspace with two spectra (squashograms)
  API::MatrixWorkspace_sptr outputWs = getProperty("OutputWorkspace");
  outputWs = boost::dynamic_pointer_cast<API::MatrixWorkspace>(
    API::WorkspaceFactory::Instance().create("Workspace2D", 2, m_nData+1, m_nData));
  outputWs->getAxis(0)->unit() = inputWs->getAxis(0)->unit();

  // Rescale detector efficiency to maximum value
  normaliseAlphas(m_histData);
  // Remove exponential decay and save results into tempWs
  loseExponentialDecay(inputWs,tempWs);
  // Compute squashograms
  squash (tempWs, outputWs);
  // Regain exponential decay
  regainExponential(outputWs);
  
  setProperty("OutputWorkspace", outputWs);
  //setProperty("OutputWorkspace1", tempWs1); // TODO: change this
  //setProperty("OutputWorkspace2", tempWs2); // TODO: change this
  // TODO remove temporary workspace tempWs
  //Mantid::API::FrameworkManager::Instance().deleteWorkspace(tempWs);


}

//----------------------------------------------------------------------------------------------
/** Load PhaseTable file to a vector of HistData.
* @param filename :: [input] phase table .inf filename
*/
void PhaseQuadMuon::loadPhaseTable(const std::string& filename )
{

  std::ifstream input(filename.c_str(), std::ios_base::in);

  if (input.is_open())
  {

    if ( input.eof() )
    {
      throw Exception::FileError("PhaseQuad: File is empty", filename);
    }
    else
    {
      std::string line;

      // Header of .INF file is as follows:
      //
      // Comment on the output file
      // Top row of numbers are:
      // #histos, typ. first good bin#, typ. bin# when pulse over, mean lag.
      // Tabulated numbers are, per histogram:
      // det ok, asymmetry, phase, lag, deadtime_c, deadtime_m.
      //
      std::getline( input, line ); // Skip first line in header
      std::getline( input, line ); // Skip second line
      std::getline( input, line ); // ...
      std::getline( input, line );
      std::getline( input, line );

      // Read first useful line
      input >> m_nHist >> m_tValid >> m_tNoPulse >> m_meanLag;

      // Read histogram data
      int cont=0;
      HistData tempData;
      while( input >> tempData.detOK >> tempData.alpha >> tempData.phi >> tempData.lag >> tempData.dead >> tempData.deadm )
      {
        m_histData.push_back (tempData);
        cont++;
      }

      if ( cont != m_nHist )
      {
        if ( cont < m_nHist )
        {
          throw Exception::FileError("PhaseQuad: Lines missing in phase table ", filename);
        }
        else
        {
          throw Exception::FileError("PhaseQuad: Extra lines in phase table ", filename);
        }
      }

    }
  }
  else
  {
    // Throw exception if file cannot be opened
    throw std::runtime_error("PhaseQuad: Unable to open PhaseTable");
  }

}

//----------------------------------------------------------------------------------------------
/** Rescale detector efficiencies to maximum value.
* @param histData :: vector of HistData containing detector efficiencies
*/
void PhaseQuadMuon::normaliseAlphas (std::vector<HistData>& histData)
{
  double max=0;
  for (size_t h=0; h<m_nHist; h++)
  {
    if (histData[h].alpha>max)
    {
      max = histData[h].alpha;
    }
  }
  if ( !max )
  {
    throw std::runtime_error("PhaseQuad: Could not rescale efficiencies");
  }

  for (size_t h=0; h<m_nHist; h++)
  {
    histData[h].alpha /= max;
  }
}


//----------------------------------------------------------------------------------------------
/** Remove exponential decay from input histograms, i.e., calculate asymmetry
* @param inputWs :: input workspace containing the spectra
* @param outputWs :: output workspace to hold temporary results
*/
void PhaseQuadMuon::removeExponentialDecay (API::MatrixWorkspace_sptr inputWs, API::MatrixWorkspace_sptr outputWs)
{
  // Muon decay: N(t) = N0 exp(-t/tau) [ 1 + a P(t) ]
  // N(t) - N0 exp(-t/tau) = 1 + a P(t)
  // Int N(t) dt = N0 Int exp(-t/tau) dt

  for (size_t h=0; h<inputWs->getNumberHistograms(); h++)
  {
    auto specIn = inputWs->getSpectrum(h);
    auto specOut = outputWs->getSpectrum(h);

    specOut->dataX()=specIn->readX();

    double sum1, sum2;
    sum1=sum2=0;
    for(int i=0; i<m_nData; i++)
    {
      if ( specIn->readX()[i]>=0 )
      {
        sum1 += specIn->dataY()[i];
        sum2 += exp( -((specIn->dataX()[i+1]-specIn->dataX()[i])*0.5+specIn->dataX()[i])/m_muLife);
      }
    }
    double N0 = sum1/sum2;

    for (int i=0; i<m_nData; i++)
    {
      if ( specIn->readX()[i]>=0 )
      {
        specOut->dataY()[i] = specIn->dataY()[i] - N0*exp(-((specIn->dataX()[i+1]-specIn->dataX()[i])*0.5+specIn->dataX()[i])/m_muLife);
        specOut->dataE()[i] = ( specIn->readY()[i] > m_poissonLim) ? specIn->readE()[i] : sqrt(N0*exp(-specIn->readX()[i]/m_muLife));
      }
    }
  } // Histogram loop

}

//----------------------------------------------------------------------------------------------
/** Remove exponential decay from input histograms, i.e., calculate asymmetry
* @param inputWs :: input workspace containing the spectra
* @param outputWs :: output workspace to hold temporary results
*/
void PhaseQuadMuon::loseExponentialDecay (API::MatrixWorkspace_sptr inputWs, API::MatrixWorkspace_sptr outputWs)
{
  for (size_t h=0; h<inputWs->getNumberHistograms(); h++)
  {
    auto specIn = inputWs->getSpectrum(h);
    auto specOut = outputWs->getSpectrum(h);

    specOut->dataX()=specIn->readX();

    for(int i=0; i<m_nData; i++)
    {
      if ( specIn->readX()[i]>=0 )
      {
        double usey = specIn->readY()[i];
        double oops = ( (usey<=0) || (specIn->readE()[i]>=m_bigNumber));
        specOut->dataY()[i] = oops ? 0 : log(usey);
        specOut->dataE()[i] = oops ? m_bigNumber : specIn->readE()[i]/usey;
      }
    }

    double s, sx, sy, sig;
    s = sx = sy =0;
    for (int i=0; i<m_nData; i++)
    {
      if ( specIn->readX()[i]>=0 )
      {
        sig = specOut->readE()[i]*specOut->readE()[i];
        s += 1./sig;
        sx+= specOut->readX()[i]/sig;
        sy+= specOut->readY()[i]/sig;
      }
    }
    double N0 = (sy+sx/m_muLife)/s;
    N0=exp(N0);

    for (int i=0; i<m_nData; i++)
    {
      if ( specIn->readX()[i]>=0 )
      {
        specOut->dataY()[i] = specIn->dataY()[i] - N0 *exp(-specIn->readX()[i]/m_muLife);
        specOut->dataE()[i] = ( specIn->readY()[i] > m_poissonLim) ? specIn->readE()[i] : sqrt(N0*exp(-specIn->readX()[i]/m_muLife));
      }
    }
  } // Histogram loop

}


//----------------------------------------------------------------------------------------------
/** Compute Squashograms
* @param tempWs :: input workspace containing the asymmetry in the lab frame
* @param outputWs :: output workspace to hold squashograms
*/
void PhaseQuadMuon::squash(API::MatrixWorkspace_sptr tempWs, API::MatrixWorkspace_sptr outputWs)
{

  double sxx=0;
  double syy=0;
  double sxy=0;

  for (size_t h=0; h<m_nHist; h++)
  {
    auto data = m_histData[h];
    double X = data.detOK * data.alpha * cos(data.phi);
    double Y = data.detOK * data.alpha * sin(data.phi);
    sxx += X*X;
    syy += Y*Y;
    sxy += X*Y;
  }

  double lam1 = 2 * syy / (sxx*syy - sxy*sxy);
  double mu1  = 2 * sxy / (sxy*sxy - sxx*syy);
  double lam2 = 2 * sxy / (sxy*sxy - sxx*syy);
  double mu2  = 2 * sxx / (sxx*syy - sxy*sxy);
  std::vector<double> aj, bj;

  for (size_t h=0; h<m_nHist; h++)
  {
    auto data = m_histData[h];
    double X = data.detOK * data.alpha * cos(data.phi);
    double Y = data.detOK * data.alpha * sin(data.phi);
    aj.push_back( (lam1 * X + mu1 * Y)*0.5 );
    bj.push_back( (lam2 * X + mu2 * Y)*0.5 );
  }

  std::vector<double> data1(m_nData,0), data2(m_nData,0);
  std::vector<double> sigm1(m_nData,0), sigm2(m_nData,0);
  for (size_t i=0; i<m_nData; i++)
  {
    for (size_t h=0; h<m_nHist; h++)
    {
      auto spec = tempWs->getSpectrum(h);
      data1[i] += aj[h] * spec->readY()[i];
      data2[i] += bj[h] * spec->readY()[i];
      sigm1[i] += aj[h]*aj[h] * spec->readE()[i] * spec->readE()[i];
      sigm2[i] += bj[h]*bj[h] * spec->readE()[i] * spec->readE()[i];
    }
    sigm1[i] = sqrt(sigm1[i]);
    sigm2[i] = sqrt(sigm2[i]);
  }

  outputWs->getSpectrum(0)->dataX() = tempWs->getSpectrum(0)->readX();
  outputWs->getSpectrum(0)->dataY() = data1;
  outputWs->getSpectrum(0)->dataE() = sigm1;
  outputWs->getSpectrum(1)->dataX() = tempWs->getSpectrum(1)->readX();
  outputWs->getSpectrum(1)->dataY() = data2;
  outputWs->getSpectrum(1)->dataE() = sigm2;

}


//----------------------------------------------------------------------------------------------
/** Put back in exponential decay
* @param outputWs :: output workspace with squashograms to update
*/
void PhaseQuadMuon::regainExponential(API::MatrixWorkspace_sptr outputWs)
{
  auto specRe = outputWs->getSpectrum(0);
  auto specIm = outputWs->getSpectrum(1);

  for (size_t i=0; i<m_nData; i++)
  {
    double x = outputWs->getSpectrum(0)->readX()[i];
    double xp= outputWs->getSpectrum(0)->readX()[i+1];
    double e = exp(-( (xp-x)*0.5+x )/m_muLife);
    specRe->dataY()[i] /= e;
    specIm->dataY()[i] /= e;
    specRe->dataE()[i] /= e;
    specIm->dataE()[i] /= e;
  }
}

}
}