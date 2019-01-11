// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_MERGELOGS_H_
#define MANTID_DATAHANDLING_MERGELOGS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/System.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid {
namespace DataHandling {

/** Merge2WorkspaceLogs : TODO: DESCRIPTION

  @date 2011-12-15
*/
class DLLExport Merge2WorkspaceLogs : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "MergeLogs"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Merge 2 TimeSeries logs in a given Workspace.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"AddTimeSeriesLog"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Logs"; }

private:
  void init() override;

  void exec() override;

  void mergeLogs(std::string ilogname1, std::string ilogname2,
                 std::string ologname, bool resetlogvalue, double logvalue1,
                 double logvalue2);

  Kernel::TimeSeriesProperty<double> *getTimeSeriesLog(std::string logname);

  API::MatrixWorkspace_sptr matrixWS;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_MERGELOGS_H_ */
