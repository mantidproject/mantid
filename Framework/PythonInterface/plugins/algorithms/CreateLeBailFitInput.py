# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
import mantid.simpleapi as api
from mantid.api import (
    AlgorithmFactory,
    AnalysisDataService,
    FileAction,
    FileProperty,
    ITableWorkspaceProperty,
    PythonAlgorithm,
    WorkspaceFactory,
)
from mantid.kernel import Direction, FloatBoundedValidator, IntArrayBoundedValidator, IntArrayProperty


class CreateLeBailFitInput(PythonAlgorithm):
    """Create the input TableWorkspaces for LeBail Fitting"""

    def category(self):
        return "Diffraction\\Fitting;Utility\\Workspaces"

    def seeAlso(self):
        return ["LeBailFit"]

    def name(self):
        return "CreateLeBailFitInput"

    def summary(self):
        return "Create various input Workspaces required by algorithm LeBailFit."

    def PyInit(self):
        """Declare properties"""
        self.declareProperty(
            FileProperty("ReflectionsFile", "", FileAction.OptionalLoad, [".hkl"]),
            "Name of [http://www.ill.eu/sites/fullprof/ Fullprof] .hkl file that contains the peaks.",
        )

        self.declareProperty(
            FileProperty("FullprofParameterFile", "", FileAction.Load, [".irf"]), "Fullprof's .irf file containing the peak parameters."
        )

        self.declareProperty("GenerateBraggReflections", False, "Generate Bragg reflections other than reading a Fullprof .irf file. ")

        arrvalidator = IntArrayBoundedValidator(lower=0)
        self.declareProperty(
            IntArrayProperty("MaxHKL", values=[12, 12, 12], validator=arrvalidator, direction=Direction.Input),
            "Maximum reflection (HKL) to generate",
        )

        self.declareProperty("Bank", 1, "Bank ID for output if there are more than one bank in .irf file.")

        self.declareProperty(
            "LatticeConstant", -0.0, validator=FloatBoundedValidator(lower=1.0e-9), doc="Lattice constant for cubic crystal."
        )

        self.declareProperty(
            ITableWorkspaceProperty("InstrumentParameterWorkspace", "", Direction.Output),
            "Name of Table Workspace Containing Peak Parameters From .irf File.",
        )

        self.declareProperty(
            ITableWorkspaceProperty("BraggPeakParameterWorkspace", "", Direction.Output),
            "Name of Table Workspace Containing Peaks' Miller Indices From .prf File.",
        )

        return

    def PyExec(self):
        """Main Execution Body"""
        # 1. Peak profile parameter workspace
        irffilename = self.getPropertyValue("FullprofParameterFile")
        paramWS = self.createPeakParameterWorkspace(irffilename)
        self.setProperty("InstrumentParameterWorkspace", paramWS)

        # 2. Get Other Properties
        reflectionfilename = self.getPropertyValue("ReflectionsFile")

        # 3. Import reflections list
        genhkl = self.getProperty("GenerateBraggReflections").value
        print("GeneraateHKL? = ", genhkl)
        if genhkl:
            hklmax = self.getProperty("MaxHKL").value
            if len(hklmax) != 3:
                raise RuntimeError("MaxHKL must have 3 integers")
            hklws = self.generateBraggReflections(hklmax)
        else:
            hklwsname = self.getProperty("BraggPeakParameterWorkspace").value
            hklws = self.importFullProfHKLFile(reflectionfilename, hklwsname)
        self.setProperty("BraggPeakParameterWorkspace", hklws)

        return

    def importFullProfHKLFile(self, hklfilename, hklwsname):
        """Import Fullprof's .hkl file"""
        import random

        rand = random.randint(1, 100000)
        dummywsname = "Foo%d" % (rand)
        hklwsname = self.getPropertyValue("BraggPeakParameterWorkspace")

        api.LoadFullprofFile(Filename=hklfilename, PeakParameterWorkspace=hklwsname, OutputWorkspace=dummywsname)

        hklws = AnalysisDataService.retrieve(hklwsname)
        if hklws is None:
            raise RuntimeError("Unable to retrieve LoadFullprofFile's output TempXXX from analysis data service.")

        api.DeleteWorkspace(Workspace=dummywsname)

        return hklws

    def createPeakParameterWorkspace(self, irffilename):
        """Create TableWorkspace by importing Fullprof .irf file

        Note:
        1. Sig-0, Sig-1 and Sig-2 in .irf file are actually the square of sig0, sig1 and sig2
           defined in the manual

        Input:
         - irffilename:  Resolution file (.irf)  Can be single bank or multiple bank

        Output:
         - tableworkspace
        """
        # 1. Import
        irfwsname = irffilename.split("/")[-1]
        irfws = api.LoadFullprofResolution(Filename=irffilename, OutputTableWorkspace=irfwsname)

        # 2. Create an empty workspace
        tablews = WorkspaceFactory.createTable()

        tablews.addColumn("str", "Name")
        tablews.addColumn("double", "Value")
        tablews.addColumn("str", "FitOrTie")
        tablews.addColumn("double", "Min")
        tablews.addColumn("double", "Max")
        tablews.addColumn("double", "StepSize")

        numrows = irfws.rowCount()
        for ir in range(numrows):
            tablews.addRow(["Parameter", 0.0, "tie", -1.0e200, 1.0e200, 1.0])

        # 3. Copy between 2 workspace
        for ir in range(numrows):
            tablews.setCell(ir, 0, irfws.cell(ir, 0))
            tablews.setCell(ir, 1, irfws.cell(ir, 1))

        # 4. Extra Lattice parameter
        latticepar = float(self.getPropertyValue("LatticeConstant"))
        tablews.addRow(["LatticeConstant", latticepar, "tie", -1.0e200, 1.0e200, 1.0])

        # 5. Clean
        api.DeleteWorkspace(Workspace=irfwsname)

        return tablews

    def generateBraggReflections(self, hklmax):
        """Generate Bragg reflections from (0, 0, 0) to (HKL)_max"""
        import math

        # Generate reflections
        max_hkl_sq = hklmax[0] ** 2 + hklmax[1] ** 2 + hklmax[2] ** 2
        max_m = int(math.sqrt(max_hkl_sq)) + 1

        # Note: the maximum HKL is defined by (HKL)^2.  Therefore, the iteration should reach some larger integer
        #       to avoid skipping some valid reflections

        hkldict = {}
        for h in range(0, max_m):
            for k in range(h, max_m):
                for L in range(k, max_m):
                    dsq = h * h + k * k + L * L
                    if dsq <= max_hkl_sq:
                        if (dsq in hkldict) is False:
                            hkldict[dsq] = []
                        hkldict[dsq].append([h, k, L])
                    # ENDIF
                # ENDFOR (L)
            # ENDFOR (k)
        # ENDFOR (h)

        # Create table workspace
        tablews = WorkspaceFactory.createTable()

        tablews.addColumn("int", "H")
        tablews.addColumn("int", "K")
        tablews.addColumn("int", "L")

        # Add reflections
        for dsp in sorted(hkldict.keys()):
            hkl = hkldict[dsp][0]
            if hkl[0] + hkl[1] + hkl[2] > 0:
                tablews.addRow(hkl)

        return tablews


# Register algorithm with Mantid
AlgorithmFactory.subscribe(CreateLeBailFitInput)
