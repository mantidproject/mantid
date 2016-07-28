#ifndef MANTID_DATAHANDLING_SAVEDIFFFITTINGASCII_H_
#define MANTID_DATAHANDLING_SAVEDIFFFITTINGASCII_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"

namespace Mantid {
namespace DataHandling {

class DLLExport SaveDiffFittingAscii : public Mantid::API::Algorithm {
public:
  /// (Empty) Constructor
  SaveDiffFittingAscii();

  /// Algorithm's name
  const std::string name() const override { return "SaveDiffFittingAscii"; }

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Saves the results after carrying out single peak fitting process "
           "or running "
           "EnggFitPeaks v1 algorithm to ASCII file";
  }

  /// Algorithm's version
  int version() const override { return (1); }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Text"; }

private:
  /// Initialisation code
  void init() override;

  /// Execution code
  void exec() override;

  template <class T>
  void writeVal(T &val, std::ofstream &file, bool endline);

};
} // namespace DataHandling
} // namespace Mantid

#endif /*  MANTID_DATAHANDLING_SAVEDIFFFITTINGASCII_H_  */
