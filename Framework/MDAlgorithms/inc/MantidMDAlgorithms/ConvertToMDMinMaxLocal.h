// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/System.h"
#include "MantidMDAlgorithms/ConvertToMDParent.h"

namespace Mantid {
namespace MDAlgorithms {

/** ConvertToMDMinMaxLocal : Algorithm to calculate limits for ConvertToMD
 */
class DLLExport ConvertToMDMinMaxLocal : public ConvertToMDParent {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculate limits of ConvertToMD transformation possible for this "
           "particular workspace and the instrument, attached to it.";
  }

  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"ConvertToMD"}; }

protected: // for testing
  void findMinMaxValues(MDWSDescription &WSDescription, MDTransfInterface *const pQtransf,
                        Kernel::DeltaEMode::Type iEMode, std::vector<double> &MinValues,
                        std::vector<double> &MaxValues);

private:
  void exec() override;
  void init() override;
  /// pointer to the input workspace;
  Mantid::DataObjects::Workspace2D_sptr m_MinMaxWS2D;
};

} // namespace MDAlgorithms
} // namespace Mantid
