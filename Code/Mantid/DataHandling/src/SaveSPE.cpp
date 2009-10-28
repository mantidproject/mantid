//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/SaveSPE.h"
#include "MantidKernel/FileValidator.h"
#include "MantidAPI/WorkspaceValidators.h"

#include <fstream>
#include <iomanip>

using namespace Mantid::DataHandling;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveSPE)

//---------------------------------------------------
// Private member functions
//---------------------------------------------------
/**
 * Initialise the algorithm
 */
void SaveSPE::init()
{
  // Data must be in Energy Transfer and common bins
  API::CompositeValidator<> *wsValidator = new API::CompositeValidator<>;
  wsValidator->add(new API::WorkspaceUnitValidator<>("DeltaE"));
  wsValidator->add(new API::CommonBinsValidator<>);
  declareProperty(new API::WorkspaceProperty<>("InputWorkspace", "", Kernel::Direction::Input,wsValidator),
      "The input workspace, which must be in Energy Transfer");

  declareProperty("Filename", "", new Mantid::Kernel::FileValidator(std::vector<std::string>(), false),
      "The filename to use for the saved data", Kernel::Direction::Input);
}

/**
 * Execute the algorithm
 */
void SaveSPE::exec()
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
  
  const int nHist = inputWS->getNumberHistograms();
  const int nBins = inputWS->blocksize();

  // Retrieve the filename from the properties
  const std::string filename = getProperty("Filename");

  std::ofstream outSPE_File(filename.c_str());
  if (!outSPE_File)
  {
    g_log.error("Failed to open file:" + filename);
    throw Kernel::Exception::FileError("Failed to open file:" , filename);
  }

  // Number of Workspaces and Number of Energy Bins
  outSPE_File << std::fixed << std::setw(8) << nHist;
  outSPE_File << std::fixed << std::setw(8) << nBins;
  outSPE_File << std::endl;

  // Write the dummy angle grid
  outSPE_File << "### Phi Grid" << std::endl;

  for (int i = 0; i < nHist+1; i++)
  {
    double value = (i + 1) * 0.5;
    outSPE_File << std::fixed << std::scientific << std::setprecision(3) << std::setw(10) << value;
    if ( (i > 0) && ((i + 1) % 8 == 0) )
    {
        outSPE_File << std::endl;
    }
  }

  // If the number of angles isn't a factor of 8 then we need to add an extra CR/LF
  if (nHist % 8 != 0)
    outSPE_File << std::endl;

  // Get the Energy Axis (X) of the first spectra (they are all the same - checked above)
  const MantidVec& X = inputWS->readX(0);

  // Write the energy grid
  outSPE_File << "### Energy Grid" << std::endl;
  for (MantidVec::size_type i = 0; i < X.size(); i++) // needs to be better!
  {
    outSPE_File << std::fixed << std::scientific << std::setprecision(3) << std::setw(10) << X[i];
    if ( (i > 0) && ((i + 1) % 8 == 0) )
    {
        outSPE_File << std::endl;
    }
  }

  // If the number of energies isn't a factor of 8 then we need to add an extra CR/LF
  if (nBins % 8 != 0)
    outSPE_File << std::endl;

  for (int i = 0; i < nHist; i++)
  {
    const MantidVec& Y = inputWS->readY(i);
    const MantidVec& E = inputWS->readE(i);

    outSPE_File << "### S(Phi,w)" << std::endl;
    for (int j = 0; j < nBins; j++)
    {
      outSPE_File << std::fixed << std::scientific << std::setprecision(3) << std::setw(10) << Y[j];
      if ( (j > 0) && ((j + 1) % 8 == 0) )
      {
        outSPE_File << std::endl;
      }
    }
    // If the number of points isn't a factor of 8 then we need to add an extra CR/LF
    if (nBins % 8 != 0)
      outSPE_File << std::endl;

    outSPE_File << "### Errors" << std::endl;
    for (int j = 0; j < nBins; j++)
    {
      outSPE_File << std::fixed << std::scientific << std::setprecision(3) << std::setw(10) << E[j];
      if ( (j > 0) && ((j + 1) % 8 == 0) )
      {
        outSPE_File << std::endl;
      }
    }
    // If the number of points isn't a factor of 8 then we need to add an extra CR/LF
    if (nBins % 8 != 0)
      outSPE_File << std::endl;
  }
}
