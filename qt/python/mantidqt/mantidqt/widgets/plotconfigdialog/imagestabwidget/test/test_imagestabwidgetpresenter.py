# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest

import matplotlib
from matplotlib import use as mpl_use

mpl_use("Agg")
from matplotlib.colors import LogNorm, Normalize
from matplotlib.pyplot import figure
from numpy import linspace, random

from mantid.plots import datafunctions, MantidAxes  # register mantid projection  # noqa
from unittest.mock import Mock
from mantid.simpleapi import CreateWorkspace
from mantidqt.plotting.functions import pcolormesh
from mantidqt.widgets.plotconfigdialog.imagestabwidget import ImageProperties
from mantidqt.widgets.plotconfigdialog.imagestabwidget.presenter import ImagesTabWidgetPresenter


class ImagesTabWidgetPresenterTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        random.seed(4)
        cls.fig = figure()
        ax = cls.fig.add_subplot(221, projection="mantid")
        x = linspace(0, 10, 20)
        y = random.rand(20)
        cls.ws = CreateWorkspace(DataX=x, DataY=y, NSpec=4, OutputWorkspace="ws")
        ax.plot(cls.ws, specNum=1)

        ax1 = cls.fig.add_subplot(222)
        cls.img0_label = "Im0 label"
        cls.img0 = ax1.imshow([[0, 1], [1, 0]], label=cls.img0_label)

        ax3 = cls.fig.add_subplot(224, projection="mantid")
        cls.img1_label = "Im1 label"
        cls.img1 = ax3.imshow(cls.ws, label=cls.img1_label)
        ax3.set_title("Second Axes")

    @classmethod
    def tearDownClass(cls):
        cls.ws.delete()

    def _generate_presenter(self, view=None, fig=None):
        if not view:
            view = Mock(get_selected_image_name=lambda: "Second Axes: (1, 1) - {}".format(self.img1_label))
        if not fig:
            fig = self.fig
        return ImagesTabWidgetPresenter(view=view, fig=fig)

    def test_select_image_combo_box_populated_on_init(self):
        presenter = self._generate_presenter()
        presenter.view.populate_select_image_combo_box.assert_called_once_with(
            ["(0, 1) - {}".format(self.img0_label), "Second Axes: (1, 1) - {}".format(self.img1_label)]
        )

    def test_image_dict_names_populated_on_init(self):
        presenter = self._generate_presenter()
        expected = {"(0, 1) - {}".format(self.img0_label): [self.img0], "Second Axes: (1, 1) - {}".format(self.img1_label): [self.img1]}
        self.assertEqual(presenter.image_names_dict, expected)

    def test_image_dict_names_populated_correctly_for_plot_done_per_spectra(self):
        ws = CreateWorkspace(
            DataX=list([0, 1, 1.5, 1.75, 1.83]) * 4,
            DataY=range(20),
            NSpec=4,
            OutputWorkspace="test_ws",
            VerticalAxisUnit="TOF",
            VerticalAxisValues=[1, 2, 4, 10],
        )
        fig = figure()
        axes = fig.add_subplot(111, projection="mantid")
        label = "Test_Label"
        img = axes.pcolormesh(ws, label=label)
        fig.colorbar(img)
        mock_view = Mock(get_selected_image_name=lambda: "(0, 0) - {}".format(label))
        presenter = self._generate_presenter(fig=fig, view=mock_view)
        # need to unpack axes.collection from ArtistList
        axes_collection = list(axes.collections)
        expected = {"(0, 0) - {}".format(label): axes_collection}
        self.assertEqual(presenter.image_names_dict, expected)

    def test_generate_image_name(self):
        generated_name = ImagesTabWidgetPresenter.generate_image_name(self.img0)
        expected_name = "(0, 1) - {}".format(self.img0.get_label())
        self.assertEqual(generated_name, expected_name)

    def test_set_name_in_names_dict_increments_name_if_duplicated(self):
        names_dict = {"name0": None, "name1": None, "name2": None}
        new_names_dict = ImagesTabWidgetPresenter.set_name_in_names_dict("name0", 1, names_dict)
        names_dict.update({"name0 (1)": 1})
        self.assertEqual(names_dict, new_names_dict)

    def test_apply_properties_sets_properties(self):
        fig = figure()
        ax = fig.add_subplot(111)
        img = ax.imshow([[0, 2], [2, 0]], label="img label")
        fig.colorbar(img)
        props = {
            "title": "(0, 0) - img label",
            "label": "New Label",
            "colormap": "jet",
            "vmin": 0,
            "vmax": 2,
            "scale": "Linear",
            "interpolation": "None",
        }
        img_name = "(0, 0) - img label"
        mock_view = Mock(get_selected_image_name=lambda: img_name, get_properties=lambda: ImageProperties(props))
        presenter = self._generate_presenter(fig=fig, view=mock_view)
        presenter.apply_properties()
        self.assertEqual("New Label", img.colorbar.ax.get_ylabel())
        self.assertEqual("jet", img.cmap.name)
        self.assertEqual(0, img.norm.vmin)
        self.assertEqual(2, img.norm.vmax)
        self.assertTrue(isinstance(img.norm, Normalize))

    def test_apply_properties_log_scale_with_zero_vmin(self):
        fig = figure()
        ax = fig.add_subplot(111)
        img = ax.imshow([[0, 2], [2, 0]], label="img label")
        fig.colorbar(img)
        props = {
            "title": "(0, 0) - img label",
            "label": "New Label",
            "colormap": "jet",
            "vmin": 0,
            "vmax": 4,
            "scale": "Logarithmic",
            "interpolation": "Hanning",
        }
        mock_view = Mock(get_selected_image_name=lambda: "(0, 0) - img label", get_properties=lambda: ImageProperties(props))
        presenter = self._generate_presenter(fig=fig, view=mock_view)
        presenter.apply_properties()
        self.assertEqual("New Label", img.colorbar.ax.get_ylabel())
        self.assertEqual("jet", img.cmap.name)
        self.assertEqual(0.0001, img.norm.vmin)
        self.assertEqual(4, img.norm.vmax)
        self.assertTrue(isinstance(img.norm, LogNorm))

    def test_apply_properties_log_scale_with_negative_vmin_and_vmax(self):
        fig = figure()
        ax = fig.add_subplot(111)
        img = ax.imshow([[0, 2], [2, 0]], label="img label")
        fig.colorbar(img)
        props = {
            "title": "(0, 0) - img label",
            "label": "New Label",
            "colormap": "jet",
            "vmin": -10,
            "vmax": -5,
            "scale": "Logarithmic",
            "interpolation": "Hanning",
        }
        mock_view = Mock(get_selected_image_name=lambda: "(0, 0) - img label", get_properties=lambda: ImageProperties(props))
        presenter = self._generate_presenter(fig=fig, view=mock_view)
        presenter.apply_properties()
        self.assertEqual("New Label", img.colorbar.ax.get_ylabel())
        self.assertEqual("jet", img.cmap.name)
        self.assertEqual(0.0001, img.norm.vmin)
        self.assertEqual(1, img.norm.vmax)
        self.assertTrue(isinstance(img.norm, LogNorm))

    def test_interpolation_disabled_for_pcolormesh_image(self):
        fig = figure()
        ax = fig.add_subplot(111)
        ax.pcolormesh([[0, 2], [2, 0]], label="collection0")
        view = Mock(get_selected_image_name=lambda: "(0, 0) - collection0")
        presenter = self._generate_presenter(view=view, fig=fig)
        presenter.view.enable_interpolation.assert_called_once_with(False)

    def test_colorbars_not_included(self):
        fig = figure()
        ax = fig.add_subplot(111)
        img = ax.imshow([[0, 2], [2, 0]], label="img label")
        fig.colorbar(img, label="Colorbar")
        img_name = "(0, 0) - img label"
        view = Mock(get_selected_image_name=lambda: img_name)
        presenter = self._generate_presenter(view, fig)
        self.assertTrue(1, len(presenter.image_names_dict))
        presenter.view.populate_select_image_combo_box.assert_called_once_with([img_name])

    def test_apply_properties_applies_to_all_images_if_multiple_colorfill_plots_and_one_colorbar(self):
        fig = pcolormesh([self.ws, self.ws])
        props = {"label": "New Label", "colormap": "jet", "vmin": 0, "vmax": 2, "scale": "Linear", "interpolation": "None"}
        mock_view = Mock(get_selected_image_name=lambda: "ws: (0, 0) - child0", get_properties=lambda: ImageProperties(props))
        presenter = self._generate_presenter(fig=fig, view=mock_view)
        presenter.apply_properties()

        for ax in range(2):
            image = fig.axes[ax].images[0]

            if image.colorbar:
                self.assertEqual("New Label", image.colorbar.ax.get_ylabel())

            self.assertEqual("jet", image.cmap.name)
            self.assertEqual(0, image.norm.vmin)
            self.assertEqual(2, image.norm.vmax)
            self.assertTrue(isinstance(image.norm, Normalize))

    def test_apply_properties_applies_to_all_images_if_mixed_colorfill_plots_and_one_colorbar(self):
        # self.ws does not have common bins and has evenly spaced values on spectrum axis (gives Image)
        # Also create ws with common bins, evenly spaced bin edges and uneven values on spectrum axis (gives QuadMesh)
        ws = CreateWorkspace(
            DataX=list([0, 1, 2, 3, 4]) * 4,
            DataY=range(20),
            NSpec=4,
            OutputWorkspace="test_ws",
            VerticalAxisUnit="TOF",
            VerticalAxisValues=[1, 2, 4, 10],
        )
        fig = pcolormesh([self.ws, ws])
        self.assertTrue(isinstance(fig.axes[0].images[0], matplotlib.image.AxesImage))
        self.assertTrue(isinstance(fig.axes[1].collections[0], matplotlib.collections.QuadMesh))
        props = {"label": "New Label", "colormap": "jet", "vmin": 0, "vmax": 2, "scale": "Linear", "interpolation": "None"}
        mock_view = Mock(get_selected_image_name=lambda: "ws: (0, 0) - child0", get_properties=lambda: ImageProperties(props))
        presenter = self._generate_presenter(fig=fig, view=mock_view)
        presenter.apply_properties()

        images = datafunctions.get_images_from_figure(fig)
        for image in images:
            if image.colorbar:
                self.assertEqual("New Label", image.colorbar.ax.get_ylabel())

            self.assertEqual("jet", image.cmap.name)
            self.assertEqual(0, image.norm.vmin)
            self.assertEqual(2, image.norm.vmax)
            self.assertTrue(isinstance(image.norm, Normalize))


if __name__ == "__main__":
    unittest.main()
