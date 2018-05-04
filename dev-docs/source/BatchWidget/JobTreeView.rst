.. _JobTreeView:

=============
Job Tree View
=============

The :code:`JobTreeView` component provides an MVP style view interface for a hierarchical table
with a spreadsheet-like appearance for configuring and indicating the status of multiple (batch)
reduction jobs. It is currently used to implement the tree component of the BatchWidget.

API Concepts
############

Row Location
^^^^^^^^^^^^

A row location is the notion of an individual row's position within the table. Since the
table is unlike a traditional spreadsheet in that it is hierarchical the table is more like a
tree of rows where all non-leaf nodes can have any number of children.

In practice some interfaces, such as Reflectometry are likely to want to constrain the
number of children or depth of the tree, and the batch widget has mechanisms for performing this.

Currently a row location is represented as a path from the root node to the row node in question,
this is actualised in the :code:`RowLocation` class which contains an ordered list of integers where
each element represents the index of the next node in the path relative to it's predecessor in the
path. Some example nodes, their corresponding paths, and their representations are illustrated in
the diagram below.

.. image::  ../images/row_location_path.svg
   :align: center
   :width: 800px

Equality over :code:`RowLocation` objects is based on the equality of their paths. The other relational
operators have a definition based on a lexicographical comparison such that sorting a range of
:code:`RowLocation`\ s puts them in the same order as they would appear in the table. As demonstrated by
the code below.

.. code-block:: c++

   auto items = std::vector<RowLocation>({
     RowLocation({2}),
     RowLocation({0}),
     RowLocation({1, 1}),
     RowLocation({1})
   });

   std::sort(items.begin(), items.end());

   auto expectedItems = std::vector<RowLocation>({
     RowLocation({0}),
     RowLocation({1}),
     RowLocation({1, 1}),
     RowLocation({2})
   });

   assert(expectedItems == items);

Dealing With Cells
------------------

Each row in the table can have 0..N cells. When interacting with the :code:`JobTreeView` we
sometimes need to be able to address and change the properties of an individual cell. To do this
we use both a :code:`RowLocation` and a column index, usually actualized as an :code:`int`.


Subtrees
^^^^^^^^

As previously illustrated the conceptual model for the API is a tree of tables. However
initially this model presents some challenges when you think about how to represent a
user's selection while preserving the tree structure. This is however necessary in order
to support presenters which wish to have sensible behaviour for actions such as copy and
paste.

A subtree in this context refers to a set of one or more nodes within the tree where if the set has
a size greater than one, each node is directly connected to at least one other node in the set.
An example of a set of nodes which meets this constraint and a set of nodes which does not are
illustrated in blue in the diagram below.

.. image::  ../images/subtree.svg
   :align: center
   :width: 800px

The :code:`Subtree` type used to represent this concept in the API is defined in the header
:code:`Row.h`. Refer to the documentation for the component :doc:`ExtractSubtrees` for more detail
on the internal representation of a subtree in this API.


Notification
^^^^^^^^^^^^

:code:`JobTreeViewSubscriber` is the mechanism by which the JobTreeView communicates events such as
key presses to the presenter in an MVP setup. This interface is also implemented by
:code:`JobTreeViewSignalAdapter` which makes it easy to use a signals and slots when writing a GUI
from python.

Due to the interactive nature of some events (such as row insertion, cell modification and filter resets),
notification does not happen until after said event has taken place and the view has
already been updated. Therefore, if a presenter determines that said action is on-reflection invalid
it will be required to call a method which updates the view and rolls back the action.
This is illustrated in the depth limit example below.

Other events (those who's notification method name ends with :code:`Requested`) require the presenter
to update the view and/or model and so the notification happens before the view has been updated.

.. warning::
   After creating a :code:`JobTreeView` it is important to call the :code:`subscribe` method passing in
   the subscriber prior to calling any other methods, failure to do so may result in undefined behavior.

Usage Examples
##############

Initializing a JobTreeView
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: py

   from mantidqtpython import MantidQt

   def empty_cell():
     return MantidQt.MantidWidgets.Batch.Cell("")

   # Inside the parent view
   def setup(self):
     self.table = MantidQt.MantidWidgets.Batch.JobTreeView(
       ["Column 1", "Column 2"], # The table column headings
       empty_cell(), # The default style and content for new 'empty' cells.
       self # The parent QObject.
       )
     self.table_signals = # The signal adapter subscribes to events from the table and
                          # emits signals whenever it is notified.
       MantidQt.MantidWidgets.Batch.JobTreeViewSignalAdapter(self.table, self)

     self.table.appendChildRowOf(row([]), [cell("Value for Column A"), cell("Value for Column B")])


.. code-block:: c++

  #include "MantidQtWidgets/Common/Batch/JobTreeView.h"

  using namespace MantidWidgets::Common::Batch;

  // Inside the parent view constructor
  m_treeView = new JobTreeView(
    {"Heading 1", "Heading 2"}, // The table column headings.
    Cell(""), // The default style and content for the new 'empty' cells.
    this // The parent QObject
    );
  m_treeViewSignals = // JobTreeViewSignalAdapter is also available from C++
                      // Constructing a signal adapter with the view implicitly calls subscribe.
    new JobTreeViewSignalAdapter(*m_treeView, this);
  m_treeView->appendChildRowOf(RowLocation(), {Cell("Value for Column A"), Cell("Value for Column B")})

Initializing a JobTreeView with your own subscriber
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c++

  #include "MantidQtWidgets/Common/Batch/JobTreeView.h"

  using namespace MantidWidgets::Common::Batch;

  class SimplePresenter : public JobTreeViewSubscriber {
  public:
    SimplePresenter(JobTreeView* view) : m_view(view) {
      m_view->subscribe(this); // Since we aren't using signal adapter
                               // we must remember the call to subscribe.
    }

    void notifyCellChanged(RowLocation const &itemIndex, int column,
                           std::string const &newValue) override { /* ... */ }
    void notifyRowInserted(RowLocation const &newRowLocation) override { /* ... */ }
    void notifyRemoveRowsRequested(std::vector<RowLocation> const &locationsOfRowsToRemove) override { /* ... */ }
    void notifyCopyRowsRequested() overrride { /* ... */ }
    void notifyPasteRowsRequested() override { /* ... */}
    void notifyFilterReset() override { /* ... */ }

  private:
    JobTreeView* m_view;
  };

  // Elsewhere - Inside initialization
  m_treeView = new JobTreeView(
    {"Heading 1", "Heading 2"}, // The table column headings.
    Cell(""), // The default style and content for the new 'empty' cells.
    this // The parent QObject
    );
  m_childPresenter = SimplePresenter(m_treeView);

Limiting the depth of the tree
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: py

   from mantidqtpython import MantidQt

   def empty_cell():
     return MantidQt.MantidWidgets.Batch.Cell("")

   # Inside the parent view
   def setup(self):
     self.table = MantidQt.MantidWidgets.Batch.JobTreeView(
       ["Column 1", "Column 2"], empty_cell(), self)
     self.table_signals =
       MantidQt.MantidWidgets.Batch.JobTreeViewSignalAdapter(self.table, self)

     self.table_signals.rowInserted.connect(self.on_row_inserted)
     # The rowInserted signal is fired every time a user inserts a row.
     # It is NOT fired if we manually insert a row.

   def on_row_inserted(self, rowLocation):
     if rowLocation.depth() > 2: # If the depth is more than two then
                                 # we can safely 'rollback' the insertion.
       self.table.removeRowAt(rowLocation)
