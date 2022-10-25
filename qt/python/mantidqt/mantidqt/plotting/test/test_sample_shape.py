# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#

from unittest import TestCase, main
from unittest.mock import patch
import numpy as np
import matplotlib
matplotlib.use('AGG')  # noqa

from mantid.api import AnalysisDataService as ADS
from mantid.simpleapi import (CreateWorkspace, CreateSampleWorkspace, SetSample, LoadSampleShape, DeleteWorkspace,
                              LoadInstrument)
from mantidqt.plotting import sample_shape


def setup_workspace_shape_from_CSG_merged(workspace_name: str):
    CreateWorkspace(OutputWorkspace=workspace_name, DataX=[1, 1], DataY=[2, 2])
    merge_xml = ' \
    <cylinder id="stick"> \
    <centre-of-bottom-base x="-0.5" y="0.0" z="0.0" /> \
    <axis x="1.0" y="0.0" z="0.0" />  \
    <radius val="0.05" /> \
    <height val="1.0" /> \
    </cylinder> \
    \
    <sphere id="some-sphere"> \
    <centre x="0.7"  y="0.0" z="0.0" /> \
    <radius val="0.2" /> \
    </sphere> \
    \
    <rotate-all x="90" y="-45" z="0" /> \
    <algebra val="some-sphere (: stick)" /> \
    '
    SetSample(workspace_name, Geometry={'Shape': 'CSG', 'Value': merge_xml})


def setup_workspace_shape_from_mesh(workspace_name: str):
    CreateSampleWorkspace(OutputWorkspace=workspace_name)
    LoadSampleShape(InputWorkspace=workspace_name, OutputWorkspace=workspace_name, Filename="tube.stl")


def setup_workspace_sample_container_and_components_from_mesh(workspace_name: str):
    CreateWorkspace(OutputWorkspace=workspace_name, DataX=[1, 1], DataY=[2, 2])
    LoadInstrument(Workspace=workspace_name, RewriteSpectraMap=True, InstrumentName="Pearl")
    SetSample(workspace_name, Environment={'Name': 'Pearl'})


