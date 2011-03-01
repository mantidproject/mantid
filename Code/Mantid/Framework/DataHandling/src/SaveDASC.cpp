#include "MantidDataHandling/SaveDASC.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/WorkspaceValidators.h"
#include <boost/shared_ptr.hpp>
#include <cmath>
#include <fstream>

namespace Mantid
{
namespace DataHandling
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveDASC)
using namespace Kernel;
using namespace API;
//---------------------------------------------------
// Private member functions
//---------------------------------------------------
/**
 * Initialise the algorithm
 */
void SaveDASC::init()
{
  //this->setWikiSummary("Exports the data in a [[workspace]] into an ASCII file that is readable by the application DAVE");
  //this->setOptionalMessage("Exports the data in a workspace into an ASCII file that is readable by the application DAVE");

  API::CompositeValidator<> *wsValidator = new API::CompositeValidator<>;
  // Data must havec common bins
  wsValidator->add(new API::CommonBinsValidator<>);
  // the output of this algorithm is spectrum data, not histogram, but as a histogram to spectrum conversion is built in a spectrum as input would be no good, at the moment
  wsValidator->add(new API::HistogramValidator<>);
  declareProperty(new API::WorkspaceProperty<>("InputWorkspace", "", Kernel::Direction::Input,wsValidator),
      "The input workspace");
  declareProperty(new FileProperty("Filename","",FileProperty::Save),
    "The filename to use for the saved data");
}

/**
 * Execute the algorithm
 * throw invalid_argument if the workspace is found not to have common X bins boundaries for all spectra
 * throw FileError if it was not possible to open the output file for writing
 */
void SaveDASC::exec()
{
  using namespace Mantid::API;
  // Retrieve the input workspace
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  
  // Do the full check for common binning
  if ( ! WorkspaceHelpers::commonBoundaries(inputWS) )
  {
    g_log.error("The input workspace must have common binning");
    throw std::invalid_argument("The input workspace must have common binning");
  }
  
  // Retrieve the filename from the properties
  const std::string filename = getProperty("Filename");

  std::ofstream outDASC_File(filename.c_str());
  if (!outDASC_File)
  {
    g_log.error("Failed to open file:" + filename);
    throw Kernel::Exception::FileError("Failed to open file:" , filename);
  }
  // look out for user cancel and move the progress bar forward a small, the fraction below seemed about right from trial and error
  double fracComp = 0.03;
  progress(fracComp);
  interruption_point();

  // information about the number of bins and histograms and their units
  writeHeader(inputWS, outDASC_File);

  // now write the numbers of counts, the bulk of the data
  outDASC_File << "# Group" << std::endl;

  // prepare to loop through all the values in all the spectra, preparing to update the progress bar 100 times
  int progInt = static_cast<int>(ceil(inputWS->getNumberHistograms()/100.0));
  double progInc = (1-fracComp)/100.0;

  const int nBins = inputWS->blocksize();
  for (int i = 0; i < inputWS->getNumberHistograms(); i++)
  {
    const MantidVec& Y = inputWS->readY(i);
    const MantidVec& E = inputWS->readE(i);
    for (int j = 0; j < nBins; j++)
    {
      outDASC_File << Y[j] << " " << E[j] << std::endl;
    }

    if ( i % progInt == 1 )
    {// update the progress bar and deal with user cancels
      progress( fracComp += progInc );
      interruption_point();
    }
  }
}

/** The header contains what the units of the X-values (axis(0)) and
*  the values, at the centre of the bin, the axis(1) and its values
*  @param WS :: the input workspace
*  @param output :: file stream to write to
*/
void SaveDASC::writeHeader(API::MatrixWorkspace_const_sptr WS, std::ofstream &output)
{// Write to the header what the units of the X-values are
  // an axis doesn't have to have unit so we need to check
  Kernel::Unit_sptr& currentUnit = WS->getAxis(0)->unit();
  std::string XunitDist;
  if (currentUnit.get())
  {
    XunitDist = currentUnit->unitID();
  }
  else
  {
    XunitDist = "X-values (units unknown)";
    g_log.warning() << "No unit set for the workspace's X-values" << std::endl;
  }

  output << "#Number of " << XunitDist << " points (x)" << std::endl;
  output << WS->blocksize() << std::endl;
  
  // now write the Y units to the header
  currentUnit = WS->getAxis(1)->unit();
  std::string YunitDist;
  if (currentUnit->unitID() != "Empty")
  {
    YunitDist = currentUnit->unitID();
    output << "#Number of " << YunitDist << " points (y)" << std::endl;
  }
  else
  {
    YunitDist = "spectra";
    output << "#Number of " << YunitDist << " numbers (y)" << std::endl;
    g_log.warning() << "No unit set for the workspace's axis number 1, recording the DAVE y unit as spectra number" << std::endl;
  }
  output << WS->getNumberHistograms() << std::endl;

  // now the X-values (specifically values at the centre of bins)
  output << "# " << XunitDist << " values (x)" << std::endl;
  for (int i = 0; i < WS->blocksize(); i++)
  {// the centre of the bin = mean of the bin boundaries
    output << (WS->readX(0)[i] +  WS->readX(0)[i+1])/2 << std::endl;
  }

  // the Y-values coordinate values, these are NOT number of counts
  output << "# " << YunitDist << " values (y)" << std::endl;
  for (int i = 0; i < WS->getNumberHistograms(); i++)
  {
    Axis *YValues = WS->getAxis(1);
    output << (*YValues)(i) << std::endl;
  }

}

}
}
