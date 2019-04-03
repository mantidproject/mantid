# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

PYQT4 = False
IN_MANTIDPLOT = False
WITHOUT_GUI = False
try:
    from qtpy import PYQT4
except ImportError:
    pass  # it is already false
if PYQT4:
    try:
        import mantidplot
        IN_MANTIDPLOT = True
    except (Exception, Warning):
        pass
else:
    try:
        from mantidqt.plotting.functions import plot
    except ImportError:
        WITHOUT_GUI = True


def can_plot_beamcentrefinder():
    return not PYQT4 or IN_MANTIDPLOT


def _plot_quartiles_matplotlib(output_workspaces, sample_scatter):
    title = '{}_beam_centre_finder'.format(sample_scatter)
    ax_properties = {'xscale': 'log',
                     'yscale': 'log'}

    plot_kwargs = {"scalex": True,
                   "scaley": True}

    if not isinstance(output_workspaces, list):
        output_workspaces = [output_workspaces]

    if not WITHOUT_GUI:
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
    if not PYQT4:
        _plot_quartiles_matplotlib(output_workspaces, sample_scatter)
    elif IN_MANTIDPLOT:
        _plot_quartiles(output_workspaces, sample_scatter)
