"""
Test of basic project saving and loading
"""
import mantidplottests
from mantidplottests import *
import shutil
import numpy as np
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

        exp_contents = tokenise_project_file(file_text.split('\n'))
        contents = read_project_file(self._project_folder)
        self.assertEqual(contents, exp_contents)

    def test_project_file_with_plotted_spectrum(self):
        workspace_name = "fake_workspace"
        create_dummy_workspace(workspace_name)
        plotSpectrum(workspace_name, 1)

        saveProjectAs(self._project_folder)

        self.assert_project_files_saved(workspace_name)
        contents = read_project_file(self._project_folder)

        # Check corrent number of windows
        window_count = find_elements(contents, '<windows>')[0]
        self.assertEqual(int(window_count[1]), 1)

        # Check workspace list was written
        workspace_list = find_elements(contents, 'WorkspaceNames')[0]

        self.assertEqual(len(workspace_list), 2)
        self.assertEqual(workspace_list[1], workspace_name)

        # Check plot was written
        plot_titles = find_elements(contents, 'PlotTitle')[0]

        self.assertEqual(len(plot_titles), 4)
        self.assertEqual(plot_titles[1], workspace_name)

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

        # Check corrent number of windows
        window_count = find_elements(contents, '<windows>')[0]
        self.assertEqual(int(window_count[1]), 1)

        # Check plot title is correct
        plot_title = find_elements(contents, 'PlotTitle')[0]
        self.assertEqual(len(plot_title), 4)
        self.assertEqual(plot_title[1], "Hello World")

        # Check axes titles are correct
        axes_titles = find_elements(contents, 'AxesTitles')[0]
        self.assertEqual(len(axes_titles), 3)
        self.assertEqual(axes_titles[1], 'X Axis Modified')
        self.assertEqual(axes_titles[2], 'Y Axis Modified')

    def test_project_file_1D_plot_with_error_bars(self):
        workspace_name = "fake_workspace"
        create_dummy_workspace(workspace_name)
        plotSpectrum(workspace_name, 0, error_bars=True)

        saveProjectAs(self._project_folder)

        self.assert_project_files_saved(workspace_name)
        contents = read_project_file(self._project_folder)
        error_bars = find_elements(contents, '<MantidYErrors>1')[0]
        self.assertEqual(len(error_bars), 6)

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
        scale1, scale2, scale3, scale4 = find_elements(contents, 'scale')

        self.assertAlmostEqual(float(scale1[2]), 110.6670313)
        self.assertEqual(int(scale1[3]), 1000)

        self.assertAlmostEqual(float(scale2[2]), 110.6670313)
        self.assertEqual(int(scale2[3]), 1000)

        self.assertEqual(int(scale3[2]), 0)
        self.assertEqual(int(scale3[3]), 12)

        self.assertEqual(int(scale4[2]), 0)
        self.assertEqual(int(scale4[3]), 12)

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
        self.assertTrue(layer is not None)

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


def find_elements(tokens, name):
    """Find an element in the tokens parsed from the file """
    return filter(lambda l: l[0] == name, tokens)


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


def read_project_file(folder_name):
    """ Read lines from a .mantid project file """

    if not os.path.isdir(folder_name):
        raise IOError('Path is not a directory')

    project_name = os.path.basename(folder_name) + '.mantid'
    project_file = os.path.join(folder_name, project_name)

    if not os.path.isfile(project_file):
        raise IOError('Project file could not be found')

    with open(project_file, 'r') as file_handle:
        lines = file_handle.readlines()

    return tokenise_project_file(lines)


def tokenise_project_file(lines):
    """ Convert lines from a project file to a list of lists """
    return map(lambda s: s.strip().split('\t'), lines)


# Run the unit tests
mantidplottests.runTests(MantidPlotProjectSerialiseTest)
