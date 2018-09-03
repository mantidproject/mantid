#ifndef MANTID_ISISREFLECTOMETRY_REFLLEGACYTRANSFERSTRATEGY_H
#define MANTID_ISISREFLECTOMETRY_REFLLEGACYTRANSFERSTRATEGY_H

#include "MantidKernel/System.h"
#include "ReflTransferStrategy.h"

namespace MantidQt {
namespace CustomInterfaces {

/** ReflLegacyTransferStrategy : Replicates the old Reflectometry UI's transfer
behaviour.

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
