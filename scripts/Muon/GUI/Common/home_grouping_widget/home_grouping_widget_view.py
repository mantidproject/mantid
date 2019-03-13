# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtGui, QtCore
from Muon.GUI.Common.utilities.run_string_utils import valid_alpha_regex
from Muon.GUI.Common.message_box import warning


class HomeGroupingWidgetView(QtGui.QWidget):

    def __init__(self, parent=None):
        super(HomeGroupingWidgetView, self).__init__(parent)
        self.setup_interface()

        self.alpha_hidden(True)
        self.periods_hidden(False)

    def setup_interface(self, show_checks=False):
        self.setObjectName("GroupingWidget")
        self.resize(500, 100)

        self.grouppair_label = QtGui.QLabel(self)
        self.grouppair_label.setObjectName("groupPairLabel")
        self.grouppair_label.setText("Group / ")

        self.pair_label = QtGui.QLabel(self)
        self.pair_label.setObjectName("pairLabel")
        font = QtGui.QFont()
        font.setBold(True)
        self.pair_label.setFont(font)
        self.pair_label.setText("Pair : ")

        self.grouppair_selector = QtGui.QComboBox(self)
        self.grouppair_selector.setObjectName("groupPairSelector")
        self.grouppair_selector.addItems(["fwd", "bwd"])

        self.alpha_label_2 = QtGui.QLabel(self)
        self.alpha_label_2.setObjectName("AlphaLabel")
        self.alpha_label_2.setText("Alpha : ")

        self.alpha_edit = QtGui.QLineEdit(self)
        self.alpha_edit.setObjectName("alphaEdit")
        self.alpha_edit.setText("1.0")
        self.alpha_edit.setEnabled(False)

        reg_ex = QtCore.QRegExp(valid_alpha_regex)
        alpha_validator = QtGui.QRegExpValidator(reg_ex, self.alpha_edit)
        self.alpha_edit.setValidator(alpha_validator)

        self.horizontal_layout = QtGui.QHBoxLayout()
        self.horizontal_layout.setObjectName("horizontalLayout3")
        self.horizontal_layout.addWidget(self.grouppair_label)
        self.horizontal_layout.addWidget(self.pair_label)
        self.horizontal_layout.addWidget(self.grouppair_selector)
        self.horizontal_layout.addStretch(0)
        self.horizontal_layout.addWidget(self.alpha_label_2)
        self.horizontal_layout.addWidget(self.alpha_edit)

        self.period_label = QtGui.QLabel(self)
        self.period_label.setObjectName("periodLabel")
        self.period_label.setText("Data collected in n periods. Plot/analysis period(s) : ")

        self.summed_period_edit = QtGui.QLineEdit(self)
        self.summed_period_edit.setText("1")
        reg_ex = QtCore.QRegExp("^[0-9]*([0-9]+[,-]{0,1})*[0-9]+$")
        period_validator = QtGui.QRegExpValidator(reg_ex, self.summed_period_edit)
        self.summed_period_edit.setValidator(period_validator)

        self.minus_label = QtGui.QLabel(self)
        self.minus_label.setObjectName("minusLabel")
        self.minus_label.setText("-")

        self.subtracted_period_edit = QtGui.QLineEdit(self)
        self.subtracted_period_edit.setText("")
        period_validator = QtGui.QRegExpValidator(reg_ex, self.subtracted_period_edit)
        self.subtracted_period_edit.setValidator(period_validator)

        self.horizontal_layout_2 = QtGui.QHBoxLayout()
        self.horizontal_layout_2.setObjectName("horizontalLayout2")
        self.horizontal_layout_2.addWidget(self.period_label)
        self.horizontal_layout_2.addStretch(0)
        self.horizontal_layout_2.addWidget(self.summed_period_edit)
        self.horizontal_layout_2.addSpacing(10)
        self.horizontal_layout_2.addWidget(self.minus_label)
        self.horizontal_layout_2.addSpacing(10)
        self.horizontal_layout_2.addWidget(self.subtracted_period_edit)

        self.group = QtGui.QGroupBox("Groups and Pairs")
        self.group.setFlat(False)
        self.setStyleSheet("QGroupBox {border: 1px solid grey;border-radius: 10px;margin-top: 1ex; margin-right: 0ex}"
                           "QGroupBox:title {"
                           'subcontrol-origin: margin;'
                           "padding: 0 3px;"
                           'subcontrol-position: top center;'
                           'padding-top: 0px;'
                           'padding-bottom: 0px;'
                           "padding-right: 10px;"
                           ' color: grey; }')

        self.vertical_layout = QtGui.QVBoxLayout(self)
        self.vertical_layout.setObjectName("verticalLayout")
        self.vertical_layout.addItem(self.horizontal_layout)
        self.vertical_layout.addItem(self.horizontal_layout_2)
        if show_checks:
            self.all_groups_label = QtGui.QLabel(self)
            self.all_groups_label.setText("Apply to all groups : ")
            self.all_groups_checkbox = QtGui.QCheckBox(self)
            self.all_pairs_label = QtGui.QLabel(self)
            self.all_pairs_label.setText("Apply to all pairs (alpha will not be applied)  : ")
            self.all_pairs_checkbox = QtGui.QCheckBox(self)

            self.horizontal_layout_3 = QtGui.QHBoxLayout()
            self.horizontal_layout_3.setObjectName("horizontalLayout2")
            self.horizontal_layout_3.addWidget(self.all_groups_label)
            self.horizontal_layout_3.addWidget(self.all_groups_checkbox)

            self.horizontal_layout_4 = QtGui.QHBoxLayout()
            self.horizontal_layout_4.addWidget(self.all_pairs_label)
            self.horizontal_layout_4.addWidget(self.all_pairs_checkbox)
            self.vertical_layout.addItem(self.horizontal_layout_3)
            self.vertical_layout.addItem(self.horizontal_layout_4)

        self.group.setLayout(self.vertical_layout)

        self.widget_layout = QtGui.QVBoxLayout(self)
        self.widget_layout.addWidget(self.group)
        self.setLayout(self.widget_layout)

    def warning_popup(self, message):
        warning(message, parent=self)

    # ------------------------------------------------------------------------------------------------------------------
    # Groups and Pairs
    # ------------------------------------------------------------------------------------------------------------------

    def populate_group_pair_selector(self, group_names, pair_names):
        self.grouppair_selector.clear()

        model = self.grouppair_selector.model()
        for name in group_names:
            item = QtGui.QStandardItem(str(name))
            font = item.font()
            item.setFont(font)
            model.appendRow(item)
        for name in pair_names:
            item = QtGui.QStandardItem(str(name))
            #item.setForeground(QtGui.QColor('red'))
            font = item.font()
            font.setBold(True)
            item.setFont(font)
            model.appendRow(item)

    def get_selected_group_or_pair_name(self):
        return self.grouppair_selector.currentText()

    def on_grouppair_selection_changed(self, slot):
        self.grouppair_selector.currentIndexChanged.connect(slot)

    def get_currently_selected_group_pair(self):
        return self.grouppair_selector.currentText()

    # ------------------------------------------------------------------------------------------------------------------
    # Alpha
    # ------------------------------------------------------------------------------------------------------------------

    def on_alpha_changed(self, slot):
        self.alpha_edit.editingFinished.connect(slot)

    def set_current_alpha(self, alpha):
        self.alpha_edit.setText(str(alpha))

    def get_current_alpha(self):
        return float(self.alpha_edit.text())

    def alpha_hidden(self, hidden=True):
        """
        hide/show the functionality for the alpha parameter
        """
        if hidden:
            self.alpha_label_2.hide()
            self.alpha_edit.hide()
        if not hidden:
            self.alpha_label_2.setVisible(True)
            self.alpha_edit.setVisible(True)

    # ------------------------------------------------------------------------------------------------------------------
    # Periods
    # ------------------------------------------------------------------------------------------------------------------

    def set_summed_periods(self, text):
        self.summed_period_edit.setText(text)

    def set_subtracted_periods(self, text):
        self.subtracted_period_edit.setText(text)

    def get_summed_periods(self):
        return str(self.summed_period_edit.text())

    def get_subtracted_periods(self):
        return str(self.subtracted_period_edit.text())

    def on_summed_periods_changed(self, slot):
        self.summed_period_edit.editingFinished.connect(slot)

    def on_subtracted_periods_changed(self, slot):
        self.subtracted_period_edit.editingFinished.connect(slot)

    def set_period_number_in_period_label(self, n_periods):
        self.period_label.setText("Data collected in " + str(n_periods) + " periods. Plot/analysis period(s) : ")

    def multi_period_widget_hidden(self, hidden=True):
        self.periods_hidden(hidden)

    def periods_hidden(self, hidden=True):
        """
        show/hide the multi-period data functionality.
        """
        if hidden:
            self.period_label.hide()
            self.summed_period_edit.hide()
            self.minus_label.hide()
            self.subtracted_period_edit.hide()
        if not hidden:
            self.period_label.setVisible(True)
            self.summed_period_edit.setVisible(True)
            self.minus_label.setVisible(True)
            self.subtracted_period_edit.setVisible(True)
