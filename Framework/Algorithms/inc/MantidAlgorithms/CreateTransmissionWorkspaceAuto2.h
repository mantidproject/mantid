// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACEAUTO2_H_
#define MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACEAUTO2_H_

#include "ReflectometryWorkflowBase2.h"

namespace Mantid {
namespace Algorithms {

/** CreateTransmissionWorkspaceAuto2 : Creates a transmission run workspace in
Wavelength from input TOF workspaces. Version 2.
*/
class DLLExport CreateTransmissionWorkspaceAuto2
    : public ReflectometryWorkflowBase2 {
public:
  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string name() const override {
    return "CreateTransmissionWorkspaceAuto";
  }
  /// Algorithm's version for identification. @see Algorithm::version
  int version() const override { return 2; }
  const std::vector<std::string> seeAlso() const override {
    return {"CreateTransmissionWorkspace"};
  }
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string category() const override { return "Reflectometry\\ISIS"; }
  /// Algorithm's summary for documentation
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACEAUTO2_H_ */
