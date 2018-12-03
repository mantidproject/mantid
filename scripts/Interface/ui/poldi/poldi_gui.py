# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
try:
    from mantidplot import *
except ImportError:
    canMantidPlot = False

from PyQt4 import QtGui
from mantid.simpleapi import *
from mantidqtpython import MantidQt
from ui.poldi.ui_poldi_window import Ui_PoldiWindow
from reduction_gui.reduction.scripter import execute_script

canMantidPlot = True


class MainPresenter(MantidQt.MantidWidgets.DataProcessor.DataProcessorMainPresenter):
    """
    A DataProcessorMainPresenter. The base class provides default implementations
    but we should re-implement the following methods:
    - getPreprocessingOptions() -- to supply global pre-processing options to the table widget
    - getProcessingOptions() -- to supply global processing options
    - getPostprocessingOptionsAsString() -- to supply global post-processing options
    - notifyADSChanged() -- to act when the ADS changed, typically we want to update
      table actions with the list of table workspaces that can be loaded into the interface

    This is an intermediate layer needed in python. Ideally our gui class should
    inherit from 'DataProcessorMainPresenter' directly and provide the required implementations,
    but multiple inheritance does not seem to be fully supported, hence we need this extra class.
    """

    def __init__(self, gui):
        super(MantidQt.MantidWidgets.DataProcessor.DataProcessorMainPresenter, self).__init__()
        self.gui = gui

    def getPreprocessingOptions(self, group = 0):
        """
        Return global pre-processing options as a dict of key:value pairs
        """
        empty = {}
        return empty

    def getProcessingOptions(self, group = 0):
        """
        Return global processing options as a dict of key:value pairs
        """
        empty = {}
        return empty

    def getPostprocessingOptionsAsString(self, group = 0):
        """
        Return global post-processing options as a string.
        The string must be a sequence of key=value separated by ','.
        """
        return ""

    def notifyADSChanged(self, workspace_list, group = 0):
        """
        The widget will call this method when something changes in the ADS.
        The argument is the list of table workspaces that can be loaded into
        the table. This method is intended to be used to update the table actions,
        specifically, to populate the 'Open Table' menu with the list of table
        workspaces.
        """
        self.gui.add_actions_to_menus(workspace_list)


