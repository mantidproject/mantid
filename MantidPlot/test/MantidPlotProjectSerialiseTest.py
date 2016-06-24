"""
Test of basic project saving and loading
"""
import mantidplottests
from mantidplottests import *
import numpy as np
from PyQt4 import QtGui, QtCore


class MantidPlotProjectSerialiseTest(unittest.TestCase):

    def setUp(self):
        pass

    def tearDown(self):
        pass

    def test_saveProjectAs(self):
        raise RuntimeError("FAIL")

    def test_openProject(self):
        raise RuntimeError("FAIL")


def create_dummy_workspace(ws_name):
    """ Create a dummy mantid workspace with some data """
    X1 = np.linspace(0,10, 100)
    Y1 = 1000*(np.sin(X1)**2) + X1*10
    X1 = np.append(X1, 10.1)

    X2 = np.linspace(2,12, 100)
    Y2 = 500*(np.cos(X2/2.)**2) + 20
    X2 = np.append(X2, 12.10)

    X = np.append(X1, X2)
    Y = np.append(Y1, Y2)
    E = np.sqrt(Y)

    CreateWorkspace(OutputWorkspace=ws_name, DataX=list(X),
                    DataY=list(Y), DataE=list(E), NSpec=2,
                    UnitX="TOF", YUnitLabel="Counts",
                    WorkspaceTitle="Faked data Workspace")


def read_project_file(folder_name):
    """ Read lines from a .mantid project file """

    if not os.path.isdir(folder_name):
        raise IOError('Path is not a directory')

    project_name = os.path.basename(folder_name) + '.mantid'
    project_file = os.path.join(project_name)

    if not os.path.isfile(project_file):
        raise IOError('Project file could not be found')

    with open(project_file, 'r') as file_handle:
        lines = file_handle.readlines()

    format_func = lambda s: s.strip().split('\t')
    tokens = map(format_func, lines)
    return tokens


# Run the unit tests
mantidplottests.runTests(MantidPlotProjectSerialiseTest)
