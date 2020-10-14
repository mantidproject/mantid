# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantidqt package.


class MarkedColumns:
    X_LABEL = "[X{}]"
    Y_LABEL = "[Y{}]"
    Y_ERR_LABEL = "[Y{}_YErr]"

    def __init__(self):
        self.as_x = []
        self.as_y = []
        self.as_y_err = []

    def _remove(self, col_index, remove_from):
        """
        Remove the column index from all lists
        :param col_index: The column index to be removed
        :type remove_from: list[list[Union[int, ErrorColumn]]]
        :param remove_from: List of lists from which the column index will be removed
        :return:
        """
        removed_cols=[]
        for list in remove_from:
            try:
                # remove all (for error cols there could be more than one match)
                for _ in range(list.count(col_index)):
                    list_index = list.index(col_index)
                    col_to_remove = list.pop(list_index)
                    if col_to_remove not in removed_cols:
                        removed_cols.append(col_to_remove)
            except ValueError:
                # column not in this list, but might be in another one so we continue the loop
                continue
        return removed_cols

    def add_x(self, col_index):
        removed_items = self._remove(col_index, [self.as_y, self.as_y_err])
        # if the column previously had a Y Err associated with it -> this will remove it from the YErr list
        self._remove_associated_yerr_columns(col_index, removed_items)

        if col_index not in self.as_x:
            self.as_x.append(col_index)

        return removed_items

    def add_y(self, col_index):
        removed_items = self._remove(col_index, [self.as_x, self.as_y_err])

        if col_index not in self.as_y:
            self.as_y.append(col_index)

        return removed_items

    def add_y_err(self, err_column):
        if err_column.related_y_column in self.as_x:
            raise ValueError("Trying to add YErr for column marked as X.")
        elif err_column.related_y_column in self.as_y_err:
            raise ValueError("Trying to add YErr for column marked as YErr.")
        # remove all labels for the column index
        removed_items = self._remove(err_column, [self.as_x, self.as_y, self.as_y_err])
        # if the column previously had a Y Err associated with it -> this will remove it from the YErr list
        # this case isn't handled by the __eq__ and __comp__ functions on the ErrorColumn class
        self._remove_associated_yerr_columns(err_column, removed_items)

        self.as_y_err.append(err_column)

        return removed_items

    def remove(self, col_index):
        removed_cols = self._remove(col_index, [self.as_x, self.as_y, self.as_y_err])
        # if the column previously had a Y Err associated with it -> this will remove it from the YErr list
        self._remove_associated_yerr_columns(col_index, removed_cols)

    def _remove_associated_yerr_columns(self, col_index, removed_cols):
        # we can only have 1 Y Err for Y, so iterating and removing's iterator invalidation is not an
        # issue as the code will exit immediately after the removal
        for col in self.as_y_err:
            if col.related_y_column == col_index:
                self.as_y_err.remove(col)
                if col not in removed_cols:
                    removed_cols.append(col)
                break

    def _make_labels(self, list, label):
        return [(col_num, label.format(index),) for index, col_num in enumerate(list)]

    def build_labels(self):
        extra_labels = []
        extra_labels.extend(self._make_labels(self.as_x, self.X_LABEL))
        extra_labels.extend(self._make_labels(self.as_y, self.Y_LABEL))
        err_labels = [(err_col.column, self.Y_ERR_LABEL.format(self.as_y.index(err_col.related_y_column)),) for
                      index, err_col in
                      enumerate(self.as_y_err) if self.as_y.count(err_col.related_y_column)>0]
        extra_labels.extend(err_labels)
        return extra_labels

    def find_yerr(self, selected_columns):
        """
        Retrieve the corresponding YErr column for each Y column, so that it can be plotted
        :param selected_columns: Selected Y columns for which their YErr columns will be retrieved
        :return: Dict[Selected Column] = Column with YErr
        :returns: Dictionary, where the keys are selected columns,
                  and the values the error associated with the selected columns
        """
        yerr_for_col = {}

        # for each selected column
        for col in selected_columns:
            # find the marked error column
            for yerr_col in self.as_y_err:
                # if found append the YErr's source column - so that the data from the columns
                # can be retrieved for plotting the errors
                if yerr_col.related_y_column == col:
                    yerr_for_col[col] = yerr_col.column

        return yerr_for_col
