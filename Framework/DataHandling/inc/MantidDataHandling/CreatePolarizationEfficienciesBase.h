// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataHandling/DllConfig.h"
#include <memory>
#include <optional>

namespace Mantid {
namespace DataHandling {

/** CreatePolarizationEfficienciesBase - the base class for algorithms
 that create polarization efficiency workspaces:

   - CreatePolarizationEfficiencies
   - JoinISISPolarizationEfficiencies
   - LoadISISPolarizationEfficiencies
 */
class MANTID_DATAHANDLING_DLL CreatePolarizationEfficienciesBase : public API::Algorithm {
public:
  const std::string category() const override;

protected:
  ~CreatePolarizationEfficienciesBase() = default;
  void initOutputWorkspace();
  std::vector<std::string> getNonDefaultProperties(std::vector<std::string> const &props) const;

  /// Names of the efficiency properties
  static std::string const Pp;
  static std::string const Ap;
  static std::string const Rho;
  static std::string const Alpha;
  static std::string const P1;
  static std::string const P2;
  static std::string const F1;
  static std::string const F2;

private:
  void exec() override;
  /// Create the output workspace with efficiencies
  /// @param labels :: Names of the efficiencies to create
  virtual API::MatrixWorkspace_sptr createEfficiencies(std::vector<std::string> const &labels) = 0;
};

} // namespace DataHandling
} // namespace Mantid
