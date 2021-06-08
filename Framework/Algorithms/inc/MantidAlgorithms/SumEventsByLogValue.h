// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid {
namespace Algorithms {
/**
  Produces a table or single spectrum workspace containing the total summed
  events in the workspace
  as a function of a specified log.
*/
class MANTID_ALGORITHMS_DLL SumEventsByLogValue : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "SumEventsByLogValue"; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"FilterByLogValue"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Events"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Produces a single spectrum workspace containing the "
           "total summed events in the workspace as a function of a specified "
           "log.";
  }

  std::map<std::string, std::string> validateInputs() override;

private:
  void init() override;
  void exec() override;

  void createTableOutput(const Kernel::TimeSeriesProperty<int> *log);

  template <typename T> void createBinnedOutput(const Kernel::TimeSeriesProperty<T> *log);

  void filterEventList(const API::IEventList &eventList, const int minVal, const int maxVal,
                       const Kernel::TimeSeriesProperty<int> *log, std::vector<int> &Y);
  void addMonitorCounts(const API::ITableWorkspace_sptr &outputWorkspace, const Kernel::TimeSeriesProperty<int> *log,
                        const int minVal, const int maxVal);
  std::vector<std::pair<std::string, const Kernel::ITimeSeriesProperty *>> getNumberSeriesLogs();
  double sumProtonCharge(const Kernel::TimeSeriesProperty<double> *protonChargeLog,
                         const Kernel::TimeSplitterType &filter);

  DataObjects::EventWorkspace_const_sptr m_inputWorkspace; ///< The input workspace
  std::string m_logName;                                   ///< The name of the log to sum against
  std::vector<double> m_binningParams;                     ///< The optional binning parameters
};

} // namespace Algorithms
} // namespace Mantid
