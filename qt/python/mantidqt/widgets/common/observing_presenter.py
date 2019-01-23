# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.


class ObservingPresenter:
    """
    This class provides some common functions for classes that need to be observable.
    It is not a GUI class, and if used, should be inherited by the presenter.
    It is designed to be used with a view that inherits `ObservingView`.

    The presenter that is inheriting this must initialise a model and a view,
    that implement the methods required in the functions below.
    """

    def clear_observer(self):
        """
        If the observer is not cleared here then the C++ object is never freed,
        and observers keep getting created, and triggering on ADS events.

        This method is called from the view's close_later to ensure that
        observer are deleted both when the workspace is deleted and when
        the view window is closed manually (via the X)
        """
        self.ads_observer = None

    def close(self, workspace_name):
        if self.model.workspace_equals(workspace_name):
            # if the observer is not cleared here then the C++ object is never freed,
            # and observers keep getting created, and triggering on ADS events
            self.ads_observer = None
            self.view.emit_close()

    def force_close(self):
        self.ads_observer = None
        self.view.emit_close()

    def replace_workspace(self, workspace_name, workspace):
        raise NotImplementedError("This method must be overridden.")

    def rename_workspace(self, old_name, new_name):
        if self.model.workspace_equals(old_name):
            self.view.emit_rename(new_name)
