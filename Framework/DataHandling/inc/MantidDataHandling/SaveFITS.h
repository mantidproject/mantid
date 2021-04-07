// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataHandling/DllConfig.h"

namespace Mantid {
namespace DataHandling {

/**
  SaveFITS : Save images in FITS formats.
*/
class DLLExport SaveFITS final : public API::Algorithm {
public:
  const std::string name() const override final;

  int version() const override final;
  const std::vector<std::string> seeAlso() const override { return {"LoadFITS", "SaveNXTomo"}; }

  const std::string category() const override final;

  const std::string summary() const override final;

private:
  void init() override final;

  void exec() override final;

  std::map<std::string, std::string> validateInputs() override;

  void saveFITSImage(const API::MatrixWorkspace_sptr &img, const std::string &filename);

  void writeFITSHeaderBlock(const API::MatrixWorkspace_sptr &img, std::ofstream &file);

  void writeFITSImageMatrix(const API::MatrixWorkspace_sptr &img, std::ofstream &file);

  void writeFITSHeaderEntry(const std::string &hdr, std::ofstream &file);

  std::string makeBitDepthHeader(size_t depth) const;

  void writeFITSHeaderAxesSizes(const API::MatrixWorkspace_sptr &img, std::ofstream &file);

  void writePaddingFITSHeaders(size_t count, std::ofstream &file);

  static const size_t g_maxBitDepth;
  static const std::array<int, 3> g_bitDepths;
  static const size_t g_maxBytesPP;
  // size of header entries in bytes
  static const size_t g_maxLenHdr;
  // pre-defined header contents
  static const std::string g_FITSHdrEnd;
  static const std::string g_FITSHdrFirst;
  static const std::string g_bitDepthPre;
  static const std::string g_bitDepthPost;
  static const std::string g_FITSHdrAxes;
  static const std::string g_FITSHdrExtensions;
  static const std::string g_FITSHdrRefComment1;
  static const std::string g_FITSHdrRefComment2;
};

} // namespace DataHandling
} // namespace Mantid