class PlotSampleShapeTest(TestCase):

    def tearDown(self) -> None:
        if "ws_shape" in ADS:
            DeleteWorkspace("ws_shape")

    def test_CSG_merged_shape_is_valid(self):
        setup_workspace_shape_from_CSG_merged("ws_shape")
        ws_shape = ADS.retrieve("ws_shape")
        self.assertTrue(ws_shape.sample().getShape())
        self.assertTrue(sample_shape.get_valid_sample_shape_from_workspace("ws_shape"))

    def test_CSG_sphere_is_valid(self):
        CreateSampleWorkspace(OutputWorkspace="ws_shape")
        ws_shape = ADS.retrieve("ws_shape")
        self.assertTrue(ws_shape.sample().getShape())
        self.assertTrue(sample_shape.get_valid_sample_shape_from_workspace("ws_shape"))

    def test_CSG_empty_shape_is_not_valid(self):
        CreateWorkspace(OutputWorkspace="ws_shape", DataX=[1, 1], DataY=[2, 2])
        ws_shape = ADS.retrieve("ws_shape")
        self.assertTrue(ws_shape.sample().getShape())
        self.assertFalse(sample_shape.get_valid_sample_shape_from_workspace("ws_shape"))

    def test_mesh_is_valid(self):
        setup_workspace_shape_from_mesh("ws_shape")
        ws_shape = ADS.retrieve("ws_shape")
        self.assertTrue(ws_shape.sample().getShape())
        self.assertTrue(sample_shape.get_valid_sample_shape_from_workspace("ws_shape"))

    def test_plot_created_for_CSG_sphere_sample_only(self):
        CreateSampleWorkspace(OutputWorkspace="ws_shape")
        shape_plot = sample_shape.plot_sample_shape("ws_shape")
        self.assertTrue(shape_plot)

    def test_plot_created_for_CSG_merged_sample_only(self):
        setup_workspace_shape_from_CSG_merged("ws_shape")
        shape_plot = sample_shape.plot_sample_shape("ws_shape")
        self.assertTrue(shape_plot)

    def test_plot_created_for_mesh_sample_only(self):
        setup_workspace_shape_from_mesh("ws_shape")
        shape_plot_axes = sample_shape.plot_sample_shape("ws_shape")
        self.assertTrue(shape_plot_axes)

    def test_plot_created_for_mesh_container_and_components(self):
        setup_workspace_sample_container_and_components_from_mesh("ws_shape")
        ws_shape = ADS.retrieve("ws_shape")
        shape_plot_figure = sample_shape.plot_sample_shape("ws_shape")
        self.assertTrue(shape_plot_figure)
        components_added_to_plot = sample_shape.plot_container_and_components(ws_shape, shape_plot_figure)
        self.assertTrue(components_added_to_plot)

    @patch("mantidqt.plotting.sample_shape.plot_container_and_components")
    def test_add_components_if_environment(self, mock_plot_container_and_components):
        setup_workspace_sample_container_and_components_from_mesh("ws_shape")
        sample_shape.plot_sample_shape("ws_shape")
        self.assertEqual(1, mock_plot_container_and_components.call_count)

    @patch("mantidqt.plotting.sample_shape.plot_container_and_components")
    def test_do_not_add_components_if_no_environment(self, mock_plot_container_and_components):
        setup_workspace_shape_from_mesh("ws_shape")
        sample_shape.plot_sample_shape("ws_shape")
        self.assertEqual(0, mock_plot_container_and_components.call_count)

    @patch("mantidqt.plotting.sample_shape.call_set_mesh_axes_equal")
    def test_call_axes_equal_once_for_sample_only(self, mock_call_set_mesh_axes_equal):
        setup_workspace_shape_from_mesh("ws_shape")
        sample_shape.plot_sample_shape("ws_shape")
        self.assertEqual(1, mock_call_set_mesh_axes_equal.call_count)

    @patch("mantidqt.plotting.sample_shape.call_set_mesh_axes_equal")
    def test_call_axes_equal_once_for_sample_and_components(self, mock_call_set_mesh_axes_equal):
        setup_workspace_sample_container_and_components_from_mesh("ws_shape")
        ws_shape = ADS.retrieve("ws_shape")
        shape_plot_figure = sample_shape.plot_sample_shape("ws_shape")
        sample_shape.plot_container_and_components(ws_shape, shape_plot_figure)
        self.assertEqual(1, mock_call_set_mesh_axes_equal.call_count)

    def test_get_overall_limits_for_sample_only(self):
        setup_workspace_shape_from_mesh("ws_shape")
        ws_shape = ADS.retrieve("ws_shape")
        [minimum, maximum] = sample_shape.overall_limits_for_all_meshes(ws_shape)
        self.assertEqual([-0.15, 0.1], [minimum, maximum])

    def test_get_overall_limits_for_sample_only_PEARL(self):
        setup_workspace_sample_container_and_components_from_mesh("ws_shape")
        ws_shape = ADS.retrieve("ws_shape")
        [minimum, maximum] = sample_shape.overall_limits_for_all_meshes(ws_shape, include_components=False)
        self.assertEqual([-3.289720742031932e-06, 0.0017999805212020874], [minimum, maximum])

    def test_get_overall_limits_for_sample_and_components_PEARL(self):
        setup_workspace_sample_container_and_components_from_mesh("ws_shape")
        ws_shape = ADS.retrieve("ws_shape")
        [minimum, maximum] = sample_shape.overall_limits_for_all_meshes(ws_shape)
        self.assertEqual([-0.035900001525878904, 0.035900001525878904], [minimum, maximum])

    def test_set_axes_to_largest_mesh(self):
        setup_workspace_shape_from_mesh("ws_shape")
        ws_shape = ADS.retrieve("ws_shape")
        [minimum, maximum] = sample_shape.overall_limits_for_all_meshes(ws_shape)
        self.assertEqual([-0.15, 0.1], [minimum, maximum])

    def test_overall_limits_for_every_axis(self):
        self.assertEqual((3, 9), sample_shape.overall_limits_for_every_axis([np.array([3, 5, 7]),
                                                                             np.array([4, 6, 8]),
                                                                             np.array([5, 7, 9])]))

    def test_greater_limits(self):
        self.assertEqual((1, 7), sample_shape.greater_limits(1, 4, 5, 7))
        self.assertEqual((1, 7), sample_shape.greater_limits(4, 5, 1, 7))
        self.assertEqual((1, 7), sample_shape.greater_limits(1, 7, 4, 5))
        self.assertEqual((1, 7), sample_shape.greater_limits(4, 7, 1, 5))


if __name__ == '__main__':
    main()
