// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_COMPAREMDWORKSPACES_H_
#define MANTID_MDALGORITHMS_COMPAREMDWORKSPACES_H_

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
  const std::string summary() const override {
    return "Compare two MDWorkspaces for equality.";
  }

  int version() const override;
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
  void doComparison();
  void compareMDGeometry(Mantid::API::IMDWorkspace_sptr ws1,
                         Mantid::API::IMDWorkspace_sptr ws2);
  void compareMDHistoWorkspaces(Mantid::DataObjects::MDHistoWorkspace_sptr ws1,
                                Mantid::DataObjects::MDHistoWorkspace_sptr ws2);

  template <typename MDE, size_t nd>
  void compareMDWorkspaces(
      typename Mantid::DataObjects::MDEventWorkspace<MDE, nd>::sptr ws);

  template <typename T> void compare(T a, T b, const std::string &message);

  template <typename T>
  inline void compareTol(T a, T b, const std::string &message);

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

#endif /* MANTID_MDALGORITHMS_COMPAREMDWORKSPACES_H_ */
