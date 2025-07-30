import systemtesting
import os
from mantid.simpleapi import CreateSampleWorkspace, SetSampleMaterial, SaveNexus, LoadEmptyInstrument, SetSampleShape, mtd, Load
import tempfile

TEMP_DIR = tempfile.gettempdir()


class setupMixin:
    def setUp(self):
        self.fname = "test_file.nxs"
        self.save_fpath = os.path.join(TEMP_DIR, self.fname)

    def tearDown(self):
        mtd.clear()

    def validate(self):
        self.assertTrue(os.path.exists(self.save_fpath))
        Load(Filename=self.save_fpath, OutputWorkspace=self.fname)


class SetSampleMaterialThenShapeThenSaveTest(setupMixin, systemtesting.MantidSystemTest):
    def runTest(self):
        ws = CreateSampleWorkspace(OutputWorkspace=self.fname)
        SetSampleMaterial(ws, "Fe")
        ws.sample().setShape(ws.sample().getShape())
        SaveNexus(Filename=self.save_fpath, InputWorkspace=self.fname)


class SetSampleShapeThenMaterialThenSaveTest(setupMixin, systemtesting.MantidSystemTest):
    def runTest(self):
        ws = LoadEmptyInstrument(InstrumentName="ENGINX", OutputWorkspace=self.fname)
        shape_info = """<cuboid id='cube'>
                    <height val='0.02' />
                    <width val='0.02' />
                    <depth val='0.02' />
                    <centre x='0.0' y='0.0' z='0.0' />
                    </cuboid>
                    <algebra val='cube' />"""
        SetSampleShape(ws, shape_info)
        SetSampleMaterial(ws, "Fe")

        SaveNexus(Filename=self.save_fpath, InputWorkspace=self.fname)
