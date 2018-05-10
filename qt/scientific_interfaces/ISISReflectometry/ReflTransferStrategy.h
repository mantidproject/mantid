#ifndef MANTID_ISISREFLECTOMETRY_REFLTRANSFERSTRATEGY_H
#define MANTID_ISISREFLECTOMETRY_REFLTRANSFERSTRATEGY_H

#include <map>
#include <string>
#include <vector>

#include "TransferResults.h"

namespace Mantid {
namespace Kernel {
// Forward dec
class ProgressBase;
}
}
namespace MantidQt {
namespace CustomInterfaces {

/**
* The SearchResult struct provides search metadata information
*/
struct SearchResult {
  SearchResult() {}
  SearchResult(const std::string &desc, const std::string &loc)
      : description(desc), location(loc) {}
  std::string description;
  std::string location;
  std::string issues;
};

/// Helper typdef for map of SearchResults keyed by run
using SearchResultMap = std::map<std::string, SearchResult>;

// This enum defines different strictness level when looking up
// rows to transfer
enum class TransferMatch : unsigned int {
  Any,        // any that match the regex
  ValidTheta, // any that match and have a valid theta value
  Strict      // only those that exactly match all parts of the regex
};

/** ReflTransferStrategy : Provides an stratgegy for transferring runs from
search results to a format suitable for processing.

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
class ReflTransferStrategy {
public:
  virtual ~ReflTransferStrategy(){};

  /**
   * @param searchResults : A map where the keys are the runs and the values
   * the descriptions, location etc.
   * @param progress : Progress object to notify.
   * @returns A vector of maps where each map represents a row,
   * with Keys matching Column headings and Values matching the row entries
   * for those columns
   */
  virtual TransferResults transferRuns(SearchResultMap &searchResults,
                                       Mantid::Kernel::ProgressBase &progress,
                                       const TransferMatch matchType) = 0;

  std::unique_ptr<ReflTransferStrategy> clone() const {
    return std::unique_ptr<ReflTransferStrategy>(doClone());
  }

  /**
  * Filter. Individual transfer strategies may veto file types they
  * do not understand and will be unable to extract metadata for.
  * @param filename : Full name of the file.
  * @return True only if the file type is known.
  */
  virtual bool knownFileType(const std::string &filename) const = 0;

private:
  /**
   * Virtual constructor
   * @return : A new instance of this.
   */
  virtual ReflTransferStrategy *doClone() const = 0;
};
}
}

#endif
