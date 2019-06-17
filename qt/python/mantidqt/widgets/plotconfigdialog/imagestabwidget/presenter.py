# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from mantidqt.utils import BlockQSignals
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
        props = ImageProperties.from_view(self.view)
        image = self.get_selected_image()
        self.set_selected_image_label(props.label)
        image.set_cmap(props.colormap)
        if props.vmin == 0 and props.scale == "Logarithmic":
            props.vmin += 1e-6  # Avoid 0 log scale error
        image.set_norm(SCALES[props.scale](vmin=props.vmin, vmax=props.vmax))
        if props.interpolation:
            image.set_interpolation(props.interpolation)

    def get_selected_image(self):
        return self.image_names_dict[self.view.get_selected_image_name()]

    def populate_select_image_combo_box_and_update_view(self):
        with BlockQSignals(self.view.select_image_combo_box):
            self._populate_select_image_combo_box()
        self.update_view()

    def set_selected_image_label(self, label):
        image = self.image_names_dict.pop(self.view.get_selected_image_name())
        image.set_label(label)
        new_name = self.generate_image_name(image)
        self.image_names_dict[new_name] = image
        self.view.replace_selected_image_name(new_name)

    def update_view(self):
        # self.view.update(ImageProperties.from_image(self.get_selected_image()))
        img_props = ImageProperties.from_image(self.get_selected_image())
        self.view.set_label(img_props.label)
        self.view.set_colormap(img_props.colormap)
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
        self.view.populate_select_image_combo_box(self.image_names_dict.keys())
