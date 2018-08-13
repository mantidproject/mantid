#ifndef MANTID_ISISREFLECTOMETRY_REFLMEASURETRANSFERSTRATEGY_H_
#define MANTID_ISISREFLECTOMETRY_REFLMEASURETRANSFERSTRATEGY_H_

#include "DllConfig.h"
#include "ReflTransferStrategy.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Mantid {
namespace Kernel {
// Forward dec
class ICatalogInfo;
} // namespace Kernel
} // namespace Mantid

namespace MantidQt {
namespace CustomInterfaces {

// Forward dec
class ReflMeasurementItemSource;

/** ReflMeasureTransferStrategy : Transfer strategy that uses the measurement
  information
  from the loaded workspaces to complete the transfer.

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
class MANTIDQT_ISISREFLECTOMETRY_DLL ReflMeasureTransferStrategy
    : public ReflTransferStrategy {
public:
  ReflMeasureTransferStrategy(
      std::unique_ptr<Mantid::Kernel::ICatalogInfo> catInfo,
      std::unique_ptr<ReflMeasurementItemSource> measurementItemSource);

  ReflMeasureTransferStrategy(const ReflMeasureTransferStrategy &other);

  TransferResults
  transferRuns(SearchResultMap &searchResults,
               Mantid::Kernel::ProgressBase &progress,
               const TransferMatch matchType = TransferMatch::Any) override;

  std::unique_ptr<ReflMeasureTransferStrategy> clone() const;

  bool knownFileType(const std::string &filename) const override;

  ~ReflMeasureTransferStrategy() override;

private:
  ReflMeasureTransferStrategy *doClone() const override;

  /// Catalog information needed for transformations
  std::unique_ptr<Mantid::Kernel::ICatalogInfo> m_catInfo;

  /// Measurement source for loading.
  std::unique_ptr<ReflMeasurementItemSource> m_measurementItemSource;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_ISISREFLECTOMETRY_REFLMEASURETRANSFERSTRATEGY_H_ */
