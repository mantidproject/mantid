//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/ArrayProperty.h"
#include "MantidAlgorithms/PhaseQuadMuon.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IFileLoader.h"

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

  declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace1","", Direction::Output), 
    "Name of the output workspace" );

  declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace2","", Direction::Output), 
    "Name of the output workspace" );

  declareProperty(new API::FileProperty("PhaseTable", "", API::FileProperty::Load, ".INF"),
    "The name of the list of phases for each spectrum");
}

/** Executes the algorithm
 *
 */
void PhaseQuadMuon::exec()
{
	// TODO

	// Get input workspace
  API::MatrixWorkspace_sptr inputWs = getProperty("InputWorkspace");

  // Get phase table
  std::string filename = getPropertyValue("PhaseTable");
  loadPhaseTable ( filename );

  m_nData = inputWs->getSpectrum(0)->readY().size();
  m_nHist = inputWs->getNumberHistograms();

  // Create temporary workspace to perform operations on it
  API::MatrixWorkspace_sptr tempWs1 = boost::dynamic_pointer_cast<API::MatrixWorkspace>(
    API::WorkspaceFactory::Instance().create("Workspace2D", inputWs->getNumberHistograms(), inputWs->getSpectrum(0)->readX().size(), inputWs->getSpectrum(0)->readY().size()));
  // Create temporary workspace to perform operations on it
  API::MatrixWorkspace_sptr tempWs2 = boost::dynamic_pointer_cast<API::MatrixWorkspace>(
    API::WorkspaceFactory::Instance().create("Workspace2D", inputWs->getNumberHistograms(), inputWs->getSpectrum(0)->readX().size(), inputWs->getSpectrum(0)->readY().size()));

  // Create output workspace with two spectra
  API::MatrixWorkspace_sptr outputWs = getProperty("OutputWorkspace");
  outputWs = boost::dynamic_pointer_cast<API::MatrixWorkspace>(
    API::WorkspaceFactory::Instance().create("Workspace2D", 2, inputWs->getSpectrum(0)->readX().size(), inputWs->getSpectrum(0)->readY().size()));
  outputWs->getAxis(0)->unit() = inputWs->getAxis(0)->unit();

  normaliseAlphas(m_histData);
  removeExponentialDecay(inputWs,tempWs1);
  loseExponentialDecay(inputWs,tempWs2);
  squash (tempWs2, outputWs);
  
  setProperty("OutputWorkspace", outputWs); // TODO: change this
  setProperty("OutputWorkspace1", tempWs1); // TODO: change this
  setProperty("OutputWorkspace2", tempWs2); // TODO: change this
}

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
  for (size_t h=0; h<m_nHist; h++)
  {
    histData[h].alpha /= max;
  }
}

//----------------------------------------------------------------------------------------------
/** Load PhaseTable file to a vector of HistData.
* @param filename :: phase table .inf filename
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

      int cont=0;
      while( !input.eof() )
      {
        // Read histogram data
        HistData tempData;
        input >> tempData.n0 >> tempData.alpha >> tempData.phi >> tempData.lag >> tempData.dead >> tempData.deadm;
        m_histData.push_back (tempData);
        cont++;
      }

      if ( cont!= m_nHist )
      {
        throw Exception::FileError("PhaseQuad: File was not in expected format", filename); // TODO: specify that number of lines is wrong
      }

      if ( cont<3 )
      {
        throw std::runtime_error("PhaseQuad: Found less than 4 histograms");
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
/** Remove exponential decay from input histograms, i.e., calculate asymmetry
* @param inputWs :: input workspace containing the spectra
* @param outputWs :: output workspace to hold temporary results
*/
void PhaseQuadMuon::removeExponentialDecay (API::MatrixWorkspace_sptr inputWs, API::MatrixWorkspace_sptr outputWs)
{

  if ( m_nHist != inputWs->getNumberHistograms() )
  {
    throw std::runtime_error("InputWorkspace and PhaseTable do not have the same number of spectra");
  }
  else
  {

    // Muon decay: N(t) = N0 exp(-t/tau) [ 1 + a P(t) ]
    // N(t) - N0 exp(-t/tau) = 1 + a P(t)
    // N0?
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

  }// else

}

//----------------------------------------------------------------------------------------------
/** Remove exponential decay from input histograms, i.e., calculate asymmetry
* @param inputWs :: input workspace containing the spectra
* @param outputWs :: output workspace to hold temporary results
*/
void PhaseQuadMuon::loseExponentialDecay (API::MatrixWorkspace_sptr inputWs, API::MatrixWorkspace_sptr outputWs)
{

  if ( m_nHist != inputWs->getNumberHistograms() )
  {
    throw std::runtime_error("InputWorkspace and PhaseTable do not have the same number of spectra");
  }
  else
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

  }// else

}


void PhaseQuadMuon::squash(API::MatrixWorkspace_sptr tempWs, API::MatrixWorkspace_sptr outputWs)
{

  double sxx=0;
  double syy=0;
  double sxy=0;

  for (size_t h=0; h<tempWs->getNumberHistograms(); h++)
  {
    auto data = m_histData[h];
    double X = data.n0 * data.alpha * cos(data.phi);
    double Y = data.n0 * data.alpha * sin(data.phi);
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
    double X = data.n0 * data.alpha * cos(data.phi);
    double Y = data.n0 * data.alpha * sin(data.phi);
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

  for (size_t i=0; i<m_nData; i++)
  {
    double x = tempWs->getSpectrum(0)->readX()[i];
    double xp= tempWs->getSpectrum(0)->readX()[i+1];
    double e = exp(-( (xp-x)*0.5+x )/m_muLife);
    data1[i] /= e;
    data2[i] /= e;
    sigm1[i] /= e;
    sigm2[i] /= e;
  }

  outputWs->getSpectrum(0)->dataX() = tempWs->getSpectrum(0)->readX();
  outputWs->getSpectrum(0)->dataY() = data1;
  outputWs->getSpectrum(0)->dataE() = sigm1;
  outputWs->getSpectrum(1)->dataX() = tempWs->getSpectrum(1)->readX();
  outputWs->getSpectrum(1)->dataY() = data2;
  outputWs->getSpectrum(1)->dataE() = sigm2;

}


}
}