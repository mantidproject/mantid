// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace MDAlgorithms {

/** GetSpiceDataRawCountsFromMD : Export raw detectors' counts or sample log
  values from
    IMDEventWorkspaces from the output of algorithm ConvertSpiceDataToRealSpace.
*/
class DLLExport GetSpiceDataRawCountsFromMD : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "GetSpiceDataRawCountsFromMD"; }

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Get detectors' raw counts or sample environment log values from "
           "IMDEventWorkspace created from SPICE data file.";
  }

  /// Algorithm's version
  int version() const override { return (1); }

  /// Algorithm's category for identification
  const std::string category() const override { return "Diffraction\\ConstantWavelength"; }

private:
  /// Initialisation code
  void init() override;

  /// Execution code
  void exec() override;

  /// Export all detectors' counts for a run
  void exportDetCountsOfRun(const API::IMDEventWorkspace_const_sptr &datamdws,
                            const API::IMDEventWorkspace_const_sptr &monitormdws, const int runnumber,
                            std::vector<double> &vecX, std::vector<double> &vecY, std::string &xlabel,
                            std::string &ylabel, bool donormalize);

  /// Export a detector's counts accross all runs
  void exportIndividualDetCounts(const API::IMDEventWorkspace_const_sptr &datamdws,
                                 const API::IMDEventWorkspace_const_sptr &monitormdws, const int detid,
                                 std::vector<double> &vecX, std::vector<double> &vecY, std::string &xlabel,
                                 std::string &ylabel, const bool &donormalize);

  /// Export sample log values accross all runs
  void exportSampleLogValue(const API::IMDEventWorkspace_const_sptr &datamdws, const std::string &samplelogname,
                            std::vector<double> &vecX, std::vector<double> &vecY, std::string &xlabel,
                            std::string &ylabel);

  /// Get detectors' counts
  void getDetCounts(const API::IMDEventWorkspace_const_sptr &mdws, const int &runnumber, const int &detid,
                    std::vector<double> &vecX, std::vector<double> &vecY, bool formX);

  /// Get sample log values
  void getSampleLogValues(const API::IMDEventWorkspace_const_sptr &mdws, const std::string &samplelogname,
                          const int runnumber, std::vector<double> &vecSampleLog);

  /// Create output workspace
  API::MatrixWorkspace_sptr createOutputWorkspace(const std::vector<double> &vecX, const std::vector<double> &vecY,
                                                  const std::string &xlabel, const std::string &ylabel);
};

} // namespace MDAlgorithms
} // namespace Mantid
