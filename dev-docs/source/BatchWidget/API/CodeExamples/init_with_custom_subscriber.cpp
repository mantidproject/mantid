#include "MantidQtWidgets/Common/Batch/JobTreeView.h"

using namespace MantidWidgets::Common::Batch;

class SimplePresenter : public JobTreeViewSubscriber {
public:
  SimplePresenter(JobTreeView *view) : m_view(view) {
    m_view->subscribe(this); // Since we aren't using signal adapter
                             // we must remember the call to subscribe.
  }

  void notifyCellChanged(RowLocation const &itemIndex, int column,
                         std::string const &newValue) override {}
  void notifyRowInserted(RowLocation const &newRowLocation) override {}
  void notifyRemoveRowsRequested(
      std::vector<RowLocation> const &locationsOfRowsToRemove) override {}
  void notifyCopyRowsRequested() override {}
  void notifyPasteRowsRequested() override {}
  void notifyFilterReset() override {}

private:
  JobTreeView *m_view;
};

// Elsewhere - Inside initialization
m_treeView = new JobTreeView(
    {"Heading 1", "Heading 2"}, // The table column headings.
    Cell(""), // The default style and content for the new 'empty' cells.
    this      // The parent QObject
    );
m_childPresenter = SimplePresenter(m_treeView);
