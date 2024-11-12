# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
#
#
from workbench.widgets.settings.categories.view import CategoriesSettingsView

from qtpy.QtCore import Qt


class CategoriesSettings(object):
    """
    Presenter of the visible categories settings section. It handles all changes to options
    within the section, and updates the ConfigService and workbench CONF accordingly.

    If new options are added to the categories settings, their events when changed should
    be handled here.
    """

    def __init__(self, parent, model, view=None):
        self._view = view if view else CategoriesSettingsView(parent, self)
        self.parent = parent
        self._model = model
        self._view.algorithm_tree_widget.setHeaderLabel("Show/Hide Algorithm Categories")
        self._view.interface_tree_widget.setHeaderLabel("Show/Hide Interface Categories")
        self.set_algorithm_tree_categories()
        self.set_interface_tree_categories()
        self._view.algorithm_tree_widget.itemClicked.connect(self.nested_box_clicked)
        self._view.algorithm_tree_widget.itemChanged.connect(self.set_hidden_algorithms_string)
        self._view.interface_tree_widget.itemChanged.connect(self.set_hidden_interfaces_string)

    def get_view(self):
        return self._view

    def set_hidden_algorithms_string(self, _):
        categories_string = ";".join(self._create_hidden_categories_string(self._view.algorithm_tree_widget))
        self._model.set_hidden_algorithms(categories_string)

    def set_hidden_interfaces_string(self, _):
        categories_string = ";".join(self._create_hidden_categories_string(self._view.interface_tree_widget))
        self._model.set_hidden_interfaces(categories_string)

    def nested_box_clicked(self, item_clicked, column):
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

    def _create_hidden_categories_string(self, widget, parent=None):
        results = []
        if parent:
            count = parent.childCount()
        else:
            count = widget.topLevelItemCount()

        for i in range(0, count):
            if parent:
                item = parent.child(i)
            else:
                item = widget.topLevelItem(i)

            if item.checkState(0) == Qt.Unchecked:
                results.append(item.text(0))
            child_hidden_categories = self._create_hidden_categories_string(widget, item)

            for child in child_hidden_categories:
                results.append(item.text(0) + "\\" + child)
        return results

    def set_interface_tree_categories(self):
        widget = self._view.interface_tree_widget
        interfaces = []
        if self.parent:
            interfaces = self.parent.interface_list
        hidden_interfaces = self._model.get_hidden_interfaces().split(";")
        interface_map = {}
        for interface in interfaces:
            if interface in hidden_interfaces:
                interface_map[interface] = True
            else:
                interface_map[interface] = False

        self._set_tree_categories(widget, interface_map, False)

    def set_algorithm_tree_categories(self):
        widget = self._view.algorithm_tree_widget
        category_map = self._model.get_algorithm_factory_category_map()
        self._set_tree_categories(widget, category_map)

    def update_properties(self):
        self.set_algorithm_tree_categories()
        self.set_interface_tree_categories()

    def _set_tree_categories(self, widget, category_and_states, has_nested=True):
        widget.clear()
        seen_categories = {}
        for category_location in sorted(category_and_states):
            split_cat = category_location.split("\\")
            if len(split_cat) == 1:
                # This is a top level category
                category_name = str(split_cat[0])
                category_item = self._view.add_checked_widget_item(widget, category_name, category_and_states[category_location])
                seen_categories[category_name] = category_item
            else:
                for i in range(0, len(split_cat) - 1):
                    # Check if parent categories have already been added and if not add them
                    parent_name = str(split_cat[i])
                    if split_cat[i] not in seen_categories:
                        # we need to go back up the category tree to find the first seen grandparent
                        # to attach the new tree node to the correct parent
                        grand_parent_cat = None
                        for k in range(i - 1, -1, -1):
                            if split_cat[k] in seen_categories:
                                grand_parent_cat = seen_categories[split_cat[k]]
                        parent_category = self._view.add_checked_widget_item(
                            widget, parent_name, category_and_states[category_location], grand_parent_cat
                        )
                        seen_categories[parent_name] = parent_category

                child_name = str(split_cat[-1])
                child_item = self._view.add_checked_widget_item(
                    widget, child_name, category_and_states[category_location], seen_categories[parent_name]
                )
                seen_categories[child_name] = child_item
        if has_nested:
            for i in range(0, widget.topLevelItemCount()):
                self._update_partial_ticks(widget.topLevelItem(i))

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
