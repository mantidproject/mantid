// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"
#include <iosfwd>

namespace Mantid {
namespace DataHandling {

class MANTID_DATAHANDLING_DLL SaveOpenGenieAscii : public Mantid::API::Algorithm {
public:
  /// (Empty) Constructor
  SaveOpenGenieAscii() = default;
  ~SaveOpenGenieAscii() override = default;

  /// Algorithm's name
  const std::string name() const override { return "SaveOpenGenieAscii"; }

  /// Summary of algorithms purpose
  const std::string summary() const override { return "Saves a focused data set into an OpenGenie ASCII file "; }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"SaveAscii"}; }

  /// Algorithm's category for identification
  const std::string category() const override { return "Diffraction\\DataHandling;DataHandling\\Text"; }

private:
  /// Typedef of a tuple containing the name, type and value as strings
  using OutputBufferEntry = std::tuple<std::string, std::string, std::string>;

  /// Initialisation code
  void init() override;

  /// Execution code
  void exec() override;

  inline void addToOutputBuffer(const std::string &outName, const std::string &outType, const std::string &outVal) {
    m_outputVector.emplace_back(OutputBufferEntry(outName, outType, outVal));
  }

  /// Adds ENGINX related data which is required for OpenGenie
  void applyEnginxFormat();

  /// Calculate delta x/y/z from the log files for ENGINX
  void calculateXYZDelta(const std::string &unit, const Kernel::Property *values);

  /// Converts XYE data to a tuple containing the OPENGENIE string and store
  /// it into the output buffer
  template <typename T> void convertWorkspaceData(const T &histoData, const char &axis);

  /// Determines the ENGIN-X bank from the detectors IDs present
  void determineEnginXBankId();

  /// Parses and stores appropriate output logs into the output buffer
  void getSampleLogs();

  /// Validates that workspace is focused and not empty
  void inputValidation();

  /// Attempts to open the user specified file path as an output stream
  void openFileStream(std::ofstream &stream);

  /// Stores fields that aren't found in the WS but required by OpenGenie
  void storeEmptyFields();

  /// Stores parameters from the workspace which are required for OpenGenie
  void storeWorkspaceInformation();

  /// sorts and writes out the data portion of the file
  void writeDataToFile(std::ofstream &outfile);

  /// Output buffer which holds the tuples to be written
  std::vector<OutputBufferEntry> m_outputVector;
  /// Workspace to save
  API::MatrixWorkspace_const_sptr m_inputWS;
  /// Output type - String
  const std::string m_stringType = "String";
  /// Output type - Float
  const std::string m_floatType = "Float";
  /// Output type - Integer
  const std::string m_intType = "Integer";
};
} // namespace DataHandling
} // namespace Mantid
