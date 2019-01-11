// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
