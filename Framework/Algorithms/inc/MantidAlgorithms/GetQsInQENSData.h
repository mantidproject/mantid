// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_GETQSINQENSDATA_H
#define MANTID_ALGORITHMS_GETQSINQENSDATA_H

#include "MantidAPI/Algorithm.h"

#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace Algorithms {

/** Extracts Q-values from a workspace containing QENS data

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the workspace to extract the Q-values from
   </LI>
    <LI> RaiseMode - If set to true, exceptions will be raised, instead of an
   empty list being returned </LI>
    <LI> Qvalues - The Q-values extracted from the input workspace </LI>
    </UL>
*/
class DLLExport GetQsInQENSData : public API::Algorithm {
public:
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string category() const override { return "Inelastic\\Indirect"; }

  /// Algorithm's version for identification. @see Algorithm::version
  int version() const override { return 1; }

  /// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
  const std::string summary() const override {
    return "Get Q-values in the vertical axis of a MatrixWorkspace containing "
           "QENS S(Q,E) of S(theta,E) data.";
  }

  /// Algorithms name for identification. @see Algorithm::name
  const std::string name() const override { return "GetQsInQENSData"; }

  /// Cross-check properties with each other @see IAlgorithm::validateInputs
  std::map<std::string, std::string> validateInputs() override;

private:
  /// Initialization code
  void init() override;

  /// Execution code
  void exec() override;

  /// Extracts Q-values from the specified matrix workspace
  MantidVec extractQValues(const Mantid::API::MatrixWorkspace_sptr workspace);
};
} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_GETQSINQENSDATA_H */