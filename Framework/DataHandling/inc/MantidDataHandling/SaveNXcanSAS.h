// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_SAVENXCANSAS_H_
#define MANTID_DATAHANDLING_SAVENXCANSAS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"

namespace Mantid {
namespace DataHandling {

/** SaveNXcanSAS : Saves a reduced workspace in the NXcanSAS format. Currently
 * only MatrixWorkspaces resulting from 1D and 2D reductions are supported.
 */
class MANTID_DATAHANDLING_DLL SaveNXcanSAS : public API::Algorithm {
public:
  /// Constructor
  SaveNXcanSAS();
  /// Virtual dtor
  ~SaveNXcanSAS() override {}
  const std::string name() const override { return "SaveNXcanSAS"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Writes a MatrixWorkspace to a file in the NXcanSAS format.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"SaveCanSAS1D", "LoadNXcanSAS"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Nexus"; }

  std::map<std::string, std::string> validateInputs() override;

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

std::string MANTID_DATAHANDLING_DLL
makeCanSASRelaxedName(const std::string &input);

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_SAVENXCANSAS_H_ */
