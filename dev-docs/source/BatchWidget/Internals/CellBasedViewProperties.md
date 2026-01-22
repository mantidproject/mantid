# Cell Based View Properties

As described in `../API/JobTreeView`, per-cell view properties can be
retrieved as an instance of the `Cell` class. The mechanics behind this
are implemented in `CellStandardItem.h`.

## Implementation

This header contains an enumeration of the per-cell attributes which are
not members of `QStandardItem`. This enumeration starts at
`Qt::UserRole + 1`. Qt has a built in mechanism for extending the
per-item view attributes which we use here.

Two significant functions defined in this file are
`applyCellPropertiesToItem` and `extractCellPropertiesFromItem`. These
functions are used to save the property values from the `Cell` instance
onto the `QStandardItem` and later retrieve them.

## A note on adding new properties

When adding new properties to the `Cell` class and by extension the
`CellUserRoles` enumeration it is important to consider if the new
property is indeed a **view** property or whether it should instead be
part of your model. The `JobTreeView` is designed with this use case in
mind and has mechanisms to allow you to synchronise the state of the
view with the state of your model.
