# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
#
from functools import partial
from qtpy.QtWidgets import QApplication, QVBoxLayout

from mantid.api import AnalysisDataService, WorkspaceGroup
from mantid.kernel import logger
from mantidqt.plotting import functions
from mantidqt.plotting.functions import (
    can_overplot,
    pcolormesh,
    plot,
    plot_from_names,
    plot_md_ws_from_names,
    superplot_from_names,
    superplot_with_errors_from_names,
)
from mantidqt.plotting import sample_shape
from mantid.plots.utility import MantidAxType
from mantid.simpleapi import CreateDetectorTable
from mantidqt.utils.asynchronous import BlockingAsyncTaskWithCallback
from mantidqt.widgets.instrumentview.presenter import InstrumentViewPresenter
from mantidqt.widgets.samplelogs.presenter import SampleLogs
from mantidqt.widgets.sliceviewer.presenters.presenter import SliceViewer
from mantidqt.widgets.workspacedisplay.matrix.presenter import MatrixWorkspaceDisplay
from mantidqt.widgets.workspacedisplay.table.presenter import TableWorkspaceDisplay
from mantidqt.widgets.workspacewidget.algorithmhistorywindow import AlgorithmHistoryWindow
from mantidqt.widgets.samplematerialdialog.samplematerial_presenter import SampleMaterialDialogPresenter
from mantidqt.widgets.workspacewidget.workspacetreewidget import WorkspaceTreeWidget
from workbench.config import CONF
from workbench.plugins.base import PluginWidget
from workbench.config import get_window_config


