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

  /// Typedef the tuple to outputTuple
  using outputTuple = std::tuple<std::string, std::string, std::string>;

  /// Converts XYE data to OPENGENIE strings
  std::vector<std::tuple<std::string, int>> convertWorkspaceToStrings();

  /// Parses and stores the workspace data into the output buffer
  void parseWorkspaceData();

  void getSampleLogs();

  /// Generates a ENGINX compatible spectrum number string
  std::string getSpectrumNumAsString(const API::MatrixWorkspace &wsToSave);

  /// sort and write out the data portion of the file
  void writeDataToFile(std::ofstream &outfile);

  /// apply enginX format field which is required for OpenGenie
  void applyEnginxFormat();

  /// Vector to safe sample log
  std::vector<outputTuple> m_outputVector;
  /// Workspace
  API::MatrixWorkspace_sptr m_inputWS;
};
}
}
#endif // DATAHANDING_SAVEOPENGENIEASCII_H_
