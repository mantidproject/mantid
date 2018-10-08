// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
} // namespace Kernel
} // namespace Mantid
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
*/
class ReflTransferStrategy {
public:
  virtual ~ReflTransferStrategy(){};

  /**
   * @param searchResults : A map where the keys are the runs and the values
   * the descriptions, location etc.
   * @param progress : Progress object to notify.
   * @param matchType : An enum defining how strictly to match runs against
   * the transfer criteria
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
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
