.. _QtStandardItemTreeModelAdapter:

==============================
QtStandardItemTreeModelAdapter
==============================

The :code:`QtStandardItemTreeModelAdapter` is a wrapper around :code:`QStandardItemModel`
which helps to enforce the strong typing on :code:`QModelIndex`\ s. It eliminates some of the
boilerplate required when working with the model in the :doc:`../API/JobTreeView`, aiming to prevent
:code:`JobTreeView` and higher level classes from working directly with :code:`QStandardItem`\ s.

Its header also contains definitions for :code:`modelItemFromIndex` whose usage in its raw
form is discouraged outside of the implementation of :code:`QtStandardItemTreeModelAdapter`
but currently necessary in :code:`CellDelegate`.


Usage
#############################

The :code:`QtStandardItemTreeModelAdapter` is used when performing model manipulations
in the :code:`JobTreeView`.
