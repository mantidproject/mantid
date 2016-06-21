#ifndef DATAHANDING_SAVEOPENGENIEASCII_H_
#define DATAHANDING_SAVEOPENGENIEASCII_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid {
namespace DatHandling {

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

  // write file header
  void writeFileHeader(std::ofstream &outfile);

  /// Uses AxisHeader and WriteAxisValues to write out file
  void axisToFile(const std::string alpha, const std::string singleSpc,
                  const std::string fourspc, int nBins, bool isHistogram);

  /// Generates the header for the axis which saves to file
  std::string getAxisHeader(const std::string alpha,
                            const std::string singleSpc,
                            const std::string fourspc, int nBins);

  /// Reads if alpha is e then reads the E values accordingly
  std::string getAxisValues(std::string alpha, int bin,
                            const std::string singleSpc);

  /// Generates the header which saves to the openGenie file
  void getSampleLogs(std::string fourspc);

  /// sort and write out the sample logs
  void writeSampleLogs(std::ofstream &outfile);

  /// add ntc field which is required for OpenGenie
  void addNtc(const std::string fourspc, int nBins);

  /// apply enginX format field which is required for OpenGenie
  void applyEnginxFormat(const std::string fourspc);

  /// Vector to safe sample log
  std::vector<std::string> logVector;
  /// Workspace
  API::MatrixWorkspace_sptr ws;
};
}
}
#endif // DATAHANDING_SAVEOPENGENIEASCII_H_
