// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/Run.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/**
    Used to insert a single string into the sample in a workspace

    Required Properties:
    <UL>
    <LI> Workspace -The log data will be added to this workspace</LI>
    <LI> LogName -The named entry will be accessible through this name</LI>
    Optional property:
    <LI> LogText -The log data</LI>
    </UL>

    Workspaces contain information in logs. Often these detail what happened
    to the sample during the experiment. This algorithm allows one named log
    to be entered.
*/
class MANTID_ALGORITHMS_DLL AddSampleLog final : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "AddSampleLog"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Used to insert a value into the sample logs in a workspace."; }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"AddSampleLogMultiple", "AddTimeSeriesLog", "DeleteLog", "LoadLog"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Logs"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  void addStringLog(API::Run &theRun, const std::string &propName, const std::string &propValue,
                    const std::string &propUnit);

  void addTimeSeriesProperty(API::Run &run_obj, const std::string &prop_name, const std::string &prop_value,
                             const std::string &prop_unit, const std::string &prop_number_type);

  void addSingleValueProperty(API::Run &theRun, const std::string &propName, const std::string &propValue,
                              const std::string &propUnit, const std::string &propNumberType);

  /// set the time series property's entries to the newly added
  /// TimeSeriesProperty
  void setTimeSeriesData(const API::Run &run_obj, const std::string &property_name, bool value_is_int);

  /// get run start time
  Types::Core::DateAndTime getRunStart(const API::Run &run_obj);

  /// get value vector of the integer TimeSeriesProperty entries
  std::vector<int> getIntValues(const API::MatrixWorkspace_const_sptr &dataws, int workspace_index);

  /// get value vector of the double TimeSeriesProperty entries
  std::vector<double> getDblValues(const API::MatrixWorkspace_const_sptr &dataws, int workspace_index);

  /// get the vector of times of the TimeSeriesProperty entries
  std::vector<Types::Core::DateAndTime> getTimes(const API::MatrixWorkspace_const_sptr &dataws, int workspace_index,
                                                 bool is_epoch, bool is_second, const API::Run &run_obj);

  /// get meta data from input workspace or user input
  void getMetaData(const API::MatrixWorkspace_const_sptr &dataws, bool &epochtime, std::string &timeunit);
};

} // namespace Algorithms
} // namespace Mantid
