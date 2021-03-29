# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.ElementalAnalysis2.auto_widget.ea_match_table_view import EAMatchTableView


class EAMatchTablePresenter(object):

    def __init__(self , view : EAMatchTableView):
        self.view = view

    def update_table(self,table_entries):
        for entry in table_entries:
            self.view.add_entry_to_table(entry)

    def clear_table(self):
        self.view.table.clearContents()
