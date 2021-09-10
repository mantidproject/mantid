// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/ParallelAlgorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataHandling {

/** SortTableWorkspace : TODO: DESCRIPTION
 */
class DLLExport SortTableWorkspace : public API::ParallelAlgorithm {
public:
  const std::string name() const override { return "SortTableWorkspace"; }
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"CreateEmptyTableWorkspace"}; }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid
