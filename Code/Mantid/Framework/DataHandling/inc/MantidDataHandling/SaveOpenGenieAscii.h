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

  //// Virtual destructor
  virtual ~SaveOpenGenieAscii() {}

  /// Algorithm's name
  virtual const std::string name() const { return "SaveOpenGenieAscii"; }

  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Saves a focused data set into an OpenGenie ASCII file ";
  }

  /// Algorithm's version
  virtual int version() const { return (1); }

  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "Diffraction;DataHandling\\Text";
  }

private:
  /// Initialisation code
  void init();

  /// Execution code
  void exec();

  // write file header
  void writeFileHeader(std::ofstream &outfile);

  /// Uses AxisHeader and WriteAxisValues to write out file
  void writeToFile(const std::string alpha, const std::string singleSpc,
                   const std::string fourspc, int nBins, bool isHistogram);

  /// Generates the header for the axis which saves to file
  std::string writeAxisHeader(const std::string alpha,
                              const std::string singleSpc,
                              const std::string fourspc, int nBins);

  /// Reads if alpha is e then reads the E values accordingly
  std::string writeAxisValues(std::string alpha, int bin,
                              const std::string singleSpc);

  /// Generates the header which saves to the openGenie file
  void writeSampleLogs(std::ofstream &outfile, std::string fourspc);

  // vector
  std::vector<std::string> myVector;
  /// Workspace
  API::MatrixWorkspace_sptr ws;
};
}
}
#endif // DATAHANDING_SAVEOPENGENIEASCII_H_
