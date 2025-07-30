# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import subprocess
import sys
import tempfile
import os
import json
from systemtesting import MantidSystemTest
from mantid.simpleapi import Load


class SetupMixin:
    def setUp(self):
        temp_dir = tempfile.gettempdir()
        self.fname = "test_file.nxs"
        self.save_fpath = os.path.join(temp_dir, self.fname)

        self.temp_dir_literal = json.dumps(temp_dir)
        self.fname_literal = json.dumps(self.fname)
        self.save_path_literal = json.dumps(self.save_fpath)

    def execTest(self, test_script, runs=1):
        script_path = os.path.join(tempfile.gettempdir(), "subtest_script.py")
        with open(script_path, "w") as f:
            f.write(test_script)

        run_num = 0
        self.result_code = 0

        while run_num < runs:
            result = subprocess.run(
                [sys.executable, script_path],
                capture_output=True,
                text=True,
            )

            print("Subprocess STDOUT:\n", result.stdout)
            print("Subprocess STDERR:\n", result.stderr)
            self.result_code = result.returncode
            run_num += 1

    def validate(self):
        self.assertEqual(self.result_code, 0, "Subprocess crashed or failed assertion")
        self.assertTrue(os.path.exists(self.save_fpath), "Expected output file not created")
        try:
            Load(Filename=self.save_fpath, OutputWorkspace="loaded_ws")
        except Exception as e:
            self.fail(f"Failed to load saved file: {e}")
        return True


class TestSetMaterialThenShapeThenSave(SetupMixin, MantidSystemTest):
    def runTest(self):
        test_script = f"""
from mantid.simpleapi import *
import os

TEMP_DIR = {self.temp_dir_literal}
fname = {self.fname_literal}
save_fpath = {self.save_path_literal}

ws = CreateSampleWorkspace(OutputWorkspace=fname)
SetSampleMaterial(ws, "Fe")
ws.sample().setShape(ws.sample().getShape())

SaveNexus(Filename=save_fpath, InputWorkspace=fname)
assert os.path.exists(save_fpath), "File was not saved."
"""
        self.execTest(test_script)


class TestSetShapeThenMaterialThenSave(SetupMixin, MantidSystemTest):
    def runTest(self):
        test_script = rf'''
from mantid.simpleapi import *
import os

TEMP_DIR = {self.temp_dir_literal}
fname = {self.fname_literal}
save_fpath = {self.save_path_literal}

ref_ws = LoadEmptyInstrument(InstrumentName="ENGINX", OutputWorkspace=fname)
shape_info = """<cuboid id='cube'>     \
<height val='0.02'  />     \
<width val='0.02' />      \
<depth  val='0.02' />      \
<centre x='0.0' y='0.0' z='0.0'  />      \
</cuboid>      \
<algebra val='cube' /> \ """

SetSampleShape(ref_ws, shape_info)
SetSampleMaterial(ref_ws, "Fe")

SaveNexus(Filename=save_fpath, InputWorkspace=fname)
assert os.path.exists(save_fpath), "File was not saved."
'''
        self.execTest(test_script)


class TestRepeatedScriptMemoryAllocation(SetupMixin, MantidSystemTest):
    def runTest(self):
        test_script = r"""
from mantid.simpleapi import *

cuboid = " \
<cuboid id='some-cuboid'> \
<height val='2.0'  /> \
<width val='2.0' />  \
<depth  val='0.2' />  \
<centre x='10.0' y='10.0' z='10.0'  />  \
</cuboid>  \
<algebra val='some-cuboid' /> \
"

ws = CreateSampleWorkspace()
SetSample(ws, Geometry={'Shape': 'CSG', 'Value': cuboid})
SetSampleMaterial(ws, "Fe")

ws2 = CreateSampleWorkspace()
SetSampleMaterial(ws2, "Fe")
ws2.sample().setShape(ws.sample().getShape())
    """
        self.execTest(test_script, 2)

    def validate(self):
        self.assertEqual(self.result_code, 0, "Subprocess crashed or failed assertion")
