#ifndef DATAHANDING_SAVEOPENGENIEASCII_H_
#define DATAHANDING_SAVEOPENGENIEASCII_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace DataHandling {

class DLLExport SaveOpenGenieAscii : public Mantid::API::Algorithm {
public:
  /// (Empty) Constructor
  SaveOpenGenieAscii() = default;
  ~SaveOpenGenieAscii() = default;

  /// Algorithm's name
  const std::string name() const override { return "SaveOpenGenieAscii"; }

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Saves a focused data set into an OpenGenie ASCII file ";
  }

  /// Algorithm's version
  int version() const override { return (1); }

  /// Algorithm's category for identification
  const std::string category() const override {
    return "Diffraction\\DataHandling;DataHandling\\Text";
  }

private:
  /// Typedef of a tuple containing the name, type and value as strings
  using outputTuple = std::tuple<std::string, std::string, std::string>;

  /// Initialisation code
  void init() override;

  /// Execution code
  void exec() override;

  inline void addToOutputBuffer(const std::string outName,
                                const std::string outType,
                                const std::string outVal) {
    m_outputVector.push_back(outputTuple(outName, outType, outVal));
  }

  /// Adds ENGINX related data which is required for OpenGenie
  void applyEnginxFormat();

  /// Calculate delta x/y/z from the log files for ENGINX
  void calculateXYZDelta(const std::string &unit,
                         const Kernel::Property *values);

  /// Converts XYE data to OPENGENIE strings and number of data points
  std::vector<std::tuple<std::string, int>> convertWorkspaceToStrings();

  /// Parses and stores appropriate output logs into the output buffer
  void getSampleLogs();

  /// Validates that workspace is focused and not empty
  void inputValidation();

  /// Attempts to open the user specified file path as an output stream
  std::ofstream openFileStream();

  /// Parses and stores the workspace data into the output buffer
  void parseWorkspaceData();

  /// Stores fields that aren't found in the WS but required by OpenGenie
  void storeEmptyFields();

  /// Stores parameters from the workspace which are required for OpenGenie
  void storeWorkspaceInformation();

  /// sorts and writes out the data portion of the file
  void writeDataToFile(std::ofstream &outfile);

  /// Output buffer which holds the tuples to be written
  std::vector<outputTuple> m_outputVector;
  /// Workspace to save
  API::MatrixWorkspace_const_sptr m_inputWS;
  /// Output type - String
  const std::string m_stringType = "String";
  /// Output type - Float
  const std::string m_floatType = "Float";
  /// Output type - Integer
  const std::string m_intType = "Integer";
};
}
}
#endif // DATAHANDING_SAVEOPENGENIEASCII_H_
