// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid {
namespace Algorithms {

/** GetTimeSeriesLogInformation : Read a TimeSeries log and return some
  information required by users.

  @date 2011-12-22
*/
class MANTID_ALGORITHMS_DLL GetTimeSeriesLogInformation final : public API::Algorithm {
public:
  GetTimeSeriesLogInformation();

  const std::string name() const override { return "GetTimeSeriesLogInformation"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Get information from a TimeSeriesProperty log."; }

  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"AddSampleLogMultiple"}; }
  const std::string category() const override { return "Diffraction\\Utility;Events\\EventFiltering"; }

private:
  API::MatrixWorkspace_sptr m_dataWS;

  Types::Core::DateAndTime mRunStartTime;
  Types::Core::DateAndTime mFilterT0;
  Types::Core::DateAndTime mFilterTf;

  std::map<std::string, std::size_t> m_intInfoMap;
  std::map<std::string, double> m_dblInfoMap;

  Kernel::TimeSeriesProperty<double> *m_log;
  std::vector<Types::Core::DateAndTime> m_timeVec;
  std::vector<double> m_valueVec;

  Types::Core::DateAndTime m_starttime;
  Types::Core::DateAndTime m_endtime;

  bool m_ignoreNegativeTime;

  void init() override;

  void exec() override;

  void examLog(std::string logname, std::string outputdir);

  void generateCalibrationFile();

  void processTimeRange();

  /// Calcualte the distribution of delta T in time stamps
  DataObjects::Workspace2D_sptr calDistributions(std::vector<Types::Core::DateAndTime> timevec, double stepsize);

  void exportLog(API::MatrixWorkspace_sptr ws, std::vector<Types::Core::DateAndTime> abstimevec, double dts);

  void setupEventWorkspace(int numentries, std::vector<Types::Core::DateAndTime> &times, std::vector<double> values);

  void setupWorkspace2D(int numentries, std::vector<Types::Core::DateAndTime> &times, std::vector<double> values);

  void execQuickStatistics();

  void exportErrorLog(const API::MatrixWorkspace_sptr &ws, std::vector<Types::Core::DateAndTime> &abstimevec,
                      double dts);

  void checkLogValueChanging(std::vector<Types::Core::DateAndTime> &timevec, std::vector<double> &values, double delta);

  void checkLogBasicInforamtion();

  /// Generate statistic information table workspace
  DataObjects::TableWorkspace_sptr generateStatisticTable();

  Types::Core::DateAndTime getAbsoluteTime(double abstimens);

  Types::Core::DateAndTime calculateRelativeTime(double deltatime);
};

} // namespace Algorithms
} // namespace Mantid
