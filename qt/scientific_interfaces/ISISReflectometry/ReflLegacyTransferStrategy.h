// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_REFLLEGACYTRANSFERSTRATEGY_H
#define MANTID_ISISREFLECTOMETRY_REFLLEGACYTRANSFERSTRATEGY_H

#include "MantidKernel/System.h"
#include "ReflTransferStrategy.h"

namespace MantidQt {
namespace CustomInterfaces {

/** ReflLegacyTransferStrategy : Replicates the old Reflectometry UI's transfer
behaviour.
*/
class DLLExport ReflLegacyTransferStrategy : public ReflTransferStrategy {
public:
  TransferResults
  transferRuns(SearchResultMap &searchResults,
               Mantid::Kernel::ProgressBase &progress,
               const TransferMatch matchType = TransferMatch::Any) override;

  std::unique_ptr<ReflLegacyTransferStrategy> clone() const;

  bool knownFileType(const std::string &filename) const override;

private:
  ReflLegacyTransferStrategy *doClone() const override;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
