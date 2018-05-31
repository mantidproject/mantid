#include <gmock/gmock.h>
#include "MantidQtWidgets/Common/Batch/IJobTreeView.h"
#include "MantidKernel/WarningSuppressions.h"

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

GCC_DIAG_OFF_SUGGEST_OVERRIDE

class EXPORT_OPT_MANTIDQT_COMMON MockJobTreeView : public IJobTreeView {
public:
  void filterRowsBy(std::unique_ptr<RowPredicate> predicate) override {
    filterRowsBy(predicate.get());
  }
  MOCK_METHOD1(filterRowsBy, void(RowPredicate *));

  MOCK_METHOD0(resetFilter, void());
  MOCK_CONST_METHOD0(hasFilter, bool());

  MOCK_METHOD1(subscribe, void(JobTreeViewSubscriber &));

  MOCK_METHOD3(insertChildRowOf,
               RowLocation(RowLocation const &, int, std::vector<Cell> const &));
  MOCK_METHOD2(insertChildRowOf, RowLocation(RowLocation const &, int));
  MOCK_METHOD1(appendChildRowOf, RowLocation(RowLocation const &));
  MOCK_METHOD2(appendChildRowOf,
               RowLocation(RowLocation const &, std::vector<Cell> const &));

  MOCK_METHOD1(removeRowAt, void(RowLocation const &));
  MOCK_METHOD1(removeRows, void(std::vector<RowLocation>));
  MOCK_CONST_METHOD1(isOnlyChildOfRoot, bool(RowLocation const &));

  MOCK_METHOD2(replaceRows,
               void(std::vector<RowLocation>, std::vector<Subtree>));

  MOCK_METHOD2(appendSubtreesAt,
               void(RowLocation const &, std::vector<Subtree>));
  MOCK_METHOD2(appendSubtreeAt,
               void(RowLocation const &parent, Subtree const &subtree));
  MOCK_METHOD2(replaceSubtreeAt, void(RowLocation const&, Subtree const&));
  MOCK_METHOD3(insertSubtreeAt, void(RowLocation const&, int, Subtree const&));

  MOCK_METHOD0(clearSelection, void());
  MOCK_METHOD0(expandAll, void());
  MOCK_METHOD0(collapseAll, void());
  MOCK_CONST_METHOD1(cellsAt, std::vector<Cell>(RowLocation const&));
  MOCK_METHOD2(setCellsAt, void(RowLocation const&, std::vector<Cell> const& rowText));

  MOCK_CONST_METHOD2(cellAt, Cell(RowLocation, int));
  MOCK_METHOD3(setCellAt, void(RowLocation, int, Cell const&));

  MOCK_CONST_METHOD0(selectedRowLocations, std::vector<RowLocation>());
  MOCK_CONST_METHOD0(selectedSubtrees, boost::optional<std::vector<Subtree>>());
  MOCK_CONST_METHOD0(selectedSubtreeRoots, boost::optional<std::vector<RowLocation>>());
  MOCK_CONST_METHOD0(deadCell, Cell());
};

GCC_DIAG_ON_SUGGEST_OVERRIDE
}
}
}