class PoldiGui(QtGui.QMainWindow, Ui_PoldiWindow):

    data_processor_table = None
    main_presenter = None

    def __init__(self):
        """
        Initialise the interface
        """
        super(QtGui.QMainWindow, self).__init__()
        self.setupUi(self)

    def setup_layout(self):
        """
        Do further setup that could not be done in the designer.
        So far only two menus have been added, we need to add the processing table manually.
        """

        # The white list (mandatory)
        # Defines the number of columns, their names and the algorithm properties linked to them
        # Metdho 'addElement' adds a column to the widget
        # The first argument is the name of the column
        # The second argument is the algorithm property
        # The third argument is a brief description of the column
        # The fourth argument is a boolean indicating if the value in this column will be used to name the reduced run
        # The fifth arument is a prefix added to the value in this column used to generate the name of the reduced run
        # (unused if the previous argument is false)
        # In addition to the specified columns, a last column 'Options' is always added
        whitelist = MantidQt.MantidWidgets.DataProcessor.WhiteList()
        whitelist.addElement('Run(s)', 'InputWorkspace', 'Workspace with Poldi 2D-data and valid IDF', True)
        whitelist.addElement('Expected peak(s)', 'ExpectedPeaks', 'TableWorkspace with expected peaks used for indexing')
        whitelist.addElement('Maximum number of peaks', 'MaximumPeakNumber', 'Maximum number of peaks to process')
        whitelist.addElement('Profile function', 'ProfileFunction', 'Peak function to fit peaks to')
        whitelist.addElement('Pawley fit', 'PawleyFit', 'If enabled, refines lattice parameters')
        whitelist.addElement('Plot result', 'PlotResult', 'If enabled, plots results')

        # Pre-processing instructions (optional)
        # Not used in this interface

        # Processing algorithm (mandatory)
        # The main reduction algorithm
        # A number of prefixes equal to the number of output workspace properties must be specified
        # This prefixes are used in combination with the whitelist to name the reduced workspaces
        # For instance in this case, the first output workspace will be named IvsQ_binned_<runno>,
        # because column 'Run' is the only column used to create the workspace name, according to
        # the whitelist above
        # Additionally (not specified here) a blacklist of properties can be specified as the third
        # argument. These properties will not appear in the 'Options' column when typing
        alg = MantidQt.MantidWidgets.DataProcessor.ProcessingAlgorithm('PoldiDataAnalysis','Poldi_','')

        # Post-processing algorithm (optional)
        # Not used in this interface

        # The table widget
        self.data_processor_table = MantidQt.MantidWidgets.DataProcessor.QDataProcessorWidget(whitelist, alg, self)

        # A main presenter
        # Needed to supply global options for pre-processing/processing/post-processing to the widget
        self.main_presenter = MainPresenter(self)
        self.data_processor_table.accept(self.main_presenter)

        # Set the list of available instruments in the widget and the default instrument
        self.data_processor_table.setInstrumentList('POLDI', 'POLDI')
        # The widget will emit a 'runAsPythonScript' signal to run python code
        # We need to re-emit this signal so that it reaches mantidplot and the code is executed as a python script
        self.data_processor_table.runAsPythonScript.connect(self._run_python_code)

        # Add the widget to this interface
        self.layoutBase.addWidget(self.data_processor_table)

        return True

    def add_actions_to_menus(self, workspace_list):
        """
        Initialize table actions. Some table actions are not shown with the widget but they can be added to external menus.
        In this interface we have a 'File' menu and an 'Edit' menu
        """
        self.menuEdit.clear()
        self.menuFile.clear()

        # Demo (unrelated to the data processor widget)
        demo = QtGui.QAction('&Demo', self)
        demo.triggered.connect(lambda: self._demo_clicked())
        self.menuFile.addAction(demo)

        # Actions that go in the 'Edit' menu
        self._create_action(MantidQt.MantidWidgets.DataProcessor.ProcessCommand(self.data_processor_table), self.menuEdit)
        self._create_action(MantidQt.MantidWidgets.DataProcessor.ExpandCommand(self.data_processor_table), self.menuEdit)
        self._create_action(MantidQt.MantidWidgets.DataProcessor.PlotRowCommand(self.data_processor_table), self.menuEdit)
        self._create_action(MantidQt.MantidWidgets.DataProcessor.PlotGroupCommand(self.data_processor_table), self.menuEdit)
        self._create_action(MantidQt.MantidWidgets.DataProcessor.AppendRowCommand(self.data_processor_table), self.menuEdit)
        self._create_action(MantidQt.MantidWidgets.DataProcessor.AppendGroupCommand(self.data_processor_table), self.menuEdit)
        self._create_action(MantidQt.MantidWidgets.DataProcessor.GroupRowsCommand(self.data_processor_table), self.menuEdit)
        self._create_action(MantidQt.MantidWidgets.DataProcessor.CopySelectedCommand(self.data_processor_table), self.menuEdit)
        self._create_action(MantidQt.MantidWidgets.DataProcessor.CutSelectedCommand(self.data_processor_table), self.menuEdit)
        self._create_action(MantidQt.MantidWidgets.DataProcessor.PasteSelectedCommand(self.data_processor_table), self.menuEdit)
        self._create_action(MantidQt.MantidWidgets.DataProcessor.ClearSelectedCommand(self.data_processor_table), self.menuEdit)
        self._create_action(MantidQt.MantidWidgets.DataProcessor.DeleteRowCommand(self.data_processor_table), self.menuEdit)
        self._create_action(MantidQt.MantidWidgets.DataProcessor.DeleteGroupCommand(self.data_processor_table), self.menuEdit)

        # Actions that go in the 'File' menu
        self._create_action(MantidQt.MantidWidgets.DataProcessor.OpenTableCommand(self.data_processor_table), self.menuFile, workspace_list)
        self._create_action(MantidQt.MantidWidgets.DataProcessor.NewTableCommand(self.data_processor_table), self.menuFile)
        self._create_action(MantidQt.MantidWidgets.DataProcessor.SaveTableCommand(self.data_processor_table), self.menuFile)
        self._create_action(MantidQt.MantidWidgets.DataProcessor.SaveTableAsCommand(self.data_processor_table), self.menuFile)
        self._create_action(MantidQt.MantidWidgets.DataProcessor.ImportTableCommand(self.data_processor_table), self.menuFile)
        self._create_action(MantidQt.MantidWidgets.DataProcessor.ExportTableCommand(self.data_processor_table), self.menuFile)
        self._create_action(MantidQt.MantidWidgets.DataProcessor.OptionsCommand(self.data_processor_table), self.menuFile)

    def _demo_clicked(self):
        PoldiLoadRuns(2013, 6903, 6904, 2, OutputWorkspace='poldi', MaskBadDetectors=False)
        PoldiCreatePeaksFromCell(SpaceGroup='F d -3 m', Atoms='Si 0 0 0 1.0 0.01', a=5.431, LatticeSpacingMin=0.7, OutputWorkspace='Si')
        #self.data_processor_table.transfer(["Run(s):poldi_data_6904"])
        self.data_processor_table.transfer(["Run(s):poldi_data_6904,Expected peak(s):Si,Pawley fit:1,Maximum number of peaks:8"])

    def _create_action(self, command, menu, workspace_list = None):
        """
        Create an action from a given Command and add it to a given menu
        A 'workspace_list' can be provided but it is only intended to be used with OpenTableCommand.
        It refers to the list of table workspaces in the ADS that could be loaded into the widget. Note that only
        table workspaces with an appropriate number of columns and column types can be loaded.
        """

        if (workspace_list is not None and command.name() == "Open Table"):
            submenu = QtGui.QMenu(command.name(), self)
            submenu.setIcon(QtGui.QIcon(command.icon()))

            for ws in workspace_list:
                ws_command = MantidQt.MantidWidgets.DataProcessor.WorkspaceCommand(self.data_processor_table, ws)
                action = QtGui.QAction(QtGui.QIcon(ws_command.icon()), ws_command.name(), self)
                action.triggered.connect(lambda: self._connect_action(ws_command))
                submenu.addAction(action)

            menu.addMenu(submenu)
        else:
            action = QtGui.QAction(QtGui.QIcon(command.icon()), command.name(), self)
            action.setShortcut(command.shortcut())
            action.setStatusTip(command.tooltip())
            action.triggered.connect(lambda: self._connect_action(command))
            menu.addAction(action)

    def _connect_action(self, command):
        """
        Executes an action
        """
        command.execute()

    def _run_python_code(self, text):
        """
        Re-emits 'runPytonScript' signal
        """
        execute_script(text)
