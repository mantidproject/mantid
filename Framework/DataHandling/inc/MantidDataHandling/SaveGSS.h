// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidKernel/cow_ptr.h"

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

// Forward declare
namespace Mantid {
namespace HistogramData {
class Histogram;
}

namespace DataHandling {
/**
     Saves a focused data set
     into a three column GSAS format containing X_i, Y_i*step, and E_i*step.
   Exclusively for
     the crystallography package GSAS and data needs to be in time-of-flight
     For data where the focusing routine has generated several spectra (for
   example, multi-bank instruments),
     the option is provided for saving all spectra into a single file, separated
   by headers, or into
     several files that will be named "workspaceName_"+spectra_number.

     Required properties:
     <UL>
     <LI> InputWorkspace - The workspace name to save. </LI>
     <LI> Filename       - The filename for output </LI>
     </UL>

     Optional properties:
     <UL>
     <LI> SplitFiles - Option for splitting into N files for workspace with
   N-spectra</LI>
     <LI> Append     - Append to Filename, if it already exists (default:
   true).</LI>
     <LI> Bank       - The bank number of the first spectrum (default: 1)</LI>
     </UL>

     @author Laurent Chapon, ISIS Facility, Rutherford Appleton Laboratory
     @date 04/03/2009
  */
class MANTID_DATAHANDLING_DLL SaveGSS : public Mantid::API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "SaveGSS"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Saves a focused data set into a three column GSAS format."; }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"LoadGSS", "SaveVulcanGSS", "SaveGSASInstrumentFile", "SaveAscii"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "Diffraction\\DataHandling;DataHandling\\Text"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  /// Determines if all spectra have detectors
  bool areAllDetectorsValid() const;

  /// Process input user-specified headers
  void processUserSpecifiedHeaders();

  /// Turns the data associated with this spectra into a string stream
  void generateBankData(std::stringstream &outBuf, size_t specIndex, const std::string &outputFormat,
                        const std::vector<int> &slog_xye_precisions) const;

  /// Generates the bank header and returns this as a string stream
  void generateBankHeader(std::stringstream &out, const API::SpectrumInfo &spectrumInfo, size_t specIndex) const;

  /// Generates the output which will be written to the GSAS file
  void generateGSASBuffer(size_t numOutFiles, size_t numOutSpectra);

  /// Generates the instrument header and returns this as a string stream
  void generateInstrumentHeader(std::stringstream &out, double l1) const;

  /// Generates the filename(s) and paths to write to and stores in member var
  void generateOutFileNames(size_t numberOfOutFiles);

  /// Returns the log value in a GSAS format as a string stream
  void getLogValue(std::stringstream &out, const API::Run &runInfo, const std::string &name,
                   const std::string &failsafeValue = "UNKNOWN") const;

  /// Returns if the input workspace instrument is valid
  bool isInstrumentValid() const;

  /// Opens a new file stream at the path specified.
  void openFileStream(const std::string &outFilePath, std::ofstream &outStream);

  /// sets non workspace properties for the algorithm
  void setOtherProperties(IAlgorithm *alg, const std::string &propertyName, const std::string &propertyValue,
                          int periodNum) override;

  /// Validates the user input and warns / throws on bad conditions
  std::map<std::string, std::string> validateInputs() override;

  /// Writes the current buffer to the user specified file path
  void writeBufferToFile(size_t numOutFiles, size_t numSpectra);

  // Writes the header for RALF data format to the buffer
  void writeRALFHeader(std::stringstream &out, int bank, const HistogramData::Histogram &histo) const;

  /// Write out the data in RALF - ALT format
  void writeRALF_ALTdata(std::stringstream &out, const int bank, const HistogramData::Histogram &histo) const;

  /// Write out the data in RALF - FXYE format
  void writeRALF_XYEdata(const int bank, const bool MultiplyByBinWidth, std::stringstream &out,
                         const HistogramData::Histogram &histo) const;

  /// Write out the data in SLOG format
  void writeSLOGdata(const size_t ws_index, const int bank, const bool MultiplyByBinWidth, std::stringstream &out,
                     const HistogramData::Histogram &histo, const std::vector<int> &xye_precision) const;

  /// Workspace
  API::MatrixWorkspace_const_sptr m_inputWS;
  /// The output buffer. This is either n spectra in one file,
  /// or n files with 1 spectra
  std::vector<std::unique_ptr<std::stringstream>> m_outputBuffer{};
  /// The output filename(s)
  std::vector<std::string> m_outFileNames{};
  /// Indicates whether all spectra have valid detectors
  bool m_allDetectorsValid{false};
  /// Holds pointer to progress bar
  std::unique_ptr<API::Progress> m_progress{nullptr};
  /// User specified header string
  std::vector<std::string> m_user_specified_gsas_header;
  /// flag to overwrite standard GSAS header
  bool m_overwrite_std_gsas_header{false};
  /// User specified bank header
  std::vector<std::string> m_user_specified_bank_headers;
  /// flag to overwrite standard GSAS bank header
  bool m_overwrite_std_bank_header{false};
};
} // namespace DataHandling
} // namespace Mantid
