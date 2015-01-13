//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/SaveSPE.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "Poco/File.h"
#include <cstdio>
#include <cmath>
#include <boost/math/special_functions/fpclassify.hpp>
#include <stdexcept>

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveSPE)

/** A Macro wrapping std::fprintf in order to throw an exception when there is a
* fault writing to disk
*  @param stream :: the file object to write to
*  @param format :: C string that contains the text to be written to the stream.
*  @param ... :: Additional arguments to fill format specifiers
*  @throws std::runtime_error :: throws when there is a problem writing to disk,
* usually disk space or permissions based
*/
#define FPRINTF_WITH_EXCEPTION(stream, format, ...)                            \
  if (fprintf(stream, format, ##__VA_ARGS__) <= 0) {                           \
    throw std::runtime_error(                                                  \
        "Error writing to file. Check folder permissions and disk space.");    \
  }

using namespace Kernel;
using namespace API;

///@cond
const char NUM_FORM[] = "%-10.4G";
const char NUMS_FORM[] =
    "%-10.4G%-10.4G%-10.4G%-10.4G%-10.4G%-10.4G%-10.4G%-10.4G\n";
static const char Y_HEADER[] = "### S(Phi,w)\n";
static const char E_HEADER[] = "### Errors\n";
///@endcond

/// set to the number of numbers on each line (the length of lines is hard-coded
/// in other parts of the code too)
static const unsigned int NUM_PER_LINE = 8;

const double SaveSPE::MASK_FLAG = -1e30;
const double SaveSPE::MASK_ERROR = 0.0;

SaveSPE::SaveSPE() : API::Algorithm(), m_remainder(-1), m_nBins(0) {}

//---------------------------------------------------
// Private member functions
//---------------------------------------------------
/**
* Initialize the algorithm
*/
void SaveSPE::init() {
  // Data must be in Energy Transfer and common bins
  auto wsValidator = boost::make_shared<Kernel::CompositeValidator>();
  wsValidator->add<API::CommonBinsValidator>();
  wsValidator->add<API::HistogramValidator>();
  declareProperty(new API::WorkspaceProperty<>("InputWorkspace", "",
                                               Direction::Input, wsValidator),
                  "The input workspace, which must be in Energy Transfer");
  declareProperty(new FileProperty("Filename", "", FileProperty::Save, ".spe"),
                  "The filename to use for the saved data");
}

/**
* Execute the algorithm
*/
void SaveSPE::exec() {
  using namespace Mantid::API;
  // Retrieve the input workspace
  const MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

  // Do the full check for common binning
  if (!WorkspaceHelpers::commonBoundaries(inputWS)) {
    throw std::invalid_argument("The input workspace must have common binning");
  }

  // Retrieve the filename from the properties
  const std::string filename = getProperty("Filename");

  FILE *outSPEFile = fopen(filename.c_str(), "w");
  if (!outSPEFile) {
    throw Kernel::Exception::FileError("Failed to open file:", filename);
  }
  try {
    // write to the file being ready to catch it if something happens during
    // writing
    writeSPEFile(outSPEFile, inputWS);
    fclose(outSPEFile);
  } catch (std::exception &) {
    fclose(outSPEFile);
    Poco::File(filename).remove();
    // throw the exception again so the base class can deal with it too in it's
    // own way
    throw;
  }
}

/** Write the data to the SPE file
*  @param outSPEFile :: the file object to write to
*  @param inputWS :: the workspace to be saved
*/
void SaveSPE::writeSPEFile(FILE *outSPEFile,
                           const API::MatrixWorkspace_const_sptr &inputWS) {
  const size_t nHist = inputWS->getNumberHistograms();
  m_nBins = inputWS->blocksize();
  // Number of Workspaces and Number of Energy Bins
  FPRINTF_WITH_EXCEPTION(outSPEFile, "%8u%8u\n",
                         static_cast<unsigned int>(nHist),
                         static_cast<unsigned int>(m_nBins));
  // Write the angle grid (dummy if no 'vertical' axis)
  size_t phiPoints(0);
  if (inputWS->axes() > 1 && inputWS->getAxis(1)->isNumeric()) {
    const Axis &axis = *inputWS->getAxis(1);
    const std::string commentLine = "### " + axis.unit()->caption() + " Grid\n";
    FPRINTF_WITH_EXCEPTION(outSPEFile, "%s", commentLine.c_str());
    const size_t axisLength = axis.length();
    phiPoints = (axisLength == nHist) ? axisLength + 1 : axisLength;
    for (size_t i = 0; i < phiPoints; i++) {
      const double value =
          (i < axisLength) ? axis(i) : axis(axisLength - 1) + 1;
      FPRINTF_WITH_EXCEPTION(outSPEFile, NUM_FORM, value);
      if ((i + 1) % 8 == 0) {
        FPRINTF_WITH_EXCEPTION(outSPEFile, "\n");
      }
    }
  } else {
    FPRINTF_WITH_EXCEPTION(outSPEFile, "### Phi Grid\n");
    phiPoints = nHist + 1; // Pretend this is binned
    for (size_t i = 0; i < phiPoints; i++) {
      const double value = static_cast<int>(i) + 0.5;
      FPRINTF_WITH_EXCEPTION(outSPEFile, NUM_FORM, value);
      if ((i + 1) % 8 == 0) {
        FPRINTF_WITH_EXCEPTION(outSPEFile, "\n");
      }
    }
  }

  // If the number of points written isn't a factor of 8 then we need to add an
  // extra newline
  if (phiPoints % 8 != 0) {
    FPRINTF_WITH_EXCEPTION(outSPEFile, "\n");
  }

  // Get the Energy Axis (X) of the first spectra (they are all the same -
  // checked above)
  const MantidVec &X = inputWS->readX(0);

  // Write the energy grid
  FPRINTF_WITH_EXCEPTION(outSPEFile, "### Energy Grid\n");
  const size_t energyPoints = m_nBins + 1; // Validator enforces binned data
  size_t i = NUM_PER_LINE - 1;
  for (; i < energyPoints;
       i += NUM_PER_LINE) { // output a whole line of numbers at once
    FPRINTF_WITH_EXCEPTION(outSPEFile, NUMS_FORM, X[i - 7], X[i - 6], X[i - 5],
                           X[i - 4], X[i - 3], X[i - 2], X[i - 1], X[i]);
  }
  // if the last line is not a full line enter them individually
  if (energyPoints % NUM_PER_LINE != 0) { // the condition above means that the
                                          // last line has less than the maximum
                                          // number of digits
    for (i -= 7; i < energyPoints; ++i) {
      FPRINTF_WITH_EXCEPTION(outSPEFile, NUM_FORM, X[i]);
    }
    FPRINTF_WITH_EXCEPTION(outSPEFile, "\n");
  }

  writeHists(inputWS, outSPEFile);
}

/** Write the bin values and errors for all histograms to the file
*  @param WS :: the workspace to be saved
*  @param outFile :: the file object to write to
*/
void SaveSPE::writeHists(const API::MatrixWorkspace_const_sptr WS,
                         FILE *const outFile) {
  // We write out values NUM_PER_LINE at a time, so will need to do extra work
  // if nBins isn't a factor of NUM_PER_LINE
  m_remainder = m_nBins % NUM_PER_LINE;
  bool isNumericAxis = WS->getAxis(1)->isNumeric();
  const size_t nHist = WS->getNumberHistograms();
  // Create a progress reporting object
  Progress progress(this, 0, 1, 100);
  const int progStep = static_cast<int>(ceil(static_cast<int>(nHist) / 100.0));

  // there are very often spectra that are missing detectors, as this can be a
  // lot of detectors log it once at the end
  std::vector<int> spuriousSpectra;
  // used only for debugging
  int nMasked = 0;
  // Loop over the spectra, writing out Y and then E values for each
  for (int i = 0; i < static_cast<int>(nHist); i++) {
    try { // need to check if _all_ the detectors for the spectrum are masked,
          // as we don't have output values for those
      if (isNumericAxis || !WS->getDetector(i)->isMasked()) {
        // there's no masking, write the data
        writeHist(WS, outFile, i);
      } else { // all the detectors are masked, write the masking value from the
               // SPE spec
               // http://www.mantidproject.org/images/3/3d/Spe_file_format.pdf
        writeMaskFlags(outFile);
        nMasked++;
      }
    } catch (Exception::NotFoundError &) { // WS->getDetector(i) throws this if
                                           // the detector isn't in the
                                           // instrument definition file, write
                                           // mask values and prepare to log
                                           // what happened
      spuriousSpectra.push_back(i);
      writeMaskFlags(outFile);
    }
    // make regular progress reports and check for canceling the algorithm
    if (i % progStep == 0) {
      progress.report();
    }
  }
  logMissingMasked(spuriousSpectra, nHist - nMasked, nMasked);
}
/**  method verifies if a spectra contains any NaN or Inf values and replaces
  these values with SPE-specified constants
  @param  inSignal :: the vector of the spectra signals
  @param  inErr  :: the vector of the spectra errors

  @param  Signal   :: the vector of the verified spectra signals, containing
  masked values in place of NaN-s and Inf-S
  @param  Error  :: the vector of the verified spectra errors, containing masked
  values in place of NaN-s and Inf-S of the correspondent signal

*/
void SaveSPE::check_and_copy_spectra(const MantidVec &inSignal,
                                     const MantidVec &inErr, MantidVec &Signal,
                                     MantidVec &Error) const {
  if (Signal.size() != inSignal.size()) {
    Signal.resize(inSignal.size());
    Error.resize(inSignal.size());
  }
  for (size_t i = 0; i < inSignal.size(); i++) {
    if (boost::math::isnan(inSignal[i]) || boost::math::isinf(inSignal[i])) {
      Signal[i] = SaveSPE::MASK_FLAG;
      Error[i] = SaveSPE::MASK_ERROR;
    } else {
      Signal[i] = inSignal[i];
      Error[i] = inErr[i];
    }
  }
}
/** Write the bin values and errors in a single histogram spectra to the file
*  @param WS :: the workspace to being saved
*  @param outFile :: the file object to write to
*  @param specIn :: the index number of the histogram to write
*/
void SaveSPE::writeHist(const API::MatrixWorkspace_const_sptr WS,
                        FILE *const outFile, const int specIn) const {
  check_and_copy_spectra(WS->readY(specIn), WS->readE(specIn), m_tSignal,
                         m_tError);
  FPRINTF_WITH_EXCEPTION(outFile, "%s", Y_HEADER);
  writeBins(m_tSignal, outFile);

  FPRINTF_WITH_EXCEPTION(outFile, "%s", E_HEADER);
  writeBins(m_tError, outFile);
}
/** Write the mask flags for in a histogram entry
*  @param outFile :: the file object to write to
*/
void SaveSPE::writeMaskFlags(FILE *const outFile) const {
  FPRINTF_WITH_EXCEPTION(outFile, "%s", Y_HEADER);
  writeValue(MASK_FLAG, outFile);

  FPRINTF_WITH_EXCEPTION(outFile, "%s", E_HEADER);
  writeValue(MASK_ERROR, outFile);
}
/** Write the values in the array to the file in the correct format
*  @param Vs :: the array of values to write (must have length given by m_nbins)
*  @param outFile :: the file object to write to
*/
void SaveSPE::writeBins(const MantidVec &Vs, FILE *const outFile) const {

  for (size_t j = NUM_PER_LINE - 1; j < m_nBins;
       j += NUM_PER_LINE) { // output a whole line of numbers at once
    FPRINTF_WITH_EXCEPTION(outFile, NUMS_FORM, Vs[j - 7], Vs[j - 6], Vs[j - 5],
                           Vs[j - 4], Vs[j - 3], Vs[j - 2], Vs[j - 1], Vs[j]);
  }
  if (m_remainder) {
    for (size_t l = m_nBins - m_remainder; l < m_nBins; ++l) {
      FPRINTF_WITH_EXCEPTION(outFile, NUM_FORM, Vs[l]);
    }
    FPRINTF_WITH_EXCEPTION(outFile, "\n");
  }
}
/** Write the  value the file a number of times given by m_nbins
*  @param value :: the value that will be written continually
*  @param outFile :: the file object to write to
*/
void SaveSPE::writeValue(const double value, FILE *const outFile) const {
  for (size_t j = NUM_PER_LINE - 1; j < m_nBins;
       j += NUM_PER_LINE) { // output a whole line of numbers at once
    FPRINTF_WITH_EXCEPTION(outFile, NUMS_FORM, value, value, value, value,
                           value, value, value, value);
  }
  if (m_remainder) {
    for (size_t l = m_nBins - m_remainder; l < m_nBins; ++l) {
      FPRINTF_WITH_EXCEPTION(outFile, NUM_FORM, value);
    }
    FPRINTF_WITH_EXCEPTION(outFile, "\n");
  }
}
/**Write a summary information about what the algorithm had managed to save to
* the
*  file
*  @param inds :: the indices of histograms whose detectors couldn't be found
*  @param nonMasked :: the number of histograms saved successfully
*  @param masked :: the number of histograms for which mask values were written
*/
void SaveSPE::logMissingMasked(const std::vector<int> &inds,
                               const size_t nonMasked, const int masked) const {
  std::vector<int>::const_iterator index = inds.begin(), end = inds.end();
  if (index != end) {
    g_log.information() << "Found " << inds.size()
                        << " spectra without associated detectors, probably "
                           "the detectors are not present in the instrument "
                           "definition, this is not unusual. The Y values for "
                           "those spectra have been set to zero." << std::endl;
  }
  g_log.debug() << "Wrote " << nonMasked << " histograms and " << masked
                << " masked histograms to the output SPE file\n";
}
}
}
