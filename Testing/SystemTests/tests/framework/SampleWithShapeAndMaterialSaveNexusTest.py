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

    def execTest(self, test_script):
        script_path = os.path.join(tempfile.gettempdir(), "subtest_script.py")
        with open(script_path, "w") as f:
            f.write(test_script)

        result = subprocess.run(
            [sys.executable, script_path],
            capture_output=True,
            text=True,
        )

        print("Subprocess STDOUT:\n", result.stdout)
        print("Subprocess STDERR:\n", result.stderr)
        self.result_code = result.returncode

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


class TestRepeatedExecutionSameProcess(SetupMixin, MantidSystemTest):
    def runTest(self):
        script_content = r"""
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

ws1 = CreateSampleWorkspace()
SetSample(ws1, Geometry={'Shape': 'CSG', 'Value': cuboid})
SetSampleMaterial(ws1, "Fe")

ws2 = CreateSampleWorkspace()
SetSampleMaterial(ws2, "Fe")
ws2.sample().setShape(ws1.sample().getShape())
"""
        # write script to a temp file
        temp_dir = tempfile.gettempdir()
        test_script_path = os.path.join(temp_dir, "mantid_shape_test_script.py")
        with open(test_script_path, "w") as f:
            f.write(script_content)

        # write wrapper script to execute initial script twice
        wrapper_script_path = os.path.join(temp_dir, "wrapper_exec_twice.py")
        with open(wrapper_script_path, "w") as f:
            f.write(f'''
exec(open(r"{test_script_path}").read())
exec(open(r"{test_script_path}").read())
''')

        # run in subprocess
        result = subprocess.run(
            [sys.executable, wrapper_script_path],
            capture_output=True,
            text=True,
        )

        print("Wrapper STDOUT:\n", result.stdout)
        print("Wrapper STDERR:\n", result.stderr)

        self.result_code = result.returncode

    def validate(self):
        self.assertEqual(self.result_code, 0, "Script crashed on repeated execution in same process")
        return True
