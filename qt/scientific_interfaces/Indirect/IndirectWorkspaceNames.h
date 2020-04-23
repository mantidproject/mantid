// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include <string>
#include <utility>

#include "DllConfig.h"
#include "IndexTypes.h"
#include "IndirectFitOutput.h"
#include "IndirectFitdata.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

std::string getFitDataName(std::string baseWorkspaceName,
                           Spectra workspaceIndexes) {
  return baseWorkspaceName + workspaceIndexes.getString();
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt