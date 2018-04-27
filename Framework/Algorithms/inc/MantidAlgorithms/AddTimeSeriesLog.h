#ifndef MANTID_ALGORITHMS_ADDTIMESERIESLOG_H_
#define MANTID_ALGORITHMS_ADDTIMESERIESLOG_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {

namespace API {
class Run;
}

namespace Algorithms {

/**
  Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
