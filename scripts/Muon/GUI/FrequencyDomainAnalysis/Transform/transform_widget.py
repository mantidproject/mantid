# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.FrequencyDomainAnalysis.Transform.transform_view import TransformView

from Muon.GUI.FrequencyDomainAnalysis.TransformSelection.transform_selection_widget import TransformSelectionWidget
from mantidqt.utils.observer_pattern import Observer, GenericObserver

from qtpy import QtWidgets


class TransformWidget(QtWidgets.QWidget):

    def __init__(self, context, fft_widget, maxent_widget, parent=None):
        super(TransformWidget, self).__init__(parent)
        self._fft = fft_widget(load=context, parent=self)
        self._maxent = maxent_widget(context=context, parent=self)
        self._selector = TransformSelectionWidget(parent=self)
        self.load_observer = LoadObserver(self)
        self.context = context
        self.instrumentObserver = instrumentObserver(self)
        self.GroupPairObserver = GenericObserver(self.handle_new_group_pair)
        self.enable_observer = EnableObserver(self)
        self.disable_observer = DisableObserver(self)
        self.phase_quad_observer = PhaseQuadObserver(self)

        groupedViews = self.getViews()

        self._view = TransformView(self._selector.widget, groupedViews, parent)

        self._selector.setSelectionConnection(self.updateDisplay)
        self.updateDisplay('FFT')
        self.update_view_from_model_observer = GenericObserver(
            self.update_view_from_model)
        self.disable_view()
        # to make it compatable with the old GUI
        try:
            self.context.update_view_from_model_notifier.add_subscriber(
                self.update_view_from_model_observer)
        except:
            pass

    def update_view_from_model(self):
        self._fft.update_view_from_model()
        self._maxent.update_view_from_model()

    @property
    def widget(self):
        return self._view

    def mockWidget(self, mockView):
        self._view = mockView

    def closeEvent(self, event):
        self._selector.closeEvent(event)
        self._fft.closeEvent(event)
        self._maxent.closeEvent(event)

    def updateDisplay(self, method):
        self._view.hideAll()
        self._view.showMethod(method)

    def getViews(self):
        groupedViews = {}
        groupedViews["FFT"] = self._fft.widget
        groupedViews["MaxEnt"] = self._maxent.widget
        return groupedViews

    def handle_new_data_loaded(self):
        self._maxent.runChanged()
        self._fft.runChanged()
        if self.getViews()["FFT"].workspace == "":
            self.disable_view()
        else:
            self.enable_view()

    def handle_new_instrument(self):
        self._maxent.clear()

    def handle_new_group_pair(self):
        # may have new groups/pairs for multiple runs
        self._fft.runChanged()
        self._maxent.presenter.update_phase_table_options()

    def disable_view(self):
        self._view.setEnabled(False)

    def enable_view(self):
        self._view.setEnabled(True)

    def set_up_calculation_observers(self, enable, disable):
        # assume FFT are cheap enough that disable/enable GUI would make no
        # difference
        self._maxent._presenter.calculation_finished_notifier.add_subscriber(
            enable)
        self._maxent._presenter.calculation_started_notifier.add_subscriber(
            disable)

    def new_data_observer(self, observer):
        self._maxent._presenter.calculation_finished_notifier.add_subscriber(
            observer)
        self._fft._presenter.calculation_finished_notifier.add_subscriber(
            observer)


class LoadObserver(Observer):

    def __init__(self, outer):
        Observer.__init__(self)
        self.outer = outer

    def update(self, observable, arg):
        self.outer.handle_new_data_loaded()


class instrumentObserver(Observer):

    def __init__(self, outer):
        Observer.__init__(self)
        self.outer = outer

    def update(self, observable, arg):
        self.outer.handle_new_instrument()


class EnableObserver(Observer):

    def __init__(self, outer):
        Observer.__init__(self)
        self.outer = outer

    def update(self, observable, arg):
        self.outer.enable_view()


class DisableObserver(Observer):

    def __init__(self, outer):
        Observer.__init__(self)
        self.outer = outer

    def update(self, observable, arg):
        self.outer.disable_view()


class PhaseQuadObserver(Observer):

    def __init__(self, outer):
        Observer.__init__(self)
        self.outer = outer

    def update(self, observable, arg):
        self.outer.handle_new_data_loaded()
