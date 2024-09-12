// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidAPI/DllConfig.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidCurveFitting/DllConfig.h"

#include <optional>
#include <string>
#include <vector>

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

struct InputSpectraToFit {
  /// Constructor
  InputSpectraToFit(const std::string &nam, int ix, int p) : name(nam), i(ix), period(p) {}
  /// Copy constructor
  InputSpectraToFit(const InputSpectraToFit &data) = default;

  std::string name;             ///< Name of a workspace or file
  int i;                        ///< Workspace index of the spectra to fit
  int period;                   ///< Period, needed if a file contains several periods
  API::MatrixWorkspace_sptr ws; ///< shared pointer to the workspace
};
/// Get a workspace
MANTID_CURVEFITTING_DLL std::vector<int> getWorkspaceIndicesFromAxes(API::MatrixWorkspace &ws, int workspaceIndex,
                                                                     int spectrumNumber, double start, double end);
MANTID_CURVEFITTING_DLL std::optional<API::Workspace_sptr> getWorkspace(const std::string &name, int period);

/// Create a list of input workspace names
MANTID_CURVEFITTING_DLL std::vector<InputSpectraToFit> makeNames(const std::string &inputList, int default_wi,
                                                                 int default_spec);

enum SpecialIndex {
  NOT_SET = -1,
  WHOLE_RANGE = -2,
};

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
