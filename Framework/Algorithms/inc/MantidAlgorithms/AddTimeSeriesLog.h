// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_ADDTIMESERIESLOG_H_
#define MANTID_ALGORITHMS_ADDTIMESERIESLOG_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {

namespace API {
class Run;
}

namespace Algorithms {

class DLLExport AddTimeSeriesLog : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Creates/updates a time-series log";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"AddSampleLog", "GetTimeSeriesLogInformation", "MergeLogs"};
  }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;

  /// Remove an existing log of the given name
  void removeExisting(API::MatrixWorkspace_sptr &logWS,
                      const std::string &name);
  /// Create or update the named log entry
  void createOrUpdate(API::Run &run, const std::string &name);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_ADDTIMESERIESLOG_H_ */
