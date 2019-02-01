// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_ORMD_H_
#define MANTID_MDALGORITHMS_ORMD_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"
#include "MantidMDAlgorithms/BooleanBinaryOperationMD.h"

namespace Mantid {
namespace MDAlgorithms {

/** OrMD : boolean operation on MDHistoWorkspaces

  @date 2011-11-08
*/
class DLLExport OrMD : public BooleanBinaryOperationMD {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"AndMD", "XorMD", "NotMD"};
  }

private:
  void execHistoHisto(
      Mantid::DataObjects::MDHistoWorkspace_sptr out,
      Mantid::DataObjects::MDHistoWorkspace_const_sptr operand) override;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_ORMD_H_ */
