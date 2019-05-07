# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import systemtesting
import mantid.simpleapi as mantid
from mantid import config
import os.path


DIRS = config['datasearch.directories'].split(';')
data_dir = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(DIRS[0]))), "Data", "UnitTest")
wsfile = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(DIRS[0]))), "Data", "SystemTest", "WISH", "input",
                      "18_1", "WISH00040503.raw")


class SampleShapeBase(systemtesting.MantidSystemTest):

    def setup(self):
        mantid.Load(Filename=wsfile, OutputWorkspace="ws", SpectrumList="100")
        mantid.ConvertUnits(InputWorkspace="ws", Target="Wavelength", OutputWorkspace="ws")

    def runTest(self):
        self.setup()
        mantid.LoadSampleShape(InputWorkspace="ws", OutputWorkspace="ws",
                               Filename=os.path.join(data_dir, "cylinder.stl"))
        mantid.SetSampleMaterial(InputWorkspace="ws", ChemicalFormula="V", SampleNumberDensity=0.1)
        mantid.MonteCarloAbsorption(InputWorkspace="ws", OutputWorkspace="ws", NumberOfWavelengthPoints=50)

    def validate(self):
        return "ws", "complexShapeAbsorb.nxs"


class RotatedSampleShape(SampleShapeBase):
    def setup(self):
        mantid.Load(Filename=wsfile, OutputWorkspace="ws", SpectrumList="100")
        mantid.ConvertUnits(InputWorkspace="ws", Target="Wavelength", OutputWorkspace="ws")
        mantid.SetGoniometer(Workspace="ws", Axis0="90,1,0,0,1")

    def validate(self):
        return "ws", "complexShapeAbsorbRotated.nxs"


class SampleEnvironment(SampleShapeBase):

    def handleEnvironment(self):
        mantid.LoadSampleEnvironment(InputWorkspace="ws", OutputWorkspace="ws",
                                     Filename=os.path.join(data_dir, "cube.stl"), Scale="cm", SetMaterial=True,
                                     ChemicalFormula="V", SampleNumberDensity=0.1)

    def runTest(self):
        self.setup()
        mantid.LoadSampleShape(InputWorkspace="ws", OutputWorkspace="ws",
                               Filename=os.path.join(data_dir, "cylinder.stl"), scale="mm")
        mantid.SetSampleMaterial(InputWorkspace="ws", ChemicalFormula="V", SampleNumberDensity=0.1)
        self.handleEnvironment()
        mantid.MonteCarloAbsorption(InputWorkspace="ws", OutputWorkspace="ws", NumberOfWavelengthPoints=50)

    def validate(self):
        return "ws", "complexEnvironmentAbsorb.nxs"


class RotatedSampleEnvironment(SampleEnvironment):

    def handleEnvironment(self):
        mantid.LoadSampleEnvironment(InputWorkspace="ws", OutputWorkspace="ws",
                                     Filename=os.path.join(data_dir, "cube.stl"), Scale="cm", SetMaterial=True,
                                     ChemicalFormula="V", SampleNumberDensity=0.1, XDegrees=90)

    def validate(self):
        return "ws", "complexEnvironmentRotatedAbsorb.nxs"


class RotatedSampleShapeAndSampleEnvironment(RotatedSampleShape):

    def runTest(self):
        self.setup()
        mantid.LoadSampleShape(InputWorkspace="ws", OutputWorkspace="ws",
                               Filename=os.path.join(data_dir, "cylinder.stl"), scale="mm")
        mantid.SetSampleMaterial(InputWorkspace="ws", ChemicalFormula="V", SampleNumberDensity=0.1)
        mantid.LoadSampleEnvironment(InputWorkspace="ws", OutputWorkspace="ws",
                                     Filename=os.path.join(data_dir, "cube.stl"), Scale="cm", SetMaterial=True,
                                     ChemicalFormula="V", SampleNumberDensity=0.1)
        mantid.MonteCarloAbsorption(InputWorkspace="ws", OutputWorkspace="ws", NumberOfWavelengthPoints=50)

    def validate(self):
        return "ws", "complexShapeRotatedAbsorbEnvironment.nxs"


class TranslatedSampleEnvironment(SampleEnvironment):

    def handleEnvironment(self):
        mantid.LoadSampleEnvironment(InputWorkspace="ws", OutputWorkspace="ws",
                                     Filename=os.path.join(data_dir, "cube.stl"), scale="cm", SetMaterial=True,
                                     ChemicalFormula="V", SampleNumberDensity=0.1, TranslationVector="0,0,100")

    def validate(self):
        return "ws", "complexEnvironmentTranslatedAbsorb.nxs"


class TranslatedAndRotatedSampleEnvironment(SampleEnvironment):

    def handleEnvironment(self):
        mantid.LoadSampleEnvironment(InputWorkspace="ws", OutputWorkspace="ws",
                                     Filename=os.path.join(data_dir, "cube.stl"), scale="cm", SetMaterial=True,
                                     ChemicalFormula="V", SampleNumberDensity=0.1, TranslationVector="0,2.5,0",
                                     XDegrees=45)

    def validate(self):
        return "ws", "complexEnvironmentRotatedTranslatedAbsorb.nxs"
