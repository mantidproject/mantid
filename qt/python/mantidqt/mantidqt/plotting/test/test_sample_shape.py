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
import matplotlib
matplotlib.use('AGG')  # noqa
# import matplotlib.pyplot as plt

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
    LoadInstrument(Workspace=workspace_name,RewriteSpectraMap=True,InstrumentName="Pearl")
    SetSample(workspace_name, Environment={'Name': 'Pearl'})


class PlotSampleShapeTest(TestCase):

    def tearDown(self) -> None:
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

    def test_plot_created_for_CSG_sphere(self):
        CreateSampleWorkspace(OutputWorkspace="ws_shape")
        shape_plot = sample_shape.plot_sample_shape("ws_shape")
        self.assertTrue(shape_plot)

    def test_plot_created_for_CSG_merged(self):
        setup_workspace_shape_from_CSG_merged("ws_shape")
        shape_plot = sample_shape.plot_sample_shape("ws_shape")
        self.assertTrue(shape_plot)

    def test_plot_created_for_mesh(self):
        setup_workspace_shape_from_mesh("ws_shape")
        shape_plot_axes = sample_shape.plot_sample_shape("ws_shape")
        self.assertTrue(shape_plot_axes)

    def test_plot_created_for_mesh_container_and_components(self):
        setup_workspace_sample_container_and_components_from_mesh("ws_shape")
        shape_plot_figure = sample_shape.plot_sample_shape("ws_shape")
        self.assertTrue(shape_plot_figure)
        components_added_to_plot = sample_shape.plot_container_and_components("ws_shape", shape_plot_figure)
        self.assertTrue(components_added_to_plot)

    # def test_do_not_add_components_if_no_environment(self):


if __name__ == '__main__':
    main()
