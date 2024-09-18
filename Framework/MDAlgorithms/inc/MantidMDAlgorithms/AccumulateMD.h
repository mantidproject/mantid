// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidMDAlgorithms/DllConfig.h"
#include <set>

namespace {}

namespace Mantid {
namespace API {
class IMDHistoWorkspace;
}

namespace MDAlgorithms {

/// Reduce the vector of input data to only data files and workspaces which can
/// be found
std::string MANTID_MDALGORITHMS_DLL filterToExistingSources(std::vector<std::string> &input_data,
                                                            std::vector<double> &psi, std::vector<double> &gl,
                                                            std::vector<double> &gs, std::vector<double> &efix);

/// Check if the named data source is an existing workspace or file
bool MANTID_MDALGORITHMS_DLL dataExists(const std::string &data_name);

/// Reduce the vector of input data to only data files and workspaces which are
/// not found in the vector of data currently in the workspace
std::string MANTID_MDALGORITHMS_DLL filterToNew(std::vector<std::string> &input_data,
                                                const std::vector<std::string> &current_data, std::vector<double> &psi,
                                                std::vector<double> &gl, std::vector<double> &gs,
                                                std::vector<double> &efix);

/// Check if the named data source is in the vector of data currently in the
/// workspace
bool appearsInCurrentData(const std::string &data_source, const std::vector<std::string> &current_data);

/// Return a vector of the names of files and workspaces which have been
/// previously added to the workspace
std::vector<std::string> getHistoricalDataSources(const API::WorkspaceHistory &ws_history,
                                                  const std::string &create_alg_name,
                                                  const std::string &accumulate_alg_name);

/// Extract names of data sources from workspace history and form a set of
/// historical data sources
void MANTID_MDALGORITHMS_DLL insertDataSources(const std::string &data_sources,
                                               std::unordered_set<std::string> &historical_data_sources);

/// Test if a file with the given full path name exists
bool fileExists(const std::string &filename);

/// Pad vector of parameters to given length
extern void MANTID_MDALGORITHMS_DLL padParameterVector(std::vector<double> &param_vector, const size_t grow_to_size);

/** AccumulateMD : Algorithm for appending new data to a MDHistoWorkspace
 */
class MANTID_MDALGORITHMS_DLL AccumulateMD : public API::DataProcessorAlgorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"MergeMD"}; }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;

  /// Use the CreateMD algorithm to create an MD workspace
  Mantid::API::IMDEventWorkspace_sptr createMDWorkspace(const std::vector<std::string> &data_sources,
                                                        const std::vector<double> &psi, const std::vector<double> &gl,
                                                        const std::vector<double> &gs, const std::vector<double> &efix,
                                                        const std::string &filename, const bool filebackend);

  std::map<std::string, std::string> validateInputs() override;
};

} // namespace MDAlgorithms
} // namespace Mantid
