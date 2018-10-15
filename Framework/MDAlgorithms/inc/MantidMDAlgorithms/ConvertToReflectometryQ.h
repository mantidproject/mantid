// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_CONVERTTOREFLECTOMETRYQ_H_
#define MANTID_MDALGORITHMS_CONVERTTOREFLECTOMETRYQ_H_

#include "MantidMDAlgorithms/BoxControllerSettingsAlgorithm.h"

namespace Mantid {
namespace MDAlgorithms {

/** ConvertToReflectometryQ : Creates a 2D MD Histogram workspace with two axis
  qz and qx.

  @date 2012-03-21
*/
class DLLExport ConvertToReflectometryQ
    : public BoxControllerSettingsAlgorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Transforms from real-space to Q or momentum space for "
           "reflectometry workspaces";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"ConvertUnits", "ReflectometryMomentumTransfer"};
  }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
  Mantid::API::MatrixWorkspace_sptr
  correctDetectors(Mantid::API::MatrixWorkspace_sptr inputWs, double theta);
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_CONVERTTOREFLECTOMETRYQ_H_ */
