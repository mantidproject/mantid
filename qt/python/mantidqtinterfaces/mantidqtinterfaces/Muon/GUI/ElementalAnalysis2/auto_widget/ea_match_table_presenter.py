# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


class EAMatchTablePresenter(object):

    def __init__(self, view):
        self.view = view
        self.table_entries = []

    def update_table(self, entry):
        index = self.find_entry_index(entry)
        if index != -1:
            self.remove_entry(index)
        self.table_entries.append(entry)
        self.view.add_entry_to_table(entry)

    def remove_entry(self, row_index):
        del self.table_entries[row_index]
        self.view.remove_row(row_index)

    def find_entry_index(self, new_entry):
        """
            Finds index of entry using first 2 columns returns -1 if not found
        """
        for i, entry in enumerate(self.table_entries):
            if entry[:2] == new_entry[:2]:
                return i
        return -1

    def clear_table(self):
        self.table_entries = []
        self.view.clear_table()
