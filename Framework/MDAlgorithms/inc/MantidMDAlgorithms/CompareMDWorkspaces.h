// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace MDAlgorithms {

/** Compare two MDWorkspaces for equality.

  @date 2012-01-19
*/
class DLLExport CompareMDWorkspaces : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Compare two MDWorkspaces for equality."; }

  int version() const override;
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
  void doComparison();
  void compareMDGeometry(const Mantid::API::IMDWorkspace_sptr &ws1, const Mantid::API::IMDWorkspace_sptr &ws2);
  void compareMDHistoWorkspaces(const Mantid::DataObjects::MDHistoWorkspace_sptr &ws1,
                                const Mantid::DataObjects::MDHistoWorkspace_sptr &ws2);

  template <typename MDE, size_t nd>
  void compareMDEventWorkspaces(typename Mantid::DataObjects::MDEventWorkspace<MDE, nd>::sptr ws);

  template <typename T> void compare(T a, T b, const std::string &message);

  template <typename T> inline void compareTol(T a, T b, const std::string &message);

  template <typename MDE, size_t nd> void compare2Boxes(API::IMDNode *box1, API::IMDNode *box2, size_t ibox);

  Mantid::API::IMDWorkspace_sptr inWS2;

  /// Result string
  std::string m_result;

  /// Tolerance
  double m_tolerance = 0.0;

  /// Is CheckEvents true
  bool m_CheckEvents = true;

  bool m_CompareBoxID = true;
};

} // namespace MDAlgorithms
} // namespace Mantid
