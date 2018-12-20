#ifndef MANTID_MDALGORITHMS_GETSPICEDATARAWCOUNTSFROMMD_H_
#define MANTID_MDALGORITHMS_GETSPICEDATARAWCOUNTSFROMMD_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace MDAlgorithms {

/** GetSpiceDataRawCountsFromMD : Export raw detectors' counts or sample log
  values from
    IMDEventWorkspaces from the output of algorithm ConvertSpiceDataToRealSpace.

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
class DLLExport GetSpiceDataRawCountsFromMD : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override {
    return "GetSpiceDataRawCountsFromMD";
  }

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Get detectors' raw counts or sample environment log values from "
           "IMDEventWorkspace created from SPICE data file.";
  }

  /// Algorithm's version
  int version() const override { return (1); }

  /// Algorithm's category for identification
  const std::string category() const override {
    return "Diffraction\\ConstantWavelength";
  }

private:
  /// Initialisation code
  void init() override;

  /// Execution code
  void exec() override;

  /// Export all detectors' counts for a run
  void exportDetCountsOfRun(API::IMDEventWorkspace_const_sptr datamdws,
                            API::IMDEventWorkspace_const_sptr monitormdws,
                            const int runnumber, std::vector<double> &vecX,
                            std::vector<double> &vecY, std::string &xlabel,
                            std::string &ylabel, bool donormalize);

  /// Export a detector's counts accross all runs
  void exportIndividualDetCounts(API::IMDEventWorkspace_const_sptr datamdws,
                                 API::IMDEventWorkspace_const_sptr monitormdws,
                                 const int detid, std::vector<double> &vecX,
                                 std::vector<double> &vecY, std::string &xlabel,
                                 std::string &ylabel, const bool &donormalize);

  /// Export sample log values accross all runs
  void exportSampleLogValue(API::IMDEventWorkspace_const_sptr datamdws,
                            const std::string &samplelogname,
                            std::vector<double> &vecX,
                            std::vector<double> &vecY, std::string &xlabel,
                            std::string &ylabel);

  /// Get detectors' counts
  void getDetCounts(API::IMDEventWorkspace_const_sptr mdws,
                    const int &runnumber, const int &detid,
                    std::vector<double> &vecX, std::vector<double> &vecY,
                    bool formX);

  /// Get sample log values
  void getSampleLogValues(API::IMDEventWorkspace_const_sptr mdws,
                          const std::string &samplelogname, const int runnumber,
                          std::vector<double> &vecSampleLog);

  /// Create output workspace
  API::MatrixWorkspace_sptr
  createOutputWorkspace(const std::vector<double> &vecX,
                        const std::vector<double> &vecY,
                        const std::string &xlabel, const std::string &ylabel);
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_GETSPICEDATARAWCOUNTSFROMMD_H_ */
