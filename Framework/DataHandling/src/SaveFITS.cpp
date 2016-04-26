#include "MantidDataHandling/SaveFITS.h"

//// ????
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/make_unique.h"

#include <fstream>

#include <Poco/File.h>
#include <Poco/Path.h>

namespace Mantid {
namespace DataHandling {

const size_t SaveFITS::g_maxLenHdr = 80;
// TODO: add headers for ToF, time bin, counts, triggers, etc.
const std::string SaveFITS::g_FITSHdrEnd = "END";
const std::string SaveFITS::g_FITSHdrFirst =
    "SIMPLE  =                    T / file does conform to FITS standard";
const std::string SaveFITS::g_FITSHdrBitDepth =
    "BITPIX  =                   16 / number of bits per data pixel";
const std::string SaveFITS::g_FITSHdrAxes =
    "NAXIS   =                    2 / number of data axes";
const std::string SaveFITS::g_FITSHdrExtensions =
    "EXTEND  =                    T / FITS dataset may contain extensions";
const std::string SaveFITS::g_FITSHdrRefComment1 =
    "COMMENT   FITS (Flexible Image "
    "Transport System) format is defined in "
    "'Astronomy";
const std::string SaveFITS::g_FITSHdrRefComment2 =
    "COMMENT   and Astrophysics', volume 376, page 359; bibcode: "
    "2001A&A...376..359H";

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveFITS)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string SaveFITS::name() const { return "SaveFITS"; }

/// Algorithm's version for identification. @see Algorithm::version
int SaveFITS::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SaveFITS::category() const { return "DataHandling\\Imaging"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string SaveFITS::summary() const {
  return "Aggregates images from multiple energy bands or wavelengths";
}

namespace {
const std::string PROP_INPUT_WS = "InputWorkspace";
const std::string PROP_FILENAME = "Filename";
const std::string PROP_BIT_DEPTH = "BitDepth";

// just to compare two Poco::Path objects, used for std algorithms
struct PocoPathComp
    : public std::binary_function<Poco::Path, Poco::Path, bool> {
  bool operator()(const Poco::Path &lhs, const Poco::Path &rhs) const {
    return lhs.toString() < rhs.toString();
  }
};
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SaveFITS::init() {
  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<>>(
          PROP_INPUT_WS, "", Kernel::Direction::Input,
          boost::make_shared<API::WorkspaceUnitValidator>("Label")),
      "Workspace holding an image (with one spectrum per pixel row).");

  declareProperty(Kernel::make_unique<API::FileProperty>(
                      PROP_FILENAME, "", API::FileProperty::Save,
                      std::vector<std::string>(1, ".fits")),
                  "Name of the output file where the image is saved.");

  const std::vector<int> bitDepths{16};
  declareProperty(PROP_BIT_DEPTH, 16,
                  boost::make_shared<Kernel::ListValidator<int>>(bitDepths),
                  "The bit depth or number of bits per pixel to use for the "
                  "output image(s). Only 16 bits is supported at the "
                  "moment.",
                  Direction::Input);
}

std::map<std::string, std::string> SaveFITS::validateInputs() {
  std::map<std::string, std::string> result;

  return result;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SaveFITS::exec() {
  auto ws = getProperty(PROP_INPUT_WS);
  const auto filename = getPropertyValue(PROP_FILENAME);

  saveFITSImage(ws, filename);
  g_log.notice() << "Image of size " + std::to_string(0) + " columns by " +
                        std::to_string(0) + " rows saved in '" + filename +
                        "'" << std::endl;
}

/**
 * Save an image workspace into a file.
 *
 * @param img matrix workspace (one spectrum per row)
 * @param filename relative or full path, should be already checked
 */
void SaveFITS::saveFITSImage(const API::MatrixWorkspace_sptr img,
                             const std::string &filename) {
  std::ofstream outfile(filename, std::ofstream::binary);

  writeFITSHeaderBlock(img, outfile);
  writeFITSImageMatrix(img, outfile);
}

void SaveFITS::writeFITSHeaderBlock(const API::MatrixWorkspace_sptr img,
                                    std::ofstream &file) {
  // minimal sequence of standard headers
  writeFITSHeaderEntry(g_FITSHdrFirst, file);
  writeFITSHeaderEntry(g_FITSHdrBitDepth, file);
  writeFITSHeaderEntry(g_FITSHdrAxes, file);
  writeFITSHeaderAxesSizes(img, file);
  writeFITSHeaderEntry(g_FITSHdrExtensions, file);
  writeFITSHeaderEntry(g_FITSHdrRefComment1, file);
  writeFITSHeaderEntry(g_FITSHdrRefComment2, file);
  writeFITSHeaderEntry(g_FITSHdrEnd, file);

  const size_t entriesPerHDU = 36;
  writePaddingFITSHeaders(entriesPerHDU - 9, file);
}

void SaveFITS::writeFITSImageMatrix(const API::MatrixWorkspace_sptr img,
                                    std::ofstream &file) {
  const size_t sizeX = img->blocksize();
  const size_t sizeY = img->getNumberHistograms();

  for (size_t row = 0; row < sizeY; row++) {
    Mantid::API::ISpectrum *spectrum = img->getSpectrum(row);
    const auto &dataY = spectrum->readY();
    for (size_t col = 0; col < sizeX; col++) {
      int16_t pixelVal = static_cast<uint16_t>(dataY[col]);

      // change endianness: to sequence of bytes in big-endian
      const size_t bytespp = 2;
      uint8_t bytesPixel[bytespp];
      uint8_t *iter = reinterpret_cast<uint8_t *>(&pixelVal);
      std::reverse_copy(iter, iter + bytespp, bytesPixel);

      file.write(reinterpret_cast<const char *>(&bytesPixel),
                 sizeof(bytesPixel));
    }
  }
}

void SaveFITS::writeFITSHeaderEntry(const std::string &hdr,
                                    std::ofstream &file) {
  static const std::vector<char> zeros(g_maxLenHdr, 0);

  size_t count = hdr.size();
  if (count >= g_maxLenHdr)
    count = g_maxLenHdr;

  file.write(hdr.c_str(), sizeof(char) * count);
  file.write(zeros.data(), g_maxLenHdr - count);
}

void SaveFITS::writeFITSHeaderAxesSizes(const API::MatrixWorkspace_sptr img,
                                        std::ofstream &file) {
  const std::string sizeX = std::to_string(img->blocksize());
  const std::string sizeY = std::to_string(img->getNumberHistograms());

  const size_t fieldWidth = 20;
  std::stringstream axis1;
  axis1 << "NAXIS1  = " << std::setw(fieldWidth) << sizeX
        << " / length of data axis 1";
  writeFITSHeaderEntry(axis1.str(), file);

  std::stringstream axis2;
  axis2 << "NAXIS2  = " << std::setw(fieldWidth) << sizeY
        << " / length of data axis 2";
  writeFITSHeaderEntry(axis2.str(), file);
}

/**
 * Writes the padding required to fill every header block. FITS headers consist
 * of subblocks of 36 entries/lines. This method
 * is to write the "padding" lines required to have 36 in a block
 *
 * @param count how may bytes to write
 * @param file output stream to write to
 */
void SaveFITS::writePaddingFITSHeaders(size_t count, std::ofstream &file) {
  static const std::vector<char> zeros(g_maxLenHdr, 0);

  for (size_t i = 0; i < count; ++i) {
    file.write(zeros.data(), g_maxLenHdr);
  }
}

} // namespace DataHandling
} // namespace Mantid
