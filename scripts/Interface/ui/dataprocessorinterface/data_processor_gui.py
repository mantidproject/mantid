# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from PyQt4 import QtGui
from mantidqtpython import MantidQt
from ui.dataprocessorinterface.ui_data_processor_window import Ui_DataProcessorWindow
from reduction_gui.reduction.scripter import execute_script


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
        result = {"AnalysisMode":"PointDetectorAnalysis"}
        return result

    def getProcessingOptions(self, group = 0):
        """
        Return global processing options as a dict of key:value pairs.
        """
        result = {"AnalysisMode":"PointDetectorAnalysis", "WavelengthMin":"1.5"}
        return result

    def getPostprocessingOptionsAsString(self, group = 0):
        """
        Return global post-processing options as a string.
        The string must be a sequence of key=value separated by ','.
        """
        return "Params='0.03, -0.04, 0.6'"

    def notifyADSChanged(self, workspace_list, group = 0):
        """
        The widget will call this method when something changes in the ADS.
        The argument is the list of table workspaces that can be loaded into
        the table. This method is intended to be used to update the table actions,
        specifically, to populate the 'Open Table' menu with the list of table
        workspaces.
        """
        self.gui.add_actions_to_menus(workspace_list)


class DataProcessorGui(QtGui.QMainWindow, Ui_DataProcessorWindow):

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
        # Method 'addElement' adds a column to the widget
        # The first argument is the name of the column
        # The second argument is the algorithm property
        # The third argument is a brief description of the column
        # The fourth argument is a boolean indicating if the value in this column will be used to name the reduced run
        # The fifth arument is a prefix added to the value in this column used to generate the name of the reduced run
        # (unused if the previous argument is false)
        # In addition to the specified columns, a last column 'Options' is always added
        whitelist = MantidQt.MantidWidgets.DataProcessor.WhiteList()
        whitelist.addElement('Runs', 'InputWorkspace', 'The run to reduce', True, '')
        whitelist.addElement('Angle', 'ThetaIn', 'The incident angle', False, '')
        whitelist.addElement('Transmission Runs', 'FirstTransmissionRun', 'Transmission runs', False, '')
        whitelist.addElement('Q min', 'MomentumTransferMin', 'Q min', False, '')
        whitelist.addElement('Q max', 'MomentumTransferMax', 'Q max', False, '')
        whitelist.addElement('dQ/Q', 'MomentumTransferStep', 'Resolution', False, '')
        whitelist.addElement('Scale', 'ScaleFactor', 'Scale Factor', False, '')

        # Pre-processing instructions (optional)
        # Indicates which columns should be pre-process and the algorithms used to pre-process
        # Pre-processing only happens when two or more runs are specified in the same column, i.e. '1+2'
        # For instance, runs '1+2' in column 'Runs' will be added
        # Runs '3+4' in column 'Transmission Runs' will be combined using 'CreateTransmissionWorkspaceAuto'
        # First argument is the column name that should be pre-processed
        # Second argument is the algorithm used to pre-process
        # Third argument is a prefix to name the pre-processed workspace
        # Fourth argument is used if a 'HintingLineEdit' is used in the interface. In this case it indicates
        # the blacklist of properties that should be hidden in the hinting line edit
        preprocess_map = MantidQt.MantidWidgets.DataProcessor.PreprocessMap()
        preprocess_map.addElement('Runs', 'Plus', '', '')
        preprocess_map.addElement('Transmission Runs', 'CreateTransmissionWorkspaceAuto', '', '')

        # Processing algorithm (mandatory)
        # The main reduction algorithm
        # A number of prefixes equal to the number of output workspace properties must be specified
        # This prefixes are used in combination with the whitelist to name the reduced workspaces
        # For instance in this case, the first output workspace will be named IvsQ_binned_<runno>,
        # because column 'Run' is the only column used to create the workspace name, according to
        # the whitelist above
        # Additionally (not specified here) a blacklist of properties can be specified as the third
        # argument. These properties will not appear in the 'Options' column when typing
        alg = MantidQt.MantidWidgets.DataProcessor.ProcessingAlgorithm('ReflectometryReductionOneAuto','IvsQ_binned_, IvsQ_, IvsLam_','')

        # Post-processing algorithm (optional, but functionality not well tested when not supplied)
        # Algorithm to post-process runs belonging to the same group
        # First argument is the name of the algorithm
        # Second argument is the prefix to be added to the name of the post-processed workspace
        # Third argument is a black list of properties to hide if a hinting line edit is added to the interface
        post_alg = MantidQt.MantidWidgets.DataProcessor.PostprocessingAlgorithm(
                'Stitch1DMany', 'IvsQ_', 'InputWorkspaces, OutputWorkspaces')

        # The table widget
        self.data_processor_table = MantidQt.MantidWidgets.DataProcessor.QDataProcessorWidget(
                whitelist, preprocess_map, alg, post_alg, self)

        # A main presenter
        # Needed to supply global options for pre-processing/processing/post-processing to the widget
        self.main_presenter = MainPresenter(self)
        self.data_processor_table.accept(self.main_presenter)

        # Set the list of available instruments in the widget and the default instrument
        self.data_processor_table.setInstrumentList('INTER, POLREF, OFFSPEC', 'INTER')
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
                ws_command = MantidQt.MantidWidgets.WorkspaceCommand(self.data_processor_table, ws)
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
