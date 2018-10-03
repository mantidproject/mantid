// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_CREATEPOLARIZATIONEFFICIENCIES_H_
#define MANTID_DATAHANDLING_CREATEPOLARIZATIONEFFICIENCIES_H_

#include "MantidDataHandling/CreatePolarizationEfficienciesBase.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataHandling {

/** CreatePolarizationEfficiencies
 */
class DLLExport CreatePolarizationEfficiencies
    : public CreatePolarizationEfficienciesBase {
public:
  const std::string name() const override;
  int version() const override;
  const std::string summary() const override;
  const std::vector<std::string> seeAlso() const override;

private:
  void init() override;
  API::MatrixWorkspace_sptr
  createEfficiencies(std::vector<std::string> const &labels) override;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_CREATEPOLARIZATIONEFFICIENCIES_H_ */
