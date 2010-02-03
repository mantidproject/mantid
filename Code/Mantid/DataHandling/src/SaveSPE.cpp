//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/SaveSPE.h"
#include "MantidKernel/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include <cstdio>

namespace Mantid
{
namespace DataHandling
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveSPE)

using namespace Kernel;

///@cond
const char NUM_FORM[] = "%10.3E";
const char NUMS_FORM[] = "%10.3E%10.3E%10.3E%10.3E%10.3E%10.3E%10.3E%10.3E\n";
///@endcond

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
  wsValidator->add(new API::HistogramValidator<>);
  declareProperty(new API::WorkspaceProperty<>("InputWorkspace", "", Direction::Input,wsValidator),
    "The input workspace, which must be in Energy Transfer");
  declareProperty(new FileProperty("Filename","", FileProperty::Save),
    "The filename to use for the saved data");
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

  FILE * outSPE_File;
  outSPE_File = fopen(filename.c_str(),"w");
  if (!outSPE_File)
  {
    g_log.error("Failed to open file:" + filename);
    throw Kernel::Exception::FileError("Failed to open file:" , filename);
  }

  // Number of Workspaces and Number of Energy Bins
  fprintf(outSPE_File,"%8u%8u\n",nHist,nBins);

  // Write the dummy angle grid
  fprintf(outSPE_File,"### Phi Grid\n");
  const int phiPoints = nHist + 1; // Pretend this is binned
  for (int i = 0; i < phiPoints; i++)
  {
    const double value = i + 0.5;
    fprintf(outSPE_File,NUM_FORM,value);
    if ( (i > 0) && ((i + 1) % 8 == 0) )
    {
      fprintf(outSPE_File,"\n");
    }
  }

  // If the number of angles isn't a factor of 8 then we need to add an extra CR/LF
  if (nHist % 8 != 0) fprintf(outSPE_File,"\n");

  // Get the Energy Axis (X) of the first spectra (they are all the same - checked above)
  const MantidVec& X = inputWS->readX(0);

  // Write the energy grid
  fprintf(outSPE_File,"### Energy Grid\n");
  const int energyPoints = nBins + 1; // Validator enforces binned data
  int i = 7;
  for (; i < energyPoints; i+=8)
  {// output a whole line of numbers at once
    fprintf(outSPE_File,NUMS_FORM,
            X[i-7],X[i-6],X[i-5],X[i-4],X[i-3],X[i-2],X[i-1],X[i]);
  }
  // if the last line is not a full line enter them individually
  if ( i != energyPoints-1 )
  {// the condition above means that the last line has fewer characters than the usual
    for (i-=7; i < energyPoints; ++i)
    {
      fprintf(outSPE_File,NUM_FORM,X[i]);
    }
    fprintf(outSPE_File,"\n");
  }

  // We write out values 8 at a time, so will need to do extra work if nBins isn't a factor of 8
  const int remainder = nBins % 8;

  // Create a progress reporting object
  Progress progress(this,0,1,nHist);
  // Loop over the spectra, writing out Y and then E values for each
  for (int i = 0; i < nHist; i++)
  {
    const MantidVec& Y = inputWS->readY(i);
    const MantidVec& E = inputWS->readE(i);

    fprintf(outSPE_File,"### S(Phi,w)\n");
    for (int j = 7; j < nBins; j+=8)
    {// output a whole line of numbers at once
      fprintf(outSPE_File,NUMS_FORM,
                          Y[j-7],Y[j-6],Y[j-5],Y[j-4],Y[j-3],Y[j-2],Y[j-1],Y[j]);
    }
    if ( remainder )
    {
      for ( int l = nBins - remainder; l < nBins; ++l)
      {
        fprintf(outSPE_File,NUM_FORM,Y[l]);
      }
      fprintf(outSPE_File,"\n");
    }

    fprintf(outSPE_File,"### Errors\n");
    for ( int k = 7; k < nBins; k+=8)
    {// output a whole line of numbers at once
      fprintf(outSPE_File,NUMS_FORM,
                          E[k-7],E[k-6],E[k-5],E[k-4],E[k-3],E[k-2],E[k-1],E[k]);
    }
    if ( remainder )
    {
      for ( int l = nBins - remainder; l < nBins; ++l)
      {
        fprintf(outSPE_File,NUM_FORM,E[l]);
      }
      fprintf(outSPE_File,"\n");
    }

    progress.report();
  }

  // Close the file
  fclose(outSPE_File);
}

}
}
