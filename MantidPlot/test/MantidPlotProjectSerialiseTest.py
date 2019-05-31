# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Test of basic project saving and loading
"""
import mantidplottests
from mantidplottests import *
import shutil
import numpy as np
import re
from PyQt4 import QtGui, QtCore


class MantidPlotProjectSerialiseTest(unittest.TestCase):

    def setUp(self):
        self._project_name = "MantidPlotTestProject"
        self._project_folder = os.path.join(os.path.expanduser("~"),
                                            self._project_name)
        file_name = "%s.mantid" % self._project_name
        self._project_file = os.path.join(self._project_folder, file_name)

    def tearDown(self):
        # Clean up project files
        if os.path.isdir(self._project_folder):
            remove_folder(self._project_folder)

        clear_mantid()

    def test_project_file_with_no_data(self):
        workspace_name = "fake_workspace"
        create_dummy_workspace(workspace_name)

        saveProjectAs(self._project_folder)

        self.assertTrue(os.path.isdir(self._project_folder))
        self.assertTrue(os.path.isfile(self._project_file))

        file_text = "MantidPlot 0.9.5 project file\n" \
                    "<scripting-lang>\tPython\n" \
                    "<windows>\t0\n" \
                    "<mantidworkspaces>\n" \
                    "WorkspaceNames\tfake_workspace\n" \
                    "</mantidworkspaces>"

        exp_contents = parse_project_file(file_text)
        contents = read_project_file(self._project_folder)
        self.assertEqual(contents, exp_contents)

    def test_project_file_with_plotted_spectrum(self):
        workspace_name = "fake_workspace"
        create_dummy_workspace(workspace_name)
        plotSpectrum(workspace_name, 1)

        saveProjectAs(self._project_folder)

        self.assert_project_files_saved(workspace_name)
        contents = read_project_file(self._project_folder)

        # Check current number of windows
        self.assertEqual(int(contents['<windows>']), 1)

        # Check workspace list was written
        workspace_list = contents['mantidworkspaces']['WorkspaceNames']
        self.assertEqual(workspace_list, workspace_name)

        # Check plot was written
        plot_titles = contents['multiLayer']['graph']['PlotTitle']
        self.assertEqual(len(plot_titles), 3)
        self.assertEqual(plot_titles[0], workspace_name)

    def test_project_file_1D_plot_with_labels_modified(self):
        workspace_name = "fake_workspace"
        create_dummy_workspace(workspace_name)
        plotSpectrum(workspace_name, [0, 1])

        # modify axes labels
        graph = windows()[0]
        layer = graph.layer(1)
        # call using threadsafe_call to ensure things are executed on the GUI
        # thread, otherwise we get segfaults.
        threadsafe_call(layer.setTitle, "Hello World")
        threadsafe_call(layer.setAxisTitle, 0, "Y Axis Modified")
        threadsafe_call(layer.setAxisTitle, 2, "X Axis Modified")

        saveProjectAs(self._project_folder)

        self.assert_project_files_saved(workspace_name)
        contents = read_project_file(self._project_folder)

        # Check current number of windows
        self.assertEqual(int(contents['<windows>']), 1)

        # Check plot title is correct

        plot_title = contents['multiLayer']['graph']['PlotTitle']
        self.assertEqual(len(plot_title), 3)
        self.assertEqual(plot_title[0], "Hello World")

        # Check axes titles are correct
        axes_titles = contents['multiLayer']['graph']['AxesTitles']
        self.assertEqual(len(axes_titles), 2)
        self.assertEqual(axes_titles[0], 'X Axis Modified')
        self.assertEqual(axes_titles[1], 'Y Axis Modified')

    def test_project_file_1D_plot_with_error_bars(self):
        workspace_name = "fake_workspace"
        create_dummy_workspace(workspace_name)
        plotSpectrum(workspace_name, 0, error_bars=True)

        saveProjectAs(self._project_folder)

        self.assert_project_files_saved(workspace_name)
        contents = read_project_file(self._project_folder)
        error_bars = contents['multiLayer']['graph']['MantidYErrors']['1']
        self.assertEqual(len(error_bars), 5)

    def test_project_file_1D_plot_with_axes_scaling(self):
        workspace_name = "fake_workspace"
        create_dummy_workspace(workspace_name)
        plotSpectrum(workspace_name, 0)

        # modify axes scales
        graph = windows()[0]
        layer = graph.layer(1)
        # call using threadsafe_call to ensure things are executed on the GUI
        # thread. Otherwise we get segfaults.
        threadsafe_call(layer.setAxisScale, 0, 10, 10)
        threadsafe_call(layer.logYlinX)

        saveProjectAs(self._project_folder)

        self.assert_project_files_saved(workspace_name)
        contents = read_project_file(self._project_folder)

        # Check axis scales are as expected
        scales = contents['multiLayer']['graph']['scale']
        scale1, scale2, scale3, scale4 = scales[0], scales[1], scales[2], scales[3]

        self.assertAlmostEqual(float(scale1[1]), 110.6670313)
        self.assertEqual(int(scale1[2]), 1000)

        self.assertAlmostEqual(float(scale2[1]), 110.6670313)
        self.assertEqual(int(scale2[2]), 1000)

        self.assertEqual(int(scale3[1]), 0)
        self.assertEqual(int(scale3[2]), 12)

        self.assertEqual(int(scale4[1]), 0)
        self.assertEqual(int(scale4[2]), 12)

    def test_serialise_with_no_data(self):
        workspace_name = "fake_workspace"
        create_dummy_workspace(workspace_name)
        self.save_and_reopen_project()

        # Check that objects were reloaded
        self.assertEqual(rootFolder().name(), self._project_name)
        self.assertEqual(len(windows()), 0)
        self.assertEqual(len(mtd.getObjectNames()), 1)

    def test_serialise_1D_plot_with_plotted_spectrum(self):
        workspace_name = "fake_workspace"
        create_dummy_workspace(workspace_name)

        plotSpectrum(workspace_name, 1)

        self.save_and_reopen_project()

        # Check that objects were reloaded
        self.assertEqual(rootFolder().name(), self._project_name)
        self.assertEqual(len(windows()), 1)
        self.assertEqual(len(mtd.getObjectNames()), 1)

    def test_serialise_1D_plot_with_two_plot_windows(self):
        create_dummy_workspace("ws1")
        create_dummy_workspace("ws2")

        plotSpectrum("ws1", 1)
        plotSpectrum("ws2", 1)

        self.save_and_reopen_project()

        # Check that objects were reloaded
        self.assertEqual(rootFolder().name(), self._project_name)
        self.assertEqual(len(windows()), 2)
        self.assertEqual(len(mtd.getObjectNames()), 2)

        # Check both windows are graph objects
        for window in windows():
            # slight hack as 'type' only returns
            # an MDIWindow instance
            self.assertTrue('Graph' in str(window))

    def test_serialise_1D_plot_with_one_plot_and_multiple_spectra(self):
        workspace_name = "fake_workspace"
        create_dummy_workspace(workspace_name)
        plotSpectrum(workspace_name, [0, 1])

        self.save_and_reopen_project()

        self.assertEqual(rootFolder().name(), self._project_name)
        self.assertEqual(len(mtd.getObjectNames()), 1)
        self.assertEqual(len(windows()), 1)

        graph = windows()[0]
        layer = graph.layer(1)

        # Check graph and layer exist
        self.assertTrue('Graph' in str(graph))
        self.assertNotEqual(layer, None)

        # Check plot curves exist
        curve1 = layer.curve(0)
        curve2 = layer.curve(1)
        self.assertTrue('QwtPlotCurve', str(type(curve1)))
        self.assertTrue('QwtPlotCurve', str(type(curve2)))

    def test_serialise_waterfall_plot(self):
        workspace_name = "fake_workspace"
        create_dummy_workspace(workspace_name)
        plotSpectrum(workspace_name, [0, 1], waterfall=True)

        self.save_and_reopen_project()

        # Check that objects were reloaded
        self.assertEqual(rootFolder().name(), self._project_name)
        self.assertEqual(len(windows()), 1)
        self.assertEqual(len(mtd.getObjectNames()), 1)

        # Check window exists
        graph = windows()[0]
        self.assertTrue('Graph' in str(graph))

        # Check plot curves exist
        layer = graph.layer(1)
        curve1 = layer.curve(0)
        curve2 = layer.curve(1)
        self.assertTrue('QwtPlotCurve', str(type(curve1)))
        self.assertTrue('QwtPlotCurve', str(type(curve2)))

    def test_serialise_2D_plot(self):
        workspace_name = "fake_workspace"
        create_dummy_workspace(workspace_name)
        plot2D(workspace_name)

        self.save_and_reopen_project()

        # Check that objects were reloaded
        self.assertEqual(rootFolder().name(), self._project_name)
        self.assertEqual(len(windows()), 1)
        self.assertEqual(len(mtd.getObjectNames()), 1)

        # Check window exists
        graph = windows()[0]
        self.assertTrue('Graph' in str(graph))

    def test_save_instrument_view(self):
      workspace_name = 'fake_workspace'
      instrument_name = 'IRIS'

      # make a workspace with an instrument
      CreateSampleWorkspace(OutputWorkspace=workspace_name)
      LoadInstrument(Workspace=workspace_name, MonitorList='1,2',
                     InstrumentName=instrument_name, RewriteSpectraMap=True)

      window = getInstrumentView(workspace_name)

      render_tab = window.getTab("Render")
      # range options
      render_tab.setMinValue(1.25)
      render_tab.setMaxValue(1.75)
      render_tab.setRange(1.35,1.85)
      render_tab.showAxes(True)
      # display options
      render_tab.displayDetectorsOnly(True)
      render_tab.setColorMapAutoscaling(True)
      render_tab.setSurfaceType(InstrumentWidgetRenderTab.CYLINDRICAL_Y)
      render_tab.flipUnwrappedView(True)

      # pick tab
      pick_tab = window.getTab(InstrumentWidget.PICK)
      pick_tab.selectTool(InstrumentWidgetPickTab.PeakSelect)

      # mask tab
      mask_tab = window.getTab(InstrumentWidget.MASK)
      mask_tab.setMode(InstrumentWidgetMaskTab.Group)
      mask_tab.selectTool(InstrumentWidgetMaskTab.DrawEllipse)

      tree_tab = window.getTab(InstrumentWidget.TREE)
      tree_tab.selectComponentByName("graphite")

      saveProjectAs(self._project_folder)

      self.assert_project_files_saved(workspace_name)
      contents = read_project_file(self._project_folder)

      window_options = contents['instrumentwindow']
      self.assertEquals(int(window_options['SurfaceType']), 2)
      self.assertEquals(int(window_options['CurrentTab']), 0)

      # render tab options
      render_options = contents['instrumentwindow']['tabs']['rendertab']
      self.assertEqual(bool(render_options["DisplayDetectorsOnly"]), True)
      self.assertEqual(bool(render_options["AutoScaling"]), True)
      self.assertEqual(bool(render_options["FlipView"]), True)

      # pick tab options
      pick_options = contents['instrumentwindow']['tabs']['picktab']
      self.assertEqual(bool(pick_options['ActiveTools'][9]), True)

      # mask tab options
      mask_options = contents['instrumentwindow']['tabs']['masktab']
      self.assertEqual(bool(mask_options['ActiveType'][1]), True)
      self.assertEqual(bool(mask_options['ActiveTools'][2]), True)

    def assert_project_files_saved(self, workspace_name):
        """Check files were written to project folder """
        file_name = '%s.nxs' % workspace_name
        file_path = os.path.join(self._project_folder, file_name)
        self.assertTrue(os.path.isdir(self._project_folder))
        self.assertTrue(os.path.isfile(self._project_file))
        self.assertTrue(os.path.isfile(file_path))

    def save_and_reopen_project(self):
        """Save project and clear mantid then reopen the project """
        saveProjectAs(self._project_folder)
        clear_mantid()
        openProject(self._project_file)


def clear_mantid():
    """Clear plots and workspaces from Mantid.

    This will also start a new project and remove any previous
    project data
    """
    # Remove windows and plots
    for window in windows():
        window.confirmClose(False)
        window.close()
        QtCore.QCoreApplication.processEvents()

    # Clear workspaces
    mtd.clear()
    # Start a blank project to remove anything else
    newProject()

def create_dummy_workspace(ws_name):
    """ Create a dummy mantid workspace with some data """
    X1 = np.linspace(0, 10, 100)
    Y1 = 1000*(np.sin(X1)**2) + X1*10
    X1 = np.append(X1, 10.1)

    X2 = np.linspace(2, 12, 100)
    Y2 = 500*(np.cos(X2/2.)**2) + 20
    X2 = np.append(X2, 12.10)

    X = np.append(X1, X2)
    Y = np.append(Y1, Y2)
    E = np.sqrt(Y)

    CreateWorkspace(OutputWorkspace=ws_name, DataX=list(X),
                    DataY=list(Y), DataE=list(E), NSpec=2,
                    UnitX="TOF", YUnitLabel="Counts",
                    WorkspaceTitle="Faked data Workspace")


def remove_folder(folder_name):
    """ Remove a project folder after a test """
    if not os.path.isdir(folder_name):
        raise IOError('Path is not a directory')

    try:
        shutil.rmtree(folder_name)
    except:
        raise IOError('Could not clean up folder after test')

def get_project_file_contents(folder_name):
    """ Get the contents of a Mantid project file given the folder """
    if not os.path.isdir(folder_name):
        raise IOError('Path is not a directory')

    project_name = os.path.basename(folder_name) + '.mantid'
    project_file = os.path.join(folder_name, project_name)

    with open(project_file, 'r') as file_handle:
        contents = file_handle.read()

    return contents

def parse_project_file(contents, pattern=""):
    """ Create a dictionary of the Mantid project file entries """
    if pattern == "":
        pattern_str = "<(?P<tag>[a-zA-Z]*)>(.*)</(?P=tag)>"
        pattern = re.compile(pattern_str, flags=re.MULTILINE | re.DOTALL)

    match = re.findall(pattern, contents)
    contents = re.sub(pattern, '', contents)

    data = {}
    # recursively parse sections
    if len(match) > 0:
        data = {}
        for x, y in match:
            data[x] = y
        for key in data.keys():
            data[key] = parse_project_file(data[key], pattern)

    # parse individual property lines
    lines = contents.strip().split('\n')
    for line in lines:
        properties = line.strip().split('\t')
        key = properties[0]
        values = properties[1:]
        if key in data.keys():
            # if it already exists then add multiple entries as a dictionary
            # with numberical keys corresponding to the order added
            if not isinstance(data[key], dict):
                data[key] = {0: data[key]}
            data[key][max(data[key])+1] = values
        elif len(properties) == 2:
            data[key] = values[0]
        else:
            data[key] = values
    return data

def read_project_file(folder_name):
    """ Read and parse a Mantid project file """
    contents = get_project_file_contents(folder_name)
    return parse_project_file(contents)

# Run the unit tests
mantidplottests.runTests(MantidPlotProjectSerialiseTest)
