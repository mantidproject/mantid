// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_SAVERMCPROFILE_H_
#define MANTID_DATAHANDLING_SAVERMCPROFILE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataHandling {

/** SaveRMCProfile : Saves a workspace containing a spectral density in a format
readable by the RMCProfile package.

Required Properties:
<UL>
<LI> InputWorkspace - An input workspace with units of Q </LI>
<LI> Filename - The filename to use for the saved data </LI>
</UL>
 */
class DLLExport SaveRMCProfile : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"SaveRMCProfile"};
  }
  const std::string category() const override;
  const std::string summary() const override;
  std::map<std::string, std::string> validateInputs() override;

private:
  void init() override;
  void exec() override;
  void writeMetaData(std::ofstream &out,
                     API::MatrixWorkspace_const_sptr inputWS);
  void writeWSData(std::ofstream &out, API::MatrixWorkspace_const_sptr inputWS);
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_SAVERMCPROFILE_H_ */
