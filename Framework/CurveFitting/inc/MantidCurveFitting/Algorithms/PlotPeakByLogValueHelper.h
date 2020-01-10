// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_PLOTPEAKBULOGVALUEHELPER_H_
#define MANTID_CURVEFITTING_PLOTPEAKBULOGVALUEHELPER_H_
#include "MantidAPI/DllConfig.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/Workspace_fwd.h"

#include <boost/optional.hpp>
#include <string>
#include <vector>

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

struct InputData {
  /// Constructor
  InputData(const std::string &nam, int ix, int p)
      : name(nam), i(ix), period(p) {}
  /// Copy constructor
  InputData(const InputData &data)
      : name(data.name), i(data.i), period(data.period), ws(data.ws) {}
  std::string name; ///< Name of a workspace or file
  int i;            ///< Workspace index of the spectra to fit
  int period;       ///< Period, needed if a file contains several periods
  API::MatrixWorkspace_sptr ws; ///< shared pointer to the workspace
};
/// Get a workspace
DLLExport std::vector<int>
getWorkspaceIndicesFromAxes(API::MatrixWorkspace_sptr, int workspaceIndex,
                            int spectrumNumber, double start, double end);
DLLExport boost::optional<API::Workspace_sptr>
getWorkspace(const std::string &name, int period);

/// Create a list of input workspace names
DLLExport std::vector<InputData> makeNames(std::string inputList,
                                           int default_wi, int default_spec);

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_PLOTPEAKBULOGVALUEHELPER_H_*/
