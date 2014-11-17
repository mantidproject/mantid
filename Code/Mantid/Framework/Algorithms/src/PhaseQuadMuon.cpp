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

  declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace","", Direction::Output), 
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

  // Create output workspace with two spectra
  API::MatrixWorkspace_sptr outputWs = getProperty("OutputWorkspace");
  outputWs = boost::dynamic_pointer_cast<API::MatrixWorkspace>(
    API::WorkspaceFactory::Instance().create("Workspace2D", inputWs->getNumberHistograms(), inputWs->getSpectrum(0)->readX().size(), inputWs->getSpectrum(0)->readY().size())); // TODO change number histograms
  outputWs->getAxis(0)->unit() = inputWs->getAxis(0)->unit();

  removeExponentialDecay(inputWs,outputWs);
  //squash (inputWs, outputWs);
  
  setProperty("OutputWorkspace", outputWs);
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
/** Remove exponential decay from input histograms
* @param inputWs :: input workspace containing the spectra
* @param outputWs :: output workspace to hold temporary results
*/
void PhaseQuadMuon::removeExponentialDecay (API::MatrixWorkspace_sptr inputWs, API::MatrixWorkspace_sptr outputWs)
{

  for (size_t h=0; h<inputWs->getNumberHistograms(); h++)
  {
    auto specIn = inputWs->getSpectrum(h);
    auto specOut = outputWs->getSpectrum(h);

    for (int i=0; i<m_nData; i++)
    {
        double usey = specIn->readY()[i];
        double oops = ( usey<= 0 ) || ( specIn->readE()[i] > m_bigNumber );
        specOut->dataY()[i] = oops ? 0 : log(usey);
        specOut->dataE()[i] = oops ? m_bigNumber : specIn->readE()[i] / usey;
        std::cout << 
    }
    double s=0;
    double sx=0;
    double sy=0;

    for (int i=0; i<m_nData; i++)
    {
      double sigma = specOut->dataE()[i] * specOut->dataE()[i];
      s+= 1./sigma;
      sx+= specOut->dataX()[i]/sigma;
      sy+= specOut->dataY()[i]/sigma;
    }
    double N0 = (sy+sx/m_muLife/1E9)/s;
//    std::cout << specIn->readY()[40] << " " << sy << " " << sx << " " << s << " " << N0 << std::endl;
  }

}


void PhaseQuadMuon::squash(API::MatrixWorkspace_sptr inputWs, API::MatrixWorkspace_sptr outputWs)
{

  if ( m_nHist != inputWs->getNumberHistograms() )
  {
    throw std::runtime_error("InputWorkspace and PhaseTable do not have the same number of spectra");
  }
  else
  {
    // TODO
  }

}


}
}