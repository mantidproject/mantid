# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, division, print_function)


class ObservingView(object):
    """
    This class provides some common functions needed across views observers the ADS.
    It runs the close_signal so that the view is closed from the GUI thread,
    and ensures that the closeEvent will clear the observer to prevent a memory leak.

    It is designed to be used with a presenter that inherits `ObservingPresenter`.

    If this class is inherited a `close_signal` and a `rename_signal` must be declared.

    It is not possible to do that here, as this is not a QObject, however it was
    attempted to do the declaration here, but the signals don't seem to work
    through inheritance.

    This class shouldn't inherit from a QObject/QWidget, otherwise the closeEvent
    doesn't replace the closeEvent of the view that is inheriting this. This also
    makes it easily testable, as the GUI library isn't pulled in.
    """

    TITLE_STRING = "{} - Mantid"

    def emit_close(self):
        """
        Emits a close signal to the main GUI thread that triggers the built-in close method.

        This function can be called from outside the main GUI thread. It is currently
        used to close the window when the relevant workspace is deleted.

        When the signal is emitted, the execution returns to the GUI thread, and thus
        GUI code can be executed.
        """
        self.close_signal.emit()

    def closeEvent(self, event):
        # This clear prevents a leak when the window is closed from X by the user
        # for some reason it can't be done in the ObservingPresenter as it doesn't
        # seem to clear the ADSObserver object properly
        self.presenter.clear_observer()
        event.accept()
        self.deleteLater()

    def emit_rename(self, new_name):
        self.rename_signal.emit(new_name)

    def _rename(self, new_name):
        self.setWindowTitle(self.TITLE_STRING.format(new_name))
