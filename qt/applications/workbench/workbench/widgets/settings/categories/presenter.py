# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
#
#
from __future__ import absolute_import, unicode_literals

from mantid.kernel import ConfigService
from workbench.widgets.settings.categories.view import CategoriesSettingsView
from mantid.api import AlgorithmFactory

from qtpy.QtCore import Qt


class CategoriesSettings(object):
    """
    Presenter of the visible categories settings section. It handles all changes to options
    within the section, and updates the ConfigService and workbench CONF accordingly.

    If new options are added to the categories settings, their events when changed should
    be handled here.
    """
    HIDDEN_ALGORITHMS = "algorithms.categories.hidden"

    def __init__(self, parent, view=None):
        self.view = view if view else CategoriesSettingsView(parent, self)
        self.parent = parent
        self._set_algorithm_tree_categories()
        self.view.algorithm_tree_widget.itemClicked.connect(self.box_clicked)
        self.view.algorithm_tree_widget.itemChanged.connect(self.set_hidden_algorithms_string)

        # Hidden until functionality added
        #self.view.functionCategories.setVisible(False)

    def set_hidden_algorithms_string(self, _):
        categories_string = ';'.join(self._create_hidden_categories_string())
        ConfigService.setString(self.HIDDEN_ALGORITHMS, categories_string)

    def box_clicked(self, item_clicked, column):
        new_state = item_clicked.checkState(column)
        self._update_child_tick_status(item_clicked, new_state)

        # find top level parent
        found_top_level_parent = False
        parent = item_clicked
        while not found_top_level_parent:
            if parent.parent():
                parent = parent.parent()
            else:
                found_top_level_parent = True

        self._update_partial_ticks(parent)

    def _create_hidden_categories_string(self, parent = None):
        results = []
        if parent:
            count = parent.childCount()
        else:
            count = self.view.algorithm_tree_widget.topLevelItemCount()

        for i in range(0, count):
            if parent:
                item = parent.child(i)
            else:
                item = self.view.algorithm_tree_widget.topLevelItem(i)

            if item.checkState(0) == Qt.Unchecked:
                results.append(item.text(0))
            child_hidden_categories = self._create_hidden_categories_string(item)

            for child in child_hidden_categories:
                results.append(item.text(0) + "\\" + child)
        return results

    def _set_algorithm_tree_categories(self):
        self.view.algorithm_tree_widget.clear()

        self.view.algorithm_tree_widget.setHeaderLabel("Show Algorithm Categories")
        category_map = AlgorithmFactory.Instance().getCategoriesandState()
        seen_categories = {}
        for category_location in sorted(category_map):
            split_cat = category_location.split("\\")
            if len(split_cat) == 1:
                # This is a top level category
                category_name = str(split_cat[0])
                category_item = self.view.add_checked_widget_item(category_name, category_map[category_location])
                seen_categories[category_name] = category_item
            else:
                for i in range(0, len(split_cat) - 1):
                    # Check if parent categories have already been added and if not add them
                    parent_name = str(split_cat[i])
                    if split_cat[i] not in seen_categories:
                        parent_category = self.view.add_checked_widget_item(parent_name, category_map[category_location])
                        seen_categories[parent_name] = parent_category

                child_name = str(split_cat[-1])
                child_item = self.view.add_checked_widget_item(child_name,
                                                               category_map[category_location],
                                                               seen_categories[parent_name])
                seen_categories[child_name] = child_item

        for i in range(0, self.view.algorithm_tree_widget.topLevelItemCount()):
            self._update_partial_ticks(self.view.algorithm_tree_widget.topLevelItem(i))

    def _update_child_tick_status(self, item, new_state):
        # update any sub levels
        for i in range(0, item.childCount()):
            self._update_child_tick_status(item.child(i), new_state)

        item.setCheckState(0, new_state)

    def _update_partial_ticks(self, parent):
        child_states = []
        for i in range(0, parent.childCount()):
            child = parent.child(i)
            # check any nested children first
            self._update_partial_ticks(child)
            child_states.append(child.checkState(0))

        if len(child_states) != 0:
            if all(i == Qt.Checked for i in child_states):
                parent.setCheckState(0, Qt.Checked)
            elif all(i == Qt.Unchecked for i in child_states):
                parent.setCheckState(0, Qt.Unchecked)
            else:
                parent.setCheckState(0, Qt.PartiallyChecked)
