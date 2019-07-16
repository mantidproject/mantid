# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import sys

IN_WORKBENCH = False
IN_MANTIDPLOT = False

try:
    import mantidplot
    IN_MANTIDPLOT = True
except (Exception, Warning):
    if "workbench.app.mainwindow" in sys.modules:
        IN_WORKBENCH = True
        try:
            from mantidqt.plotting.functions import plot
        except ImportError:
            pass


def can_plot_beamcentrefinder():
    return IN_WORKBENCH or IN_MANTIDPLOT


def _plot_quartiles_matplotlib(output_workspaces, sample_scatter):
    title = '{}_beam_centre_finder'.format(sample_scatter)
    ax_properties = {'xscale': 'log',
                     'yscale': 'log'}

    plot_kwargs = {"scalex": True,
                   "scaley": True}

    if not isinstance(output_workspaces, list):
        output_workspaces = [output_workspaces]

    plot(output_workspaces, wksp_indices=[0], ax_properties=ax_properties, overplot=True,
         plot_kwargs=plot_kwargs, window_title=title)


def _plot_quartiles(output_workspaces, sample_scatter):
    title = '{}_beam_centre_finder'.format(sample_scatter)
    graph_handle = mantidplot.plotSpectrum(output_workspaces, 0)
    graph_handle.activeLayer().logLogAxes()
    graph_handle.activeLayer().setTitle(title)
    graph_handle.setName(title)
    return graph_handle


def plot_workspace_quartiles(output_workspaces, sample_scatter):
    if IN_WORKBENCH:
        _plot_quartiles_matplotlib(output_workspaces, sample_scatter)
    elif IN_MANTIDPLOT:
        _plot_quartiles(output_workspaces, sample_scatter)
