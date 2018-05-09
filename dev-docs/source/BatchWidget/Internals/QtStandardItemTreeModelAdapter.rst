.. _QtStandardItemTreeModelAdapter:

==============================
QtStandardItemTreeModelAdapter
==============================

The :code:`QtStandardItemTreeModelAdapter` is a wrapper around :code:`QStandardItemModel`
it helps to enforce the strong typing on :code:`QModelIndex`\ s eliminates some of the boilerplate
required when working with the model in the :doc:`../API/JobTreeView`, aiming to prevent
:code:`JobTreeView` and higher level classes from working directly with :code:`QStandardItem`\ s.

It's header also contains definitions for :code:`modelItemFromIndex` who's usage in it's raw
form is discouraged outside of the implementation of :code:`QtStandardItemTreeModelAdapter`
but currently necessary in :code:`CellDelegate`.


Usage
#############################

The :code:`QtStandardItemTreeModelAdapter` is currently used when performing model manipulations
in the :code:`JobTreeView`.
