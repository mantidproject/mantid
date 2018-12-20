#ifndef MANTID_MDALGORITHMS_CREATEMD_H_
#define MANTID_MDALGORITHMS_CREATEMD_H_

#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidMDAlgorithms/DllConfig.h"
namespace Mantid {
namespace MDAlgorithms {

void MANTID_MDALGORITHMS_DLL padParameterVector(
    std::vector<double> &param_vector, const size_t grow_to_size);

bool any_given(const std::vector<std::vector<double>> &params);

bool all_given(const std::vector<std::vector<double>> &params);

extern bool dataExists(const std::string &data_name);

/** CreateMD : This workflow algorithm creates MDWorkspaces in the Q3D, HKL
  frame using ConvertToMD

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_MDALGORITHMS_DLL CreateMD : public API::DataProcessorAlgorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"CreateMDWorkspace"};
  }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;

  /// Load data from file into a workspace
  Mantid::API::Workspace_sptr loadWs(const std::string &filename,
                                     const std::string &wsname);

  /// Add a sample log to a workspace
  void addSampleLog(Mantid::API::MatrixWorkspace_sptr workspace,
                    const std::string &log_name, double log_number);

  /// Set the goniometer values in a workspace
  void setGoniometer(Mantid::API::MatrixWorkspace_sptr workspace);

  /// Set the UB matrix in a workspace
  void setUB(Mantid::API::MatrixWorkspace_sptr workspace, double a, double b,
             double c, double alpha, double beta, double gamma,
             const std::vector<double> &u, const std::vector<double> &v);

  /// Convert a workspace to MDWorkspace
  Mantid::API::IMDEventWorkspace_sptr
  convertToMD(Mantid::API::Workspace_sptr workspace,
              const std::string &analysis_mode, bool in_place,
              const std::string &filebackend_filename, const bool filebackend,
              Mantid::API::IMDEventWorkspace_sptr out_mdws);

  /// Merge input workspaces
  Mantid::API::IMDEventWorkspace_sptr
  merge_runs(const std::vector<std::string> &to_merge);

  /// Add logs and convert to MDWorkspace for a single run
  Mantid::API::IMDEventWorkspace_sptr
  single_run(Mantid::API::MatrixWorkspace_sptr input_workspace,
             const std::string &emode, double efix, double psi, double gl,
             double gs, bool in_place, const std::vector<double> &alatt,
             const std::vector<double> &angdeg, const std::vector<double> &u,
             const std::vector<double> &v,
             const std::string &filebackend_filename, const bool filebackend,
             Mantid::API::IMDEventWorkspace_sptr out_mdws);

  /// Validate the algorithm's input properties
  std::map<std::string, std::string> validateInputs() override;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_CREATEMD_H_ */