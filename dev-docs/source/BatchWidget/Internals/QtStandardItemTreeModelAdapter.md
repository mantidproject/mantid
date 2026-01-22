# QtStandardItemTreeModelAdapter

The `QtStandardItemTreeModelAdapter` is a wrapper around
`QStandardItemModel` which helps to enforce the strong typing on
`QModelIndex`s. It eliminates some of the boilerplate required when
working with the model in the `../API/JobTreeView`, aiming to prevent
`JobTreeView` and higher level classes from working directly with
`QStandardItem`s.

Its header also contains definitions for `modelItemFromIndex` whose
usage in its raw form is discouraged outside of the implementation of
`QtStandardItemTreeModelAdapter` but currently necessary in
`CellDelegate`.

## Usage

The `QtStandardItemTreeModelAdapter` is used when performing model
manipulations in the `JobTreeView`.
