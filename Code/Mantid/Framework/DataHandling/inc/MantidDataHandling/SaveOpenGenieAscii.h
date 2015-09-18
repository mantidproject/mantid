#ifndef DATAHANDING_SAVEOPENGENIEASCII_H_
#define DATAHANDING_SAVEOPENGENIEASCII_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/cow_ptr.h"

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

  /// Write out the data for e x y
  void WriteToFile(const std::string alpha, std::ofstream &outfile,
                   const std::string comment, const std::string fourspc,
                   int nBins, bool isHistogram, int nSpectra);

  void WriteAxisHeader(const std::string alpha, std::ofstream &outfile,
                       const std::string singleSpc, const std::string fourspc,
                       int nSpectra);

  void WriteAxisValues(std::string alpha, std::ofstream &outfile, int bin,
                       const std::string singleSpc);

  /// Workspace
  API::MatrixWorkspace_sptr ws;
};
}
}
#endif // DATAHANDING_SAVEOPENGENIEASCII_H_
