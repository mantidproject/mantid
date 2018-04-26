#ifndef MANTIDQTMANTIDWIDGETS_EXTRACTSUBTREES_H_
#define MANTIDQTMANTIDWIDGETS_EXTRACTSUBTREES_H_
#include "MantidQtWidgets/Common/Batch/AssertOrThrow.h"
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include "MantidQtWidgets/Common/Batch/Row.h"
#include "MantidQtWidgets/Common/DllOption.h"
#include <boost/optional.hpp>
#include <string>
#include <vector>
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

struct RecursiveSubtreeExtractionResult {
  using RowLocationConstIterator =
      typename std::vector<RowLocation>::const_iterator;
  using RowDataConstIterator =
      typename std::vector<std::vector<Cell>>::const_iterator;
  RecursiveSubtreeExtractionResult();
  RecursiveSubtreeExtractionResult(
      bool shouldContinue,
      std::pair<RowLocationConstIterator, RowDataConstIterator> const &
          currentPosition);

  bool shouldNotContinue;
  bool regionHasGaps;
  std::pair<RowLocationConstIterator, RowDataConstIterator> currentPosition;
};

class EXPORT_OPT_MANTIDQT_COMMON ExtractSubtrees {
public:
  using RowLocationConstIterator =
      typename std::vector<RowLocation>::const_iterator;
  using RowDataConstIterator =
      typename std::vector<std::vector<Cell>>::const_iterator;

  boost::optional<std::vector<Subtree>>
  operator()(std::vector<RowLocation> region,
             std::vector<std::vector<Cell>> regionData);
  bool isChildOfPrevious(RowLocation const &location) const;
  bool isSiblingOfPrevious(RowLocation const &location) const;
  bool isCorrectDepthForChild(int parentDepth, int maybeChildDepth);
  RecursiveSubtreeExtractionResult const
  finishedSubtree(RowLocationConstIterator currentRow,
                  RowDataConstIterator currentRowData);
  RecursiveSubtreeExtractionResult const
  continueOnLevelAbove(RowLocationConstIterator currentRow,
                       RowDataConstIterator currentRowData);

  RecursiveSubtreeExtractionResult const reportUnsuitableTree();
  bool currentIsInDifferentSubtree(int depthOfCurrentRow,
                                   RowLocation const &rootRelativeToTree);

  RecursiveSubtreeExtractionResult extractSubtreeRecursive(
      Subtree &subtree, RowLocation const &rootRelativeToTree,
      RowLocation parent, int minDepth, RowLocationConstIterator currentRow,
      RowLocationConstIterator endRow, RowDataConstIterator currentRowData);

private:
  bool previousWasRoot;
  RowLocation previousNode;
};

} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
#endif
