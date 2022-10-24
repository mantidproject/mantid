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
from mantid.simpleapi import CreateWorkspace, SetSample
from mantidqt.plotting.sample_shape import (get_valid_sample_shape_from_workspace)


def setup_workspace_with_CSG_merged_shape(workspace_name: str):

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


class PlotSampleShapeTest(TestCase):

    def test_microphone_is_valid(self):
        setup_workspace_with_CSG_merged_shape("ws_shape")
        ws_shape = ADS.retrieve("ws_shape")
        self.assertTrue(ws_shape.sample().getShape())
        self.assertTrue(get_valid_sample_shape_from_workspace("ws_shape"))

    def test_empty_shape_is_not_valid(self):
        CreateWorkspace(OutputWorkspace="ws_no_shape", DataX=[1, 1], DataY=[2, 2])
        ws_no_shape = ADS.retrieve("ws_no_shape")
        self.assertTrue(ws_no_shape.sample().getShape())
        self.assertFalse(get_valid_sample_shape_from_workspace("ws_no_shape"))


if __name__ == '__main__':
    main()
