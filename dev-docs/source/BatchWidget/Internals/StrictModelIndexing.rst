.. _StrictModelIndexing:

==============================
Model Indices in Job Tree View
==============================

The :doc:`../API/JobTreeView` uses :code:`RowLocation` objects in it's API as an abstraction over
:code:`QModelIndex`\ s, which are used internally to access :code:`QStandardItem`\ s from the 'models'
provided by Qt. As such, code which simply uses the :code:`JobTreeView` does not need to know anything
about :code:`QModelIndex`\ s.

MVC Models
^^^^^^^^^^

Sections of Qt such as :code:`QTreeView` are designed as an MVC framework rather than an MVP
framework, so working with some sections of it's API becomes difficult when using MVP. Models in
MVC are directly accessible from the view which is free to read the data directly from it. In MVP
however, data access is marshalled through the presenter, and both models and presenters are not
supposed to be coupled to any particular view implementation.

The Main Model
--------------

The :code:`JobTreeView` solves this problem by keeping an internal instance of
:code:`QStandardItemModel`, a Qt 'model' which fulfils Qt's requirements for a 'model' and acts
as the view state. We refer to this instance as the 'main model'.

Filtered Model
--------------

To take advantage of the filtering functionality offered by the :code:`QTreeView`, the
:code:`JobTreeView` also manages an instance of :code:`FilteredTreeModel`. This is a class derived from
:code:`QSortFilterProxyModel` - a filtered version of the 'main model'.

Strongly Typed Indexes
^^^^^^^^^^^^^^^^^^^^^^

The :code:`QModelIndex`\ s for the filtered model cannot be used to directly access items in the
main model. Likewise, the :code:`QModelIndex`\ s for the main model cannot be used to directly
access items in the filtered model. Indexes must be explicitly converted between the two spaces.

To make this less bug prone, the header file :code:`StrictQModelIndices.h` defines two types,
:code:`QModelIndexForMainModel` and :code:`QModelIndexForFilteredModel`. Functions which wish to
constrain the indices they accept and/or return can now make that explicit and use the type system
to catch errors.

Converting Between Index Spaces
-------------------------------

The filtered model holds a subset of the items in the main model, therefore:

* Conversion from an index for the filtered model to an index for the main model should
  always be successful.
* Conversion from an index for the main model to a valid index for the main model could be
  unsuccessful.

Given a :code:`QModelIndex`, in order to convert to a strongly typed variant you must know whether
it originated from the filtered model or the main model. Conversion to the appropriate
strongly typed variant is done via assertion using the functions :code:`fromMainModel` and
:code:`fromFilteredModel` provided by :code:`StrictQModelIndices.h`. These functions attempt to
check the assertion by requiring a reference to the model you are claiming the index is for and
comparing it with the pointer returned by calling :code:`.model()` on the model index. However this
is only a heuristic since the index could refer to a cell which has since been removed from the
filtered model due to a change of filter.

After the construction of the :code:`JobTreeView`, indices provided through :code:`QTreeView` APIs
are indices for the filtered model and must be mapped to the main model before being used to
modify it. :code:`RowLocation`\ s on the other hand always correspond with indices for the main
model.

Asserting that an index is for one space or the other as described above is not the same as mapping
from one space into the other. This is performed using the functions :code:`mapToMainModel` and
:code:`mapToFilteredModel` defined in :code:`JobTreeView` which internally call methods of the
filtered model.

Type Erasure
------------

Some functions are ambivalent to which model an index belongs to, a good example might be those in
:code:`QtBasicNavigation.h` - in order to still be able to use these functions without requiring them
all to be templates, the strict versions have a member function :code:`.untyped()` which returns the
internal plain old :code:`QModelIndex`.
