# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from mantid.plots.datafunctions import update_colorbar_scale
from mantidqt.utils.qt import block_signals
from mantidqt.widgets.plotconfigdialog import generate_ax_name, get_images_from_fig
from mantidqt.widgets.plotconfigdialog.imagestabwidget import ImageProperties
from mantidqt.widgets.plotconfigdialog.imagestabwidget.view import ImagesTabWidgetView, SCALES


class ImagesTabWidgetPresenter:

    def __init__(self, fig, view=None, parent=None):
        self.fig = fig
        if not view:
            self.view = ImagesTabWidgetView(parent)
        else:
            self.view = view

        self.image_names_dict = dict()
        self.populate_select_image_combo_box_and_update_view()

        # Signals
        self.view.select_image_combo_box.currentIndexChanged.connect(
            self.update_view)

    def apply_properties(self):
        props = self.view.get_properties()
        image = self.get_selected_image()
        self.set_selected_image_label(props.label)
        image.set_cmap(props.colormap)
        if props.interpolation:
            image.set_interpolation(props.interpolation)

        update_colorbar_scale(self.fig, image, SCALES[props.scale], props.vmin, props.vmax)

        if props.vmin > props.vmax:
            self.view.max_min_value_warning.setVisible(True)
            self.view.max_min_value_warning.setText("<html> <head/> <body> <p> <span style=\"color:#ff0000;\">Max "
                                                    "value is less than min value so they have been "
                                                    "swapped.</span></p></body></html>")
        elif props.vmin == props.vmax:
            self.view.max_min_value_warning.setVisible(True)
            self.view.max_min_value_warning.setText("<html><head/><body><p><span style=\"color:#ff0000;\">Min and max "
                                                    "value are the same so they have been "
                                                    "adjusted.</span></p></body></html>")
        else:
            self.view.max_min_value_warning.setVisible(False)

    def get_selected_image(self):
        return self.image_names_dict[self.view.get_selected_image_name()]

    def populate_select_image_combo_box_and_update_view(self):
        with block_signals(self.view.select_image_combo_box):
            self._populate_select_image_combo_box()
        self.update_view()

    def set_selected_image_label(self, label):
        image = self.image_names_dict.pop(self.view.get_selected_image_name())
        image.set_label(label)
        new_name = self.generate_image_name(image)
        self.image_names_dict[new_name] = image
        self.view.replace_selected_image_name(new_name)

    def update_view(self):
        img_props = ImageProperties.from_image(self.get_selected_image())
        self.view.set_label(img_props.label)
        self.view.set_colormap(img_props.colormap)
        self.view.set_reverse_colormap(img_props.reverse_colormap)
        self.view.set_min_value(img_props.vmin)
        self.view.set_max_value(img_props.vmax)
        if img_props.interpolation:
            self.view.enable_interpolation(True)
            self.view.set_interpolation(img_props.interpolation)
        else:
            self.view.set_interpolation('None')
            self.view.enable_interpolation(False)
        self.view.set_scale(img_props.scale)

    @staticmethod
    def generate_image_name(image):
        """Generate a name for an image"""
        label = image.get_label()
        ax_name = generate_ax_name(image.axes)
        if label:
            return "{} - {}".format(ax_name, label)
        else:
            return "{}".format(ax_name)

    @staticmethod
    def set_name_in_names_dict(name, value, name_dict):
        """Set name of image in image_names_dict"""
        idx = 1
        base_name = name
        while name in name_dict:
            name = base_name + "({})".format(idx)
            idx += 1
        name_dict[name] = value
        return name_dict

    def _populate_select_image_combo_box(self):
        self.view.select_image_combo_box.clear()
        for img in get_images_from_fig(self.fig):
            self.image_names_dict = self.set_name_in_names_dict(
                self.generate_image_name(img), img, self.image_names_dict)
        self.view.populate_select_image_combo_box(
            sorted(self.image_names_dict.keys()))
