//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/LoadSPE.h"
#include "MantidDataHandling/SaveSPE.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidDataObjects/Histogram1D.h"
#include "MantidKernel/UnitFactory.h"
#include <cstdio>
#include <limits>
#include <fstream>

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace API;

DECLARE_FILELOADER_ALGORITHM(LoadSPE);

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadSPE::confidence(Kernel::FileDescriptor &descriptor) const {
  if (!descriptor.isAscii())
    return 0;

  auto &file = descriptor.data();

  std::string fileline;
  // First line - expected to be a line 2 columns which is  histogram  & bin
  // numbers
  std::getline(file, fileline);
  std::istringstream is(fileline);
  unsigned int dummy(0);
  is >> dummy >> dummy;
  if (is.fail()) {
    return 0; // Couldn't read 2 numbers so fail
  }
  // Trying to read another should produce eof
  is >> dummy;
  if (!is.eof())
    return 0;

  // Next line should be comment line: "### Phi Grid" or "### Q Grid"
  std::getline(file, fileline);
  if (fileline.find("Phi Grid") != std::string::npos ||
      fileline.find("Q Grid") != std::string::npos) {
    return 80;
  } else
    return 0;
}

//---------------------------------------------------
// Private member functions
//---------------------------------------------------

/**
 * Initialise the algorithm
 */
void LoadSPE::init() {
  declareProperty(new FileProperty("Filename", "", FileProperty::Load, ".spe"),
                  "The name of the SPE file to load.");
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "The name to use for the output workspace");
}

/**
 * Execute the algorithm
 */
void LoadSPE::exec() {
  // Retrieve filename and try to open the file
  m_filename = getPropertyValue("Filename");

  FILE *speFile;
  speFile = fopen(m_filename.c_str(), "r");
  if (!speFile) {
    g_log.error("Failed to open file: " + m_filename);
    throw Exception::FileError("Failed to open file: ", m_filename);
  }

  // The first two numbers are the number of histograms and the number of bins
  size_t nhist = 0, nbins = 0;
  unsigned int nhistTemp = 0, nbinsTemp = 0;
  int retval = fscanf(speFile, "%8u%8u\n", &nhistTemp, &nbinsTemp);
  if (retval != 2)
    reportFormatError("Header line");
  // Cast from temp values to size_t values
  nhist = static_cast<size_t>(nhistTemp);
  nbins = static_cast<size_t>(nbinsTemp);

  // Next line should be comment line: "### Phi Grid" or "### Q Grid"
  char comment[100];
  fgets(comment, 100, speFile);
  if (comment[0] != '#')
    reportFormatError(std::string(comment));

  // Create the axis that will hold the phi values
  auto *phiAxis = new BinEdgeAxis(nhist + 1);
  // Look at previously read comment field to see what unit vertical axis should
  // have
  if (comment[4] == 'Q' || comment[4] == 'q') {
    phiAxis->unit() = UnitFactory::Instance().create("MomentumTransfer");
  } else {
    phiAxis->unit() = boost::shared_ptr<Unit>(new Units::Phi);
  }

  // Read in phi grid
  for (size_t i = 0; i <= nhist; ++i) {
    double phi;
    retval = fscanf(speFile, "%10le", &phi);
    if (retval != 1) {
      std::stringstream ss;
      ss << "Reading phi value" << i;
      reportFormatError(ss.str());
    }
    phiAxis->setValue(i, phi);
  }
  // Read to EOL
  fgets(comment, 100, speFile);

  // Next line should be comment line: "### Energy Grid"
  fgets(comment, 100, speFile);
  if (comment[0] != '#')
    reportFormatError(std::string(comment));

  // Now the X bin boundaries
  MantidVecPtr XValues;
  MantidVec &X = XValues.access();
  X.resize(nbins + 1);

  for (size_t i = 0; i <= nbins; ++i) {
    retval = fscanf(speFile, "%10le", &X[i]);
    if (retval != 1) {
      std::stringstream ss;
      ss << "Reading energy value" << i;
      reportFormatError(ss.str());
    }
  }
  // Read to EOL
  fgets(comment, 100, speFile);

  // Now create the output workspace
  MatrixWorkspace_sptr workspace = WorkspaceFactory::Instance().create(
      "Workspace2D", nhist, nbins + 1, nbins);
  workspace->getAxis(0)->unit() = UnitFactory::Instance().create("DeltaE");
  workspace->isDistribution(true); // It should be a distribution
  workspace->setYUnitLabel("S(Phi,Energy)");
  // Replace the default spectrum axis with the phi values one
  workspace->replaceAxis(1, phiAxis);

  // Now read in the data spectrum-by-spectrum
  Progress progress(this, 0, 1, nhist);
  for (size_t j = 0; j < nhist; ++j) {
    // Set the common X vector
    workspace->setX(j, XValues);
    // Read in the Y & E data
    readHistogram(speFile, workspace, j);

    progress.report();
  }

  // Close the file
  fclose(speFile);

  // Set the output workspace property
  setProperty("OutputWorkspace", workspace);
}

/** Reads in the data corresponding to a single spectrum
 *  @param speFile ::   The file handle
 *  @param workspace :: The output workspace
 *  @param index ::     The index of the current spectrum
 */
void LoadSPE::readHistogram(FILE *speFile, API::MatrixWorkspace_sptr workspace,
                            size_t index) {
  // First, there should be a comment line
  char comment[100];
  fgets(comment, 100, speFile);
  if (comment[0] != '#')
    reportFormatError(std::string(comment));

  // Then it's the Y values
  MantidVec &Y = workspace->dataY(index);
  const size_t nbins = workspace->blocksize();
  int retval;
  for (size_t i = 0; i < nbins; ++i) {
    retval = fscanf(speFile, "%10le", &Y[i]);
    // g_log.error() << Y[i] << std::endl;
    if (retval != 1) {
      std::stringstream ss;
      ss << "Reading data value" << i << " of histogram " << index;
      reportFormatError(ss.str());
    }
    // -10^30 is the flag for not a number used in SPE files (from
    // www.mantidproject.org/images/3/3d/Spe_file_format.pdf)
    if (Y[i] == SaveSPE::MASK_FLAG) {
      Y[i] = std::numeric_limits<double>::quiet_NaN();
    }
  }
  // Read to EOL
  fgets(comment, 100, speFile);

  // Another comment line
  fgets(comment, 100, speFile);
  if (comment[0] != '#')
    reportFormatError(std::string(comment));

  // And then the error values
  MantidVec &E = workspace->dataE(index);
  for (size_t i = 0; i < nbins; ++i) {
    retval = fscanf(speFile, "%10le", &E[i]);
    if (retval != 1) {
      std::stringstream ss;
      ss << "Reading error value" << i << " of histogram " << index;
      reportFormatError(ss.str());
    }
  }
  // Read to EOL
  fgets(comment, 100, speFile);

  return;
}

/** Called if the file is not formatted as expected
 *  @param what :: A string describing where the problem occurred
 *  @throw Mantid::Kernel::Exception::FileError terminating the algorithm
 */
void LoadSPE::reportFormatError(const std::string &what) {
  g_log.error("Unexpected formatting in file " + m_filename + " : " + what);
  throw Exception::FileError("Unexpected formatting in file: ", m_filename);
}

} // namespace DataHandling
} // namespace Mantid