class WorkspaceWidget(PluginWidget):
    """Provides a Workspace Widget for workspace manipulation"""

    def __init__(self, parent):
        super(WorkspaceWidget, self).__init__(parent)

        self._ads = AnalysisDataService.Instance()

        # layout
        self.workspacewidget = WorkspaceTreeWidget()
        layout = QVBoxLayout()
        layout.addWidget(self.workspacewidget)
        self.setLayout(layout)

        # behaviour
        self.workspacewidget.plotSpectrumClicked.connect(partial(self._do_plot_spectrum, errors=False, overplot=False))

        self.workspacewidget.plotMDHistoClicked.connect(partial(self._do_plot_1d_md, errors=False, overplot=False))
        self.workspacewidget.overplotMDHistoClicked.connect(partial(self._do_plot_1d_md, errors=False, overplot=True))
        self.workspacewidget.plotMDHistoWithErrorsClicked.connect(partial(self._do_plot_1d_md, errors=True, overplot=False))
        self.workspacewidget.overplotMDHistoWithErrorsClicked.connect(partial(self._do_plot_1d_md, errors=True, overplot=True))

        self.workspacewidget.plotBinClicked.connect(partial(self._do_plot_bin, errors=False, overplot=False))
        self.workspacewidget.overplotSpectrumClicked.connect(partial(self._do_plot_spectrum, errors=False, overplot=True))
        self.workspacewidget.plotSpectrumWithErrorsClicked.connect(partial(self._do_plot_spectrum, errors=True, overplot=False))
        self.workspacewidget.overplotSpectrumWithErrorsClicked.connect(partial(self._do_plot_spectrum, errors=True, overplot=True))
        self.workspacewidget.plotColorfillClicked.connect(self._do_plot_colorfill)
        self.workspacewidget.sampleLogsClicked.connect(self._do_sample_logs)
        self.workspacewidget.sliceViewerClicked.connect(self._do_slice_viewer)
        self.workspacewidget.showDataClicked.connect(self._do_show_data)
        self.workspacewidget.showInstrumentClicked.connect(self._do_show_instrument)
        self.workspacewidget.showAlgorithmHistoryClicked.connect(self._do_show_algorithm_history)
        self.workspacewidget.showDetectorsClicked.connect(self._do_show_detectors)
        self.workspacewidget.plotAdvancedClicked.connect(partial(self._do_plot_spectrum, errors=False, overplot=False, advanced=True))
        self.workspacewidget.plotSurfaceClicked.connect(partial(self._do_plot_3D, plot_type="surface"))
        self.workspacewidget.plotWireframeClicked.connect(partial(self._do_plot_3D, plot_type="wireframe"))
        self.workspacewidget.plotContourClicked.connect(partial(self._do_plot_3D, plot_type="contour"))
        self.workspacewidget.sampleMaterialClicked.connect(self._do_sample_material)
        self.workspacewidget.sampleShapeClicked.connect(self._show_sample_shape)
        self.workspacewidget.superplotClicked.connect(self._do_superplot)
        self.workspacewidget.superplotWithErrsClicked.connect(self._do_superplot_with_errors)
        self.workspacewidget.superplotBinsClicked.connect(self._do_superplot_bins)
        self.workspacewidget.superplotBinsWithErrsClicked.connect(self._do_superplot_bins_with_errors)
        self.workspacewidget.contextMenuAboutToShow.connect(self._on_context_menu)

        self.workspacewidget.workspaceDoubleClicked.connect(self._action_double_click_workspace)

    # ----------------- Plugin API --------------------

    def register_plugin(self):
        self.main.add_dockwidget(self)

    def get_plugin_title(self):
        return "Workspaces"

    def readSettings(self, _):
        pass

    def writeSettings(self, _):
        pass

    # ----------------- Behaviour --------------------

    def _on_context_menu(self):
        """
        Triggered when the context menu is about to be displayed.
        """
        ableToOverplot = can_overplot()
        self.workspacewidget.setOverplotDisabled(not ableToOverplot)

    def _do_superplot(self, names):
        """
        Open an empty plot with the superplot started and the selected
        workspaces selected.

        :param names: A list of workspace names
        """
        try:
            superplot_from_names(names, plot_kwargs=None)
        except RuntimeError as re:
            logger.error(str(re))

    def _do_superplot_with_errors(self, names):
        """
        Open an empty plot with the superplot started and the selected
        workspaces selected.

        :param names: A list of workspace names
        """
        try:
            superplot_with_errors_from_names(names, plot_kwargs=None)
        except RuntimeError as re:
            logger.error(str(re))

    def _do_superplot_bins(self, names):
        """
        Open an empty plot with the superplot started on bins mode.

        :param names: A list of workspace names
        """
        try:
            plot_kwargs = {"axis": MantidAxType.BIN}
            superplot_from_names(names, plot_kwargs=plot_kwargs)
        except RuntimeError as re:
            logger.error(str(re))

    def _do_superplot_bins_with_errors(self, names):
        """
        Open an empty plot with the superplot started on bins mode with error
        bars.

        :param names: A list of workspace names
        """
        try:
            plot_kwargs = {"axis": MantidAxType.BIN}
            superplot_with_errors_from_names(names, plot_kwargs=plot_kwargs)
        except RuntimeError as re:
            logger.error(str(re))

    def _do_plot_spectrum(self, names, errors, overplot, advanced=False):
        """
        Plot spectra from the selected workspaces

        :param names: A list of workspace names
        :param errors: If true then error bars will be plotted on the points
        :param overplot: If true then the add to the current figure if one
                         exists and it is a compatible figure
        :param advanced: If true then the advanced options will be shown in
                         the spectra selector dialog.
        """
        try:
            plot_from_names(names, errors, overplot, advanced=advanced)
        except RuntimeError as re:
            logger.error(str(re))

    def _do_plot_1d_md(self, names, errors, overplot):
        """
        Plot 1D IMDHistoWorlspaces

        :param names: list of workspace names
        :param errors: boolean.  if true, the error bar will be plotted
        :param overplot: boolean.  If true, then add these plots to the current figure if one exists
                                   and it is a compatible figure
        :return:
        """
        try:
            plot_md_ws_from_names(names, errors, overplot)
        except RuntimeError as re:
            logger.error(str(re))

    def _do_plot_bin(self, names, errors, overplot):
        """
        Plot a single bin from the selected workspaces

        :param names: A list of workspace names
        :param errors: If true then error bars will be plotted on the points
        :param overplot: If true then the add to the current figure if one
                         exists and it is a compatible figure
        """
        plot_kwargs = {"axis": MantidAxType.BIN}
        plot(
            self._ads.retrieveWorkspaces(names, unrollGroups=True),
            errors=errors,
            overplot=overplot,
            wksp_indices=[0],
            plot_kwargs=plot_kwargs,
        )

    def _do_plot_colorfill(self, names):
        """
        Plot a colorfill from the selected workspaces

        :param names: A list of workspace names
        :param contour: An optional bool for whether to draw contour lines.
        """
        try:
            pcolormesh(names)
        except BaseException:
            import traceback

            traceback.print_exc()

    def _do_plot_3D(self, workspaces, plot_type):
        """
        Make a 3D plot from the selected workspace.

        :param workspaces: A list of workspace names.
        :param plot_type: The type of 3D plot, either 'surface', 'wireframe', or 'contour'.
        """
        plot_function = getattr(functions, f"plot_{plot_type}", None)

        if plot_function is None:
            return

        try:
            plot_function(workspaces)
        except RuntimeError as re:
            logger.error(str(re))

    def _do_sample_logs(self, names):
        """
        Show the sample log window for the given workspaces

        :param names: A list of workspace names
        """
        parent, flags = get_window_config()
        for ws in self._ads.retrieveWorkspaces(names, unrollGroups=True):
            try:
                SampleLogs(ws=ws, parent=parent, window_flags=flags)
            except Exception as exception:
                logger.warning("Could not open sample logs for workspace '{}'." "".format(ws.name()))
                logger.warning("{}: {}".format(type(exception).__name__, exception))

    def _do_slice_viewer(self, names):
        """
        Show the sliceviewer window for the given workspaces

        :param names: A list of workspace names
        """
        parent, flags = get_window_config()
        for ws in self._ads.retrieveWorkspaces(names, unrollGroups=True):
            try:
                y = ws.getYDimension()
                if y.getName() == "sample":
                    spec_xvals = [ws.getYDimension().getX(ispec) for ispec in range(ws.getNumberHistograms())]
                    # Check if the q values are sorted, if not display error
                    if sorted(spec_xvals) != spec_xvals:
                        logger.warning("Warning: Invalid Data Format. Consider sorting by sample number and try again")

                presenter = SliceViewer(ws=ws, conf=CONF, parent=parent, window_flags=flags)
                presenter.view.show()
            except Exception as exception:
                logger.warning("Could not open slice viewer for workspace '{}'." "".format(ws.name()))
                logger.warning("{}: {}".format(type(exception).__name__, exception))

    def _do_show_instrument(self, names):
        """
        Show an instrument widget for the given workspaces

        :param names: A list of workspace names
        """
        parent, flags = get_window_config()
        for ws in self._ads.retrieveWorkspaces(names, unrollGroups=True):
            if ws.getInstrument().getName():
                try:
                    presenter = InstrumentViewPresenter(ws, parent=parent, window_flags=flags)
                    presenter.show_view()
                except Exception as exception:
                    logger.warning("Could not show instrument for workspace " "'{}':\n{}\n".format(ws.name(), exception))
            else:
                logger.warning("Could not show instrument for workspace '{}':" "\nNo instrument available.\n" "".format(ws.name()))

    def _do_show_data(self, names):
        # local import to allow this module to be imported without pyplot being imported
        import matplotlib.pyplot

        parent, flags = get_window_config()
        # Process group peak workspaces first and remove them from the list
        for ws in self._ads.retrieveGroupPeaksWorkspaces(names):
            try:
                TableWorkspaceDisplay.supports(ws)
                presenter = TableWorkspaceDisplay(ws, plot=matplotlib.pyplot, parent=parent, window_flags=flags, group=True)
                presenter.show_view()
                names.remove(ws.name())
            except ValueError as e:
                logger.error(str(e))
                logger.error(
                    "Could not open workspace: {0} with neither " "MatrixWorkspaceDisplay nor TableWorkspaceDisplay." "".format(ws.name())
                )

        for ws in self._ads.retrieveWorkspaces(names, unrollGroups=True):
            try:
                MatrixWorkspaceDisplay.supports(ws)
                # the plot function is being injected in the presenter
                # this is done so that the plotting library is mockable in testing
                presenter = MatrixWorkspaceDisplay(ws, plot=plot, parent=parent, window_flags=flags)
                presenter.show_view()
            except ValueError:
                try:
                    TableWorkspaceDisplay.supports(ws)
                    presenter = TableWorkspaceDisplay(ws, plot=matplotlib.pyplot, parent=parent, window_flags=flags, batch=True)
                    presenter.show_view()
                except ValueError:
                    logger.error(
                        "Could not open workspace: {0} with neither " "MatrixWorkspaceDisplay nor TableWorkspaceDisplay.".format(ws.name())
                    )

    def _do_show_algorithm_history(self, names):
        for name in names:
            if not isinstance(self._ads.retrieve(name), WorkspaceGroup):
                try:
                    AlgorithmHistoryWindow(self, name).show()
                except Exception as exception:
                    logger.warning("Could not open history of '{}'. " "".format(name))
                    logger.warning("{}: {}".format(type(exception).__name__, exception))

    def _do_show_detectors(self, names):
        successful_workspaces = []
        for ws in self._ads.retrieveWorkspaces(names, unrollGroups=True):
            try:
                task = BlockingAsyncTaskWithCallback(self._run_create_detector_table, [ws], blocking_cb=QApplication.processEvents)
                task.start()
            except RuntimeError:
                continue
            else:
                successful_workspaces.append(ws.name())

        self._do_show_data(list(map(lambda x: x + "-Detectors", successful_workspaces)))

    def _run_create_detector_table(self, ws):
        CreateDetectorTable(InputWorkspace=ws)

    def _do_sample_material(self, names):
        """
        Show a sample material dialog for the workspace for names[0]. It only makes sense
        to show the dialog for a single workspace, and the context menu option should
        only be available if a single workspace is selected.

        :param names: A list of workspace names
        """
        # It only makes sense to show the sample material dialog with one workspace
        # selected. The context menu should only add the sample material action
        # if there's only one workspace selected.
        if len(names) == 1:
            try:
                workspace = self._ads.retrieve(names[0])
                presenter = SampleMaterialDialogPresenter(workspace, parent=self)
                presenter.show_view()
            except Exception as exception:
                logger.warning("Could not show sample material for workspace " "'{}':\n{}\n".format(names[0], exception))
        else:
            logger.warning("Sample material can only be viewed for a single workspace.")

    def _show_sample_shape(self, workspace_names):
        """
        Create a 3D Plot of the SampleShape from the selected workspaces

        :param names: A list of workspace names
        """
        if len(workspace_names) == 1:
            try:
                sample_shape.plot_sample_container_and_components(workspace_names[0])
            except Exception as exception:
                logger.warning("Could not show sample shape for workspace " "'{}':\n{}\n".format(workspace_names[0], exception))
        else:
            logger.warning("Plot Sample Shape can only be viewed for a single workspace.")

    def _action_double_click_workspace(self, name):
        ws = self._ads.retrieve(name)
        try:
            # if this is a table workspace (or peaks workspace),
            # then it can't be plotted automatically, so the data is shown instead
            TableWorkspaceDisplay.supports(ws)
            self._do_show_data([name])
        except ValueError:
            if hasattr(ws, "getMaxNumberBins") and ws.getMaxNumberBins() == 1:
                # If this is ws is just a single value show the data, else plot the bin
                if hasattr(ws, "getNumberHistograms") and ws.getNumberHistograms() == 1:
                    self._do_show_data([name])
                else:
                    plot_kwargs = {"axis": MantidAxType.BIN}
                    plot([ws], errors=False, overplot=False, wksp_indices=[0], plot_kwargs=plot_kwargs)
            else:
                plot_from_names([name], errors=False, overplot=False, show_colorfill_btn=True)

    def refresh_workspaces(self):
        self.workspacewidget.refreshWorkspaces()

    def empty_of_workspaces(self):
        return len(self._ads.getObjectNames()) == 0
