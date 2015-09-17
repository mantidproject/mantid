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
  void WriteToFile(
    const std::string alpha, std::string filename, const std::string comment, 
    const std::string comstr, const std::string errstr,
    const std::string errstr2, const std::string GXR, const std::string banknum,
    const std::string fourspc, const std::string twospc,
    API::MatrixWorkspace_const_sptr ws, int nBins);

  void WriteHeader(
    const std::string alpha, std::ofstream& outfile, const std::string comment,
    const std::string GXR, const std::string banknum, const std::string fourspc,
    const std::string twospc, API::MatrixWorkspace_const_sptr ws);

  double WriteAxisValues(std::string alpha, int bin, API::MatrixWorkspace_const_sptr ws);

    /// Workspace
  API::MatrixWorkspace_const_sptr ws;
};


}
}
#endif // DATAHANDING_SAVEOPENGENIEASCII_H_ 
