#ifndef MANTID_CUSTOMINTERFACES_MOCKRUNSTABLEVIEW_H_
#define MANTID_CUSTOMINTERFACES_MOCKRUNSTABLEVIEW_H_
#include "../../../ISISReflectometry/GUI/RunsTable/IRunsTableView.h"
#include <gmock/gmock.h>
#include "MantidQtWidgets/Common/Batch/MockJobTreeView.h"
#include "MantidKernel/WarningSuppressions.h"

GCC_DIAG_OFF_SUGGEST_OVERRIDE

namespace MantidQt {
namespace CustomInterfaces {

class MockRunsTableView : public IRunsTableView {
public:
  MOCK_METHOD1(subscribe, void(RunsTableViewSubscriber *));
  MOCK_METHOD1(setProgress, void(int));
  MOCK_METHOD0(jobs, MantidQt::MantidWidgets::Batch::IJobTreeView &());
  MOCK_METHOD0(resetFilterBox, void());

  MOCK_METHOD0(invalidSelectionForCopy, void());
  MOCK_METHOD0(invalidSelectionForPaste, void());
  MOCK_METHOD0(invalidSelectionForCut, void());

  MOCK_METHOD0(mustSelectRow, void());
  MOCK_METHOD0(mustSelectGroup, void());
  MOCK_METHOD0(mustNotSelectGroup, void());
  MOCK_METHOD0(mustSelectGroupOrRow, void());
};
}
}
GCC_DIAG_ON_SUGGEST_OVERRIDE
#endif // MANTID_CUSTOMINTERFACES_MOCKRUNSTABLEVIEW_H_
