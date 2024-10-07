# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import sys

IN_WORKBENCH = False

if "workbench.app" in sys.modules:
    try:
        from mantidqt.plotting.functions import plot

        IN_WORKBENCH = True
    except ImportError:
        pass


def can_plot_beamcentrefinder():
    return IN_WORKBENCH


def _plot_quartiles_matplotlib(output_workspaces, sample_scatter):
    title = "{}_beam_centre_finder".format(sample_scatter)
    ax_properties = {"xscale": "log", "yscale": "log"}

    plot_kwargs = {"scalex": True, "scaley": True}

    if not isinstance(output_workspaces, list):
        output_workspaces = [output_workspaces]

    assert output_workspaces, "No workspaces were passed into plotting"

    plot(output_workspaces, wksp_indices=[0], ax_properties=ax_properties, overplot=True, plot_kwargs=plot_kwargs, window_title=title)


def plot_workspace_quartiles(output_workspaces, sample_scatter):
    if IN_WORKBENCH:
        _plot_quartiles_matplotlib(output_workspaces, sample_scatter)
