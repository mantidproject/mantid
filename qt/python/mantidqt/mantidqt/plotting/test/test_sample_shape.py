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

import mantid
from mantid.api import AnalysisDataService as ADS
from mantid.simpleapi import (CreateWorkspace, CreateSampleWorkspace, SetSample, LoadSampleShape, DeleteWorkspace,
                              LoadInstrument)
from mantidqt.plotting import sample_shape

workspace_name = "ws_shape"
test_files_path = mantid.config.getString('defaultsave.directory')


def setup_workspace_shape_from_CSG_merged():
    CreateWorkspace(OutputWorkspace="ws_shape", DataX=[1, 1], DataY=[2, 2])
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
    SetSample("ws_shape", Geometry={'Shape': 'CSG', 'Value': merge_xml})
    return ADS.retrieve(workspace_name)


def setup_workspace_sample_and_container_CSG():
    CreateWorkspace(OutputWorkspace=workspace_name, DataX=[1, 1], DataY=[2, 2])
    SetSample(workspace_name,
              Geometry={'Shape': 'Cylinder', 'Height': 4.0,
                        'Radius': 2.0, 'Center': [0., 0., 0.]},
              Material={'ChemicalFormula': '(Li7)2-C-H4-N-Cl6',
                        'NumberDensity': 0.1},
              ContainerGeometry={'Shape': 'HollowCylinder', 'Height': 4.0,
                                 'InnerRadius': 2.0, 'OuterRadius': 2.3,
                                 'Center': [0., 0., 0.]},
              ContainerMaterial={'ChemicalFormula': 'Al',
                                 'NumberDensity': 0.01})
    return ADS.retrieve(workspace_name)


def setup_workspace_container_CSG():
    CreateWorkspace(OutputWorkspace=workspace_name, DataX=[1, 1], DataY=[2, 2])

    SetSample(workspace_name,
              ContainerGeometry={'Shape': 'HollowCylinder', 'Height': 4.0,
                                 'InnerRadius': 2.0, 'OuterRadius': 2.3,
                                 'Center': [0., 0., 0.]},
              ContainerMaterial={'ChemicalFormula': 'Al',
                                 'NumberDensity': 0.01})
    return ADS.retrieve(workspace_name)


def setup_workspace_shape_from_mesh():
    CreateSampleWorkspace(OutputWorkspace="ws_shape")
    LoadSampleShape(InputWorkspace="ws_shape", OutputWorkspace="ws_shape", Filename="tube.stl")
    return ADS.retrieve("ws_shape")


def setup_workspace_sample_container_and_components_from_mesh():
    workspace = CreateWorkspace(OutputWorkspace="ws_shape", DataX=[1, 1], DataY=[2, 2])
    LoadInstrument(Workspace=workspace, RewriteSpectraMap=True, InstrumentName="Pearl")
    SetSample(workspace, Environment={'Name': 'Pearl'})
    return workspace


