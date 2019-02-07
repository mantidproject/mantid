# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

from qtpy.QtCore import QThread, Slot


class RecoveryThread(QThread):
    def __init__(self):
        super(RecoveryThread, self).__init__()
        self.failed_run_in_thread = True
        self.checkpoint = None
        self.project_recovery = None

    def run(self):
        self.failed_run_in_thread = self.project_recovery.load_checkpoint(self.checkpoint)



