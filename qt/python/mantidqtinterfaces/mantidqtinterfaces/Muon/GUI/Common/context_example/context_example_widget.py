# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.Muon.GUI.Common.context_example.context_example_view import ContextExampleView
from mantidqtinterfaces.Muon.GUI.Common.context_example.context_example_presenter import ContextExamplePresenter
from mantidqtinterfaces.Muon.GUI.Common.context_example.context_example_model import ContextExampleModel


class ContextExampleWidget(object):
    """
    An example of how to use the context with a widget class.
    The widget class exposes the MVP to the rest of the GUI
    """

    def __init__(self, context, parent=None):
        model = ContextExampleModel(context)
        sub_context = model.getSubContext()
        view = ContextExampleView(sub_context, parent)
        self._presenter = ContextExamplePresenter(view, model)

    @property
    def presenter(self):
        return self._presenter

    @property
    def widget(self):
        return self._presenter.widget

    # interact with context
    def setUpdateContext(self, slot):
        """
        This function is to set the update
        method from the main GUI to the signals
        from this GUI
        """
        view = self._presenter.widget
        view.updateSignal.connect(slot)

    def updateContext(self):
        self._presenter.updateContext()

    def loadFromContext(self):
        # extract relevant info from context via model
        model = self._presenter.model
        sub_context = model.getSubContext()
        # update the view with the subcontext
        view = self._presenter.widget
        view.loadFromContext(sub_context)