class PlotSampleShapeTest(TestCase):

    def tearDown(self) -> None:
        if "ws_shape" in ADS:
            DeleteWorkspace(workspace_name)

    def test_CSG_merged_shape_is_valid(self):
        workspace = setup_workspace_shape_from_CSG_merged()
        self.assertTrue(workspace.sample().getShape())
        self.assertTrue(sample_shape.get_valid_sample_shape_from_workspace(workspace))

    def test_CSG_sphere_is_valid(self):
        workspace = CreateSampleWorkspace(OutputWorkspace="ws_shape")
        self.assertTrue(workspace.sample().getShape())
        self.assertTrue(sample_shape.get_valid_sample_shape_from_workspace(workspace))

    def test_CSG_empty_shape_is_not_valid(self):
        workspace = CreateWorkspace(OutputWorkspace="ws_shape", DataX=[1, 1], DataY=[2, 2])
        self.assertTrue(workspace.sample().getShape())
        self.assertFalse(sample_shape.get_valid_sample_shape_from_workspace(workspace))

    def test_mesh_is_valid(self):
        workspace = setup_workspace_shape_from_mesh()
        self.assertTrue(workspace.sample().getShape())
        self.assertTrue(sample_shape.get_valid_sample_shape_from_workspace(workspace))

    def test_container_valid(self):
        workspace = setup_workspace_container_CSG()
        self.assertTrue(sample_shape.get_valid_container_shape_from_workspace(workspace))

    # def test_container_invalid(self):
    #     CreateWorkspace(OutputWorkspace=workspace_name, DataX=[1, 1], DataY=[2, 2])
    #
    #     SetSample(workspace_name,
    #           ContainerGeometry={'Shape': 'HollowCylinder', 'Height': 0.0,
    #                              'InnerRadius': 2.0, 'OuterRadius': 2.3,
    #                              'Center': [0., 0., 0.]},
    #           ContainerMaterial={'ChemicalFormula': 'Al',
    #                              'NumberDensity': 0.01})
    #     ADS.retrieve(workspace_name)

    def test_components_valid(self):
        workspace = setup_workspace_sample_container_and_components_from_mesh()
        self.assertTrue(sample_shape.get_valid_component_shape_from_workspace(workspace, 1))

    # def test_components_invalid(self):

    def test_plot_created_for_CSG_sphere_sample_only(self):
        CreateSampleWorkspace(OutputWorkspace="ws_shape")
        shape_plot = sample_shape.plot_sample_container_and_components("ws_shape")
        self.assertTrue(shape_plot)

    def test_plot_created_for_CSG_merged_sample_only(self):
        workspace = setup_workspace_shape_from_CSG_merged()
        shape_plot = sample_shape.plot_sample_container_and_components(workspace.name())
        self.assertTrue(shape_plot)

    def test_plot_created_for_mesh_sample_only(self):
        workspace = setup_workspace_shape_from_mesh()
        shape_plot_axes = sample_shape.plot_sample_container_and_components(workspace.name())
        self.assertTrue(shape_plot_axes)

    def test_plot_created_for_mesh_container_and_components(self):
        workspace = setup_workspace_sample_container_and_components_from_mesh()
        shape_plot_figure = sample_shape.plot_sample_container_and_components(workspace.name())
        self.assertTrue(shape_plot_figure)
        container_added_to_plot = sample_shape.plot_container(workspace, shape_plot_figure)
        components_added_to_plot = sample_shape.plot_components(workspace, shape_plot_figure)
        self.assertTrue(container_added_to_plot)
        self.assertTrue(components_added_to_plot)

    @patch("mantidqt.plotting.sample_shape.set_perspective")
    @patch("mantidqt.plotting.sample_shape.set_axes_labels")
    @patch("mantidqt.plotting.sample_shape.show_the_figure")
    @patch("mantidqt.plotting.sample_shape.plot_container")
    @patch("mantidqt.plotting.sample_shape.plot_components")
    def test_add_components_if_environment(self, mock_plot_container, mock_plot_components,
                                           mock_show_the_figure, mock_set_axes_labels,
                                           mock_set_perspective):
        workspace = setup_workspace_sample_container_and_components_from_mesh()
        sample_shape.plot_sample_container_and_components(workspace.name())
        self.assertEqual(1, mock_plot_container.call_count)
        self.assertEqual(1, mock_plot_components.call_count)
        self.assertEqual(1, mock_show_the_figure.call_count)
        self.assertEqual(1, mock_set_axes_labels.call_count)
        self.assertEqual(1, mock_set_perspective.call_count)

    @patch("mantidqt.plotting.sample_shape.set_perspective")
    @patch("mantidqt.plotting.sample_shape.set_axes_labels")
    @patch("mantidqt.plotting.sample_shape.show_the_figure")
    @patch("mantidqt.plotting.sample_shape.plot_container")
    @patch("mantidqt.plotting.sample_shape.plot_components")
    def test_do_not_add_components_if_no_environment(self, mock_plot_container, mock_plot_components,
                                                     mock_show_the_figure, mock_set_axes_labels,
                                                     mock_set_perspective):
        workspace = setup_workspace_shape_from_mesh()
        sample_shape.plot_sample_container_and_components(workspace.name())
        self.assertEqual(0, mock_plot_container.call_count)
        self.assertEqual(0, mock_plot_components.call_count)
        self.assertEqual(1, mock_show_the_figure.call_count)
        self.assertEqual(1, mock_set_axes_labels.call_count)
        self.assertEqual(1, mock_set_perspective.call_count)

    @patch("mantidqt.plotting.sample_shape.set_perspective")
    @patch("mantidqt.plotting.sample_shape.set_axes_labels")
    @patch("mantidqt.plotting.sample_shape.show_the_figure")
    @patch("mantidqt.plotting.sample_shape.call_set_mesh_axes_equal")
    def test_call_axes_equal_once_for_sample_only(self, mock_call_set_mesh_axes_equal,
                                                  mock_show_the_figure, mock_set_axes_labels,
                                                  mock_set_perspective):
        workspace = setup_workspace_shape_from_mesh()
        sample_shape.plot_sample_container_and_components(workspace.name())
        self.assertEqual(1, mock_call_set_mesh_axes_equal.call_count)
        self.assertEqual(1, mock_show_the_figure.call_count)
        self.assertEqual(1, mock_set_axes_labels.call_count)
        self.assertEqual(1, mock_set_perspective.call_count)

    @patch("mantidqt.plotting.sample_shape.set_perspective")
    @patch("mantidqt.plotting.sample_shape.set_axes_labels")
    @patch("mantidqt.plotting.sample_shape.show_the_figure")
    @patch("mantidqt.plotting.sample_shape.call_set_mesh_axes_equal")
    def test_call_axes_equal_once_for_sample_and_components(self, mock_call_set_mesh_axes_equal,
                                                            mock_show_the_figure, mock_set_axes_labels,
                                                            mock_set_perspective):
        workspace = setup_workspace_sample_container_and_components_from_mesh()
        shape_plot_figure = sample_shape.plot_sample_container_and_components(workspace.name())
        sample_shape.plot_container(workspace, shape_plot_figure)
        sample_shape.plot_components(workspace, shape_plot_figure)
        self.assertEqual(1, mock_call_set_mesh_axes_equal.call_count)
        self.assertEqual(1, mock_show_the_figure.call_count)
        self.assertEqual(1, mock_set_axes_labels.call_count)
        self.assertEqual(1, mock_set_perspective.call_count)

    def test_get_overall_limits_for_sample_only(self):
        workspace = setup_workspace_shape_from_mesh()
        [minimum, maximum] = sample_shape.overall_limits_for_all_meshes(workspace)
        self.assertEqual([-0.15, 0.15], [minimum, maximum])

    def test_get_overall_limits_for_sample_only_PEARL(self):
        workspace = setup_workspace_sample_container_and_components_from_mesh()
        [minimum, maximum] = sample_shape.overall_limits_for_all_meshes(workspace, include_components=False)
        self.assertEqual([-0.002939387798309326, 0.002939387798309326], [minimum, maximum])

    def test_get_overall_limits_for_sample_and_components_PEARL(self):
        workspace = setup_workspace_sample_container_and_components_from_mesh()
        [minimum, maximum] = sample_shape.overall_limits_for_all_meshes(workspace)
        self.assertEqual([-0.035900001525878904, 0.035900001525878904], [minimum, maximum])

    def test_set_axes_to_largest_mesh(self):
        workspace = setup_workspace_shape_from_mesh()
        [minimum, maximum] = sample_shape.overall_limits_for_all_meshes(workspace)
        self.assertEqual([-0.15, 0.15], [minimum, maximum])

    def test_overall_limits_for_every_axis(self):
        self.assertEqual((3, 9),
                         sample_shape.overall_limits_for_every_axis(np.array([np.array([3, 5, 7]),
                                                                              np.array([4, 6, 8]),
                                                                              np.array([5, 7, 9]),
                                                                              np.array([3, 4, 5])])
                                                                    ))

    def test_greater_limits(self):
        self.assertEqual((1, 7), sample_shape.greater_limits(1, 4, 5, 7))
        self.assertEqual((1, 7), sample_shape.greater_limits(4, 5, 1, 7))
        self.assertEqual((1, 7), sample_shape.greater_limits(1, 7, 4, 5))
        self.assertEqual((1, 7), sample_shape.greater_limits(4, 7, 1, 5))

    # Sample and Container
    def test_sample_valid_and_container_invalid(self):
        workspace = setup_workspace_shape_from_CSG_merged()
        figure = sample_shape.plot_sample_container_and_components(workspace.name())
        self.assertTrue(figure)
        self.assertEqual(f'Sample: {workspace.name()}', figure.gca().get_title())

    def test_sample_container_valid(self):
        workspace = setup_workspace_sample_and_container_CSG()
        figure = sample_shape.plot_sample_container_and_components(workspace.name())
        self.assertTrue(figure)
        self.assertEqual(f'Sample and Container: {workspace.name()}', figure.gca().get_title())

    def test_sample_invalid_container_valid(self):
        workspace = setup_workspace_container_CSG()
        figure = sample_shape.plot_sample_container_and_components(workspace.name())
        self.assertTrue(figure)
        self.assertEqual(f'Container: {workspace.name()}', figure.gca().get_title())

    def test_sample_and_container_invalid(self):
        workspace = CreateWorkspace(OutputWorkspace="ws_shape", DataX=[1, 1], DataY=[2, 2])
        figure = sample_shape.plot_sample_container_and_components(workspace.name())
        self.assertFalse(figure)

    # Sample, Container and Components
    def test_sample_container_and_components_valid(self):
        workspace = setup_workspace_sample_container_and_components_from_mesh()
        figure = sample_shape.plot_sample_container_and_components(workspace.name())
        self.assertTrue(figure)
        self.assertEqual(f'Sample, Container and Components: {workspace.name()}', figure.gca().get_title())

    def test_plot_title_for_other_component_arrangements_that_are_less_likely(self):
        self.assertEqual('Container and Components: workspace_name',
                         sample_shape.construct_title(False, True, True, "workspace_name"))
        self.assertEqual('Sample and Components: workspace_name',
                         sample_shape.construct_title(True, False, True, "workspace_name"))
        self.assertEqual('Components: workspace_name',
                         sample_shape.construct_title(False, False, True, "workspace_name"))


if __name__ == '__main__':
    main()
