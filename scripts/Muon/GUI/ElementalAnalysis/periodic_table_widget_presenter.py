import matplotlib as mpl
from qtpy.QtWidgets import QFileDialog
from copy import deepcopy

from MultiPlotting.label import Label
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_presenter import PeriodicTablePresenter
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_model import PeriodicTableModel
from Muon.GUI.ElementalAnalysis.PeriodicTable.PeakSelector.peak_selector_presenter import PeakSelectorPresenter
from Muon.GUI.ElementalAnalysis.PeriodicTable.PeakSelector.peak_selector_view import PeakSelectorView
from Muon.GUI.ElementalAnalysis.Peaks.peaks_presenter import PeaksPresenter
from Muon.GUI.Common import message_box

offset = 0.9


def is_string(value):
    if isinstance(value, str):
        return True

    return False


def gen_name(element, name):
    msg = None
    if not is_string(element):
        msg = "'{}' expected element to be 'str', found '{}' instead".format(
            str(element), type(element))
    if not is_string(name):
        msg = "'{}' expected name to be 'str', found '{}' instead".format(str(name), type(name))

    if msg is not None:
        raise TypeError(msg)

    if element in name:
        return name
    return element + " " + name


class PeriodicTableWidgetPresenter(object):

    def __init__(self, view):
        self.view = view
        self.plotting = None
        self.plot_window = None
        self.ptable_model = PeriodicTableModel()
        self.ptable = PeriodicTablePresenter(self.view.ptable_view, self.ptable_model)
        self.peakpresenter = PeaksPresenter(self.view.peaks_view)

        self.element_widgets = {}
        self.element_lines = {}

        self.num_colors = len(mpl.rcParams['axes.prop_cycle'])
        self.used_colors = {}
        self.ptable.register_table_lclicked(self.table_left_clicked)
        self.ptable.register_table_rclicked(self.table_right_clicked)
        self._generate_element_widgets()
        self._setup_peaks_widget()

    def _setup_peaks_widget(self):
        self.peakpresenter.major.setChecked(True)
        self.peakpresenter.major.on_checkbox_checked(self.major_peaks_changed)
        self.peakpresenter.major.on_checkbox_unchecked(self.major_peaks_changed)
        self.peakpresenter.minor.on_checkbox_checked(self.minor_peaks_changed)
        self.peakpresenter.minor.on_checkbox_unchecked(self.minor_peaks_changed)
        self.peakpresenter.gamma.on_checkbox_checked(self.gammas_changed)
        self.peakpresenter.gamma.on_checkbox_unchecked(self.gammas_changed)
        self.peakpresenter.electron.on_checkbox_checked(self.electrons_changed)
        self.peakpresenter.electron.on_checkbox_unchecked(self.electrons_changed)
        self.peakpresenter.set_deselect_elements_slot(self.deselect_elements)

    # interact with periodic table
    def table_right_clicked(self, item):
        self.element_widgets[item.symbol].view.show()

    def table_left_clicked(self, item):
        if self.ptable.is_selected(item.symbol):
            self._add_element_lines(item.symbol)
        else:
            self._remove_element_lines(item.symbol)

    # setup element pop up
    def _generate_element_widgets(self):
        self.element_widgets = {}
        for element in self.ptable.peak_data:
            data = self.ptable.element_data(element)
            widget = PeakSelectorPresenter(PeakSelectorView(data, element))
            widget.on_finished(self._update_peak_data)
            self.element_widgets[element] = widget

    def _update_peak_data(self, element):
        if self.ptable.is_selected(element):
            # remove all of the lines for the element
            self._remove_element_lines(element)
            # add the ticked lines to the plot
            self._add_element_lines(element)
        else:
            self._remove_element_lines(element)

    def _remove_element_lines(self, element):
        if element not in self.element_lines:
            return
        names = deepcopy(self.element_lines[element])
        for name in names:
            try:
                self.element_lines[element].remove(name)
                self._rm_line(name)
                if not self.element_lines[element]:
                    del self.element_lines[element]
                    if element in self.used_colors:
                        del self.used_colors[element]
            except:
                continue

        if element in self.element_lines and not self.element_lines[element]:
            del self.element_lines[element]

    def _add_element_lines(self, element, data=None):
        if data is None:
            data = self.element_widgets[element].get_checked()
        if element not in self.element_lines:
            self.element_lines[element] = []

        # Select a different color, if all used then use the first
        color = self.get_color(element)

        for name, x_value in data.items():
            try:
                x_value = float(x_value)
            except ValueError:
                continue
            full_name = gen_name(element, name)
            if full_name not in self.element_lines[element]:
                self.element_lines[element].append(full_name)
            self._plot_line(full_name, x_value, color, element)

    def get_color(self, element):
        """
        When requesting the colour for a new element, return the first unused colour of the matplotlib
        default colour cycle (i.e. mpl.rcParams['axes.prop_cycle']).
        If all colours are used, return the first among the least used ones.
        That is if C0-4 are all used twice and C5-9 are used once, C5 will be returned.
        When requesting the colour for an element that is already plotted return the colour of that element.
        This prevents the same element from being displayed in different colours in separate plots
        :param element: Chemical symbol of the element that one wants the colour of
        :return: Matplotlib colour string: C0, C1, ..., C9 to be used as plt.plot(..., color='C3')
        """
        if element in self.used_colors:
            return self.used_colors[element]

        occurrences = [
            list(self.used_colors.values()).count('C{}'.format(i)) for i in range(self.num_colors)
        ]

        color_index = occurrences.index(min(occurrences))

        color = "C{}".format(color_index)
        self.used_colors[element] = color

        return color

    def _gen_label(self, name, x_value_in, element=None):
        if element is None:
            return
        # check x value is a float
        try:
            x_value = float(x_value_in)
        except:
            return
        if name not in self.element_lines[element]:
            self.element_lines[element].append(name)
        # make sure the names are strings and x values are numbers
        return Label(str(name), x_value, False, offset, True, rotation=-90, protected=True)

    def _plot_line(self, name, x_value_in, color, element=None):
        label = self._gen_label(name, x_value_in, element)
        if self.plot_window is None:
            return
        for subplot in self.plotting.get_subplots():
            self._plot_line_once(subplot, x_value_in, label, color)

    def _plot_line_once(self, subplot, x_value, label, color):
        self.plotting.add_vline_and_annotate(subplot, x_value, label, color)

    def _rm_line(self, name):
        if self.plot_window is None:
            return
        for subplot in self.plotting.get_subplots():
            self.plotting.rm_vline_and_annotate(subplot, name)

    def electrons_changed(self, electron_peaks):
        for element, selector in self.element_widgets.items():
            self.checked_data(element, selector.electron_checkboxes, electron_peaks.isChecked())

    def gammas_changed(self, gamma_peaks):
        for element, selector in self.element_widgets.items():
            self.checked_data(element, selector.gamma_checkboxes, gamma_peaks.isChecked())

    def major_peaks_changed(self, major_peaks):
        for element, selector in self.element_widgets.items():
            self.checked_data(element, selector.primary_checkboxes, major_peaks.isChecked())

    def minor_peaks_changed(self, minor_peaks):
        for element, selector in self.element_widgets.items():
            self.checked_data(element, selector.secondary_checkboxes, minor_peaks.isChecked())

    def deselect_elements(self):
        self.peakpresenter.disable_deselect_elements_btn()
        for element in self.element_widgets.keys():
            self.ptable.deselect_element(element)
            self._remove_element_lines(element)
        self.peakpresenter.enable_deselect_elements_btn()

    # general checked data
    def checked_data(self, element, selection, state):
        for checkbox in selection:
            checkbox.setChecked(state)
        self._update_peak_data(element)

    def _update_checked_data(self):
        self.major_peaks_changed(self.peakpresenter.major)
        self.minor_peaks_changed(self.peakpresenter.minor)
        self.gammas_changed(self.peakpresenter.gamma)
        self.electrons_changed(self.peakpresenter.electron)

    # sets data file for periodic table
    def select_data_file(self):
        old_lines = deepcopy(list(self.element_lines.keys()))

        filename = QFileDialog.getOpenFileName()
        if isinstance(filename, tuple):
            filename = filename[0]
        filename = str(filename)
        if filename:
            self.ptable.set_peak_datafile(filename)

        try:
            self._generate_element_widgets()
        except ValueError:
            message_box.warning(
                'The file does not contain correctly formatted data, resetting to default data file.'
                'See "https://docs.mantidproject.org/nightly/interfaces/'
                'Muon%20Elemental%20Analysis.html" for more information.')
            self.ptable.set_peak_datafile(None)
            self._generate_element_widgets()

        for element in old_lines:
            if element in self.element_widgets.keys():
                self.ptable.select_element(element)
            else:
                self._remove_element_lines(element)
        self._update_checked_data()
