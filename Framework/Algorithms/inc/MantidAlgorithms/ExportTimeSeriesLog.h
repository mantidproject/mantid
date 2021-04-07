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
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid {
namespace Algorithms {

/** ExportTimeSeriesLog : Read a TimeSeries log and return some information
  required by users.

  @date 2011-12-22
*/
class MANTID_ALGORITHMS_DLL ExportTimeSeriesLog : public API::Algorithm {
public:
  const std::string name() const override { return "ExportTimeSeriesLog"; };

  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"GetTimeSeriesLogInformation"}; }

  const std::string category() const override { return "Diffraction\\DataHandling;Events\\EventFiltering"; };

  /// Summary of algorithms purpose
  const std::string summary() const override { return "Read a TimeSeries log and return information"; }

private:
  API::MatrixWorkspace_sptr m_inputWS;
  API::MatrixWorkspace_sptr m_outWS;

  std::vector<int64_t> mSETimes;
  std::vector<double> mSEValues;

  Types::Core::DateAndTime mRunStartTime;
  Types::Core::DateAndTime mFilterT0;
  Types::Core::DateAndTime mFilterTf;

  void init() override;

  void exec() override;

  bool calculateTimeSeriesRangeByTime(std::vector<Types::Core::DateAndTime> &vec_times, const double &rel_start_time,
                                      size_t &i_start, const double &rel_stop_time, size_t &i_stop,
                                      const double &time_factor);

  void exportLog(const std::string &logname, const std::string &timeunit, const double &starttime,
                 const double &stoptime, const bool exportepoch, bool outputeventws, int numentries,
                 bool cal_first_deriv);

  void setupEventWorkspace(const size_t &start_index, const size_t &stop_index, int numentries,
                           std::vector<Types::Core::DateAndTime> &times, std::vector<double> values,
                           const bool &epochtime);

  void setupWorkspace2D(const size_t &start_index, const size_t &stop_index, int numentries,
                        std::vector<Types::Core::DateAndTime> &times, std::vector<double> values, const bool &epochtime,
                        const double &timeunitfactor, size_t nspec);

  void calculateFirstDerivative(bool is_event_ws);

  void setupMetaData(const std::string &log_name, const std::string &time_unit, const bool &export_epoch);
};

} // namespace Algorithms
} // namespace Mantid
