// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IndirectFittingModel.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class DLLExport MSDFitModel : public IndirectFittingModel {
public:
  MSDFitModel();
  std::string singleFitOutputName(TableDatasetIndex index,
                                  WorkspaceIndex spectrum) const override;

private:
  std::string getResultXAxisUnit() const override;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
