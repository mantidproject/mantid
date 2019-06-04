// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveFITS.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidKernel/ListValidator.h"


#include <fstream>
#include <iomanip>

#include <boost/pointer_cast.hpp>

#include <Poco/File.h>
#include <Poco/Path.h>

namespace Mantid {
namespace DataHandling {

const size_t SaveFITS::g_maxLenHdr = 80;

// TODO: add headers for ToF, time bin, counts, triggers, etc.
const std::string SaveFITS::g_FITSHdrEnd = "END";
const std::string SaveFITS::g_FITSHdrFirst =
    "SIMPLE  =                    T / file does conform to FITS standard";

// To build something like:
// "BITPIX  =                   16 / number of bits per data pixel";
const std::string SaveFITS::g_bitDepthPre = "BITPIX  =                   ";
const std::string SaveFITS::g_bitDepthPost = " / number of bits per data pixel";

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

// extend this if we ever want to support 64 bits pixels
const size_t SaveFITS::g_maxBitDepth = 32;
// this has to have int type for the validator and getProperty
const std::array<int, 3> SaveFITS::g_bitDepths = {
    {8, 16, static_cast<int>(g_maxBitDepth)}};
const size_t SaveFITS::g_maxBytesPP = g_maxBitDepth / 8;

using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

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
  return "Saves image data from a workspace in FITS (Flexible Image Transport "
         "System) format";
}

namespace {
const std::string PROP_INPUT_WS = "InputWorkspace";
const std::string PROP_FILENAME = "Filename";
const std::string PROP_BIT_DEPTH = "BitDepth";
} // namespace

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SaveFITS::init() {
  declareProperty(
      std::make_unique<API::WorkspaceProperty<>>(
          PROP_INPUT_WS, "", Kernel::Direction::Input,
          boost::make_shared<API::WorkspaceUnitValidator>("Label")),
      "Workspace holding an image (with one spectrum per pixel row).");

  declareProperty(std::make_unique<API::FileProperty>(
                      PROP_FILENAME, "", API::FileProperty::Save,
                      std::vector<std::string>(1, ".fits")),
                  "Name of the output file where the image is saved.");

  declareProperty(PROP_BIT_DEPTH, 16,
                  boost::make_shared<Kernel::ListValidator<int>>(g_bitDepths),
                  "The bit depth or number of bits per pixel to use for the "
                  "output image(s). Only 16 bits is supported at the "
                  "moment.",
                  Direction::Input);
}

std::map<std::string, std::string> SaveFITS::validateInputs() {
  std::map<std::string, std::string> result;

  API::MatrixWorkspace_const_sptr wks = getProperty(PROP_INPUT_WS);
  if (wks) {
    if (0 == wks->blocksize()) {
      result[PROP_INPUT_WS] = "The input workspace must have at least one "
                              "column (the X axis is empty)";
    }
    if (0 == wks->getNumberHistograms()) {
      result[PROP_INPUT_WS] = "The input workspace must have at least one row "
                              "(the Y axis is empty)";
    }
  }

  return result;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SaveFITS::exec() {
  API::MatrixWorkspace_sptr ws = getProperty(PROP_INPUT_WS);
  const auto filename = getPropertyValue(PROP_FILENAME);

  saveFITSImage(ws, filename);
  g_log.information() << "Image of size " + std::to_string(ws->blocksize()) +
                             " columns by " +
                             std::to_string(ws->getNumberHistograms()) +
                             " rows saved in '" + filename + "'\n";
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
  int depth = getProperty(PROP_BIT_DEPTH);
  const std::string bitDepthHdr = makeBitDepthHeader(depth);
  writeFITSHeaderEntry(bitDepthHdr, file);
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

  int bitDepth = getProperty(PROP_BIT_DEPTH);
  const size_t bytespp = static_cast<size_t>(bitDepth) / 8;

  for (size_t row = 0; row < sizeY; ++row) {
    const auto &yData = img->y(row);
    for (size_t col = 0; col < sizeX; ++col) {
      int32_t pixelVal;
      if (8 == bitDepth) {
        pixelVal = static_cast<uint8_t>(yData[col]);
      } else if (16 == bitDepth) {
        pixelVal = static_cast<uint16_t>(yData[col]);
      } else if (32 == bitDepth) {
        pixelVal = static_cast<uint32_t>(yData[col]);
      }

      // change endianness: to sequence of bytes in big-endian
      // this needs revisiting (similarly in LoadFITS)
      // See https://github.com/mantidproject/mantid/pull/15964
      std::array<uint8_t, g_maxBytesPP> bytesPixel;
      uint8_t *iter = reinterpret_cast<uint8_t *>(&pixelVal);
      std::reverse_copy(iter, iter + bytespp, bytesPixel.data());

      file.write(reinterpret_cast<const char *>(bytesPixel.data()), bytespp);
    }
  }
}

void SaveFITS::writeFITSHeaderEntry(const std::string &hdr,
                                    std::ofstream &file) {
  static const std::vector<char> blanks(g_maxLenHdr, 32);

  auto count = hdr.size();
  if (count >= g_maxLenHdr)
    count = g_maxLenHdr;

  file.write(hdr.c_str(), sizeof(char) * count);
  file.write(blanks.data(), g_maxLenHdr - count);
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

std::string SaveFITS::makeBitDepthHeader(size_t depth) const {
  std::stringstream hdr;
  hdr << g_bitDepthPre << std::setw(2) << depth << g_bitDepthPost;
  return hdr.str();
}

/**
 * Writes the padding required to fill every header block. FITS
 * headers consist of subblocks of 36 entries/lines, with 80
 * characters per line. This method is to write as many "padding"
 * lines as required to have 36 lines in a block
 *
 * @param count how may bytes to write
 * @param file output stream to write to
 */
void SaveFITS::writePaddingFITSHeaders(size_t count, std::ofstream &file) {
  static const std::vector<char> blanks(g_maxLenHdr, 32);

  for (size_t i = 0; i < count; ++i) {
    file.write(blanks.data(), g_maxLenHdr);
  }
}

} // namespace DataHandling
} // namespace Mantid
