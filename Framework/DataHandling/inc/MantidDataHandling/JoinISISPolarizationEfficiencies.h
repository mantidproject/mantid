// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_JOINISISPOLARIZATIONEFFICIENCIES_H_
#define MANTID_DATAHANDLING_JOINISISPOLARIZATIONEFFICIENCIES_H_

#include "MantidDataHandling/CreatePolarizationEfficienciesBase.h"
#include "MantidDataHandling/DllConfig.h"

namespace Mantid {
namespace DataHandling {

/** JoinISISPolarizationEfficiencies : Joins reflectometry polarization
  efficiency correction factors to form a single matrix workspace.
*/
class MANTID_DATAHANDLING_DLL JoinISISPolarizationEfficiencies
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
  API::MatrixWorkspace_sptr
  createEfficiencies(std::vector<std::string> const &labels,
                     std::vector<API::MatrixWorkspace_sptr> const &workspaces);
  std::vector<API::MatrixWorkspace_sptr> interpolateWorkspaces(
      std::vector<API::MatrixWorkspace_sptr> const &workspaces);
  API::MatrixWorkspace_sptr
  interpolatePointDataWorkspace(API::MatrixWorkspace_sptr ws,
                                size_t const maxSize);
  API::MatrixWorkspace_sptr
  interpolateHistogramWorkspace(API::MatrixWorkspace_sptr ws,
                                size_t const maxSize);
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_JOINISISPOLARIZATIONEFFICIENCIES_H_ */
