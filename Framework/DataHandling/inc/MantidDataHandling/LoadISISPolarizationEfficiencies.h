// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADISISPOLARIZATIONEFFICIENCIES_H_
#define MANTID_DATAHANDLING_LOADISISPOLARIZATIONEFFICIENCIES_H_

#include "MantidDataHandling/CreatePolarizationEfficienciesBase.h"
#include "MantidDataHandling/DllConfig.h"

namespace Mantid {
namespace DataHandling {

/** LoadISISPolarizationEfficiencies : Load reflectometry polarization
  efficiency correction factors from disk.
*/
class MANTID_DATAHANDLING_DLL LoadISISPolarizationEfficiencies
    : public CreatePolarizationEfficienciesBase {
public:
  const std::string name() const override;
  int version() const override;
  const std::string summary() const override;
  const std::vector<std::string> seeAlso() const override;

private:
  void init() override;
  API::MatrixWorkspace_sptr
  createEfficiencies(std::vector<std::string> const &props) override;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADISISPOLARIZATIONEFFICIENCIES_H_ */
