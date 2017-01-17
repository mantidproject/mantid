#ifndef DATAHANDING_SAVEOPENGENIEASCII_H_
#define DATAHANDING_SAVEOPENGENIEASCII_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid {
namespace DataHandling {

class DLLExport SaveOpenGenieAscii : public Mantid::API::Algorithm {
public:
  /// (Empty) Constructor
  SaveOpenGenieAscii();

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
  /// Initialisation code
  void init() override;

  /// Execution code
  void exec() override;

  void inputValidation();

  /// Typedef the tuple to outputTuple
  using outputTuple = std::tuple<std::string, std::string, std::string>;

  /// Converts XYE data to OPENGENIE strings
  std::vector<std::tuple<std::string, int>> convertWorkspaceToStrings();

  /// Stores parameters from the workspace which are required for OpenGenie
  void storeWorkspaceInformation();

  /// Parses and stores the workspace data into the output buffer
  void parseWorkspaceData();

  /// Attempts to open the user specified file path as an output stream
  std::ofstream openFileStream();

  void getSampleLogs();

  /// sort and write out the data portion of the file
  void writeDataToFile(std::ofstream &outfile);

  /// apply enginX format field which is required for OpenGenie
  void applyEnginxFormat();

  /// Calculate delta x/y/z from the log files for ENGINX
  void calculateXYZDelta(const std::string &unit, const Kernel::Property *values);

  /// Output buffer which holds the tuples to be written
  std::vector<outputTuple> m_outputVector;
  /// Workspace to save
  API::MatrixWorkspace_sptr m_inputWS;
  /// Output type String
  const std::string m_stringType = "String";
  /// Output type Float
  const std::string m_floatType = "Float";
  /// Output type Integer
  const std::string m_intType = "Integer";
};
}
}
#endif // DATAHANDING_SAVEOPENGENIEASCII_H_
