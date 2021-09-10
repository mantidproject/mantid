// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

/** An algorithm used to crop an MDHistoWorkspace based on the first
    non-zero signals found in each dimension.

  @author Matt King
  @date 02-10-2015
*/

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidMDAlgorithms/CutMD.h"
#include "boost/shared_ptr.hpp"
namespace Mantid {
namespace MDAlgorithms {
class DLLExport CompactMD : public API::Algorithm {
public:
  void init() override;
  void exec() override;
  /// Algorithm's name for identification
  const std::string name() const override { return "CompactMD"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Crops an MDHistoWorkspace based on the first non-zero signals "
           "giving a more focussed area of interest.";
  }
  const std::string category() const override { return "MDAlgorithms\\Utility\\Workspaces"; }
  /// Algorithm's version for identification
  int version() const override { return 1; }
  /// Finding the extents of the first non-zero signals.
  void findFirstNonZeroMinMaxExtents(const Mantid::API::IMDHistoWorkspace_sptr &inputWs,
                                     std::vector<Mantid::coord_t> &minVec, std::vector<Mantid::coord_t> &maxVec);
};
} // namespace MDAlgorithms
} // namespace Mantid
