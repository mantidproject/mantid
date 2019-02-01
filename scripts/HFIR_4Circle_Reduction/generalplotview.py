import HFIR_4Circle_Reduction.fourcircle_utility as fourcircle_utility
from HFIR_4Circle_Reduction.integratedpeakview import GeneralPurposedPlotView
import os
from qtpy.QtWidgets import QMainWindow, QFileDialog
from mantid.kernel import Logger
try:
    from mantidqt.utils.qt import load_ui
except ImportError:
    Logger("HFIR_4Circle_Reduction").information('Using legacy ui importer')
    from mantidplot import load_ui
from qtpy.QtWidgets import (QVBoxLayout)


class GeneralPlotWindow(QMainWindow):
    """
    A window for general-purposed graphic (1-D plot) view
    """
    def __init__(self, parent):
        """
        initialization
        :param parent:
        """
        super(GeneralPlotWindow, self).__init__(parent)

        # set up UI
        ui_path = "general1dviewer.ui"
        self.ui = load_ui(__file__, ui_path, baseinstance=self)
        self._promote_widgets()

        # set up the event handling
        self.ui.pushButton_exportPlot2File.clicked.connect(self.do_export_plot)
        self.ui.pushButton_quit.clicked.connect(self.do_quit)
        self.ui.actionReset.triggered.connect(self.reset_window)

        # class variables
        self._work_dir = os.getcwd()

    def _promote_widgets(self):
        graphicsView_plotView_layout = QVBoxLayout()
        self.ui.frame_graphicsView_plotView.setLayout(graphicsView_plotView_layout)
        self.ui.graphicsView_plotView = GeneralPurposedPlotView(self)
        graphicsView_plotView_layout.addWidget(self.ui.graphicsView_plotView)

    def do_export_plot(self):
        """
        export plot
        :return:
        """
        # get directory
        file_name = QFileDialog.getSaveFileName(self, caption='File to save the plot',
                                                directory=self._work_dir,
                                                filter='Data File(*.dat);;All Files(*.*')
        if not file_name:
            return
        if isinstance(file_name, tuple):
            file_name = file_name[0]

        self.ui.graphicsView_plotView.save_current_plot(None, file_name)

    def do_quit(self):
        """
        close the window
        :return:
        """
        self.close()

    def menu_reset_window(self):
        """
        reset the window, i.e., plot
        :return:
        """
        self.ui.graphicsView_plotView.reset_plots()

    def plot_data(self, vec_x, vec_y, vec_e, x_label, y_label):  # '2theta', 'Gaussian-Sigma')
        """
        plot data on canvase
        :param vec_x:
        :param vec_y:
        :param vec_e:
        :param x_label:
        :param y_label:
        :return:
        """
        self.ui.graphicsView_plotView.plot_data(vec_x, vec_y, vec_e, 'No title', x_label, y_label)

    def set_working_dir(self, work_dir):
        """
        :param work_dir: working directory
        :return:
        """
        # check
        fourcircle_utility.check_str('Working directory', work_dir)
        if os.path.exists(work_dir) and os.access(work_dir, os.W_OK) is False:
            raise RuntimeError('Directory {0} is not writable.'.format(work_dir))
        elif not os.path.exists(work_dir):
            raise RuntimeError('Directory {0} does not exist.'.format(work_dir))
        else:
            self._work_dir = work_dir

    def reset_window(self):
        """
        reset the window to the initial state, such that no plot is made on the canvas
        :return:
        """
        self.ui.graphicsView_plotView.reset_plots()
