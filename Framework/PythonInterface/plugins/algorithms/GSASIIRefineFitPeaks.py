from __future__ import (absolute_import, division, print_function)
import os
import tempfile
import sys

from mantid.kernel import *
from mantid.api import *
import mantid.simpleapi as mantid


class GSASIIRefineFitPeaks(PythonAlgorithm):
    """
    Mantid algorithm to use the powder diffraction and related data
    from the powder diffraction module of GSAS-II
    (https://subversion.xray.aps.anl.gov/trac/pyGSAS)
    """

    PROP_GSAS_PROJ_PATH = "GSASProjectPath"
    PROP_INPUT_WORKSPACE = "InputWorkspace"
    PROP_PATH_TO_GSASII = "GSASIIPath"
    PROP_PATH_TO_INST_PARAMS = "InstrumentParameterPath"
    PROP_PATH_TO_PHASE = "PhasePath"
    PROP_OUT_GOF = "GOF"
    PROP_OUT_GROUP_RESULTS = "Results"
    PROP_OUT_LATTICE_PARAMS = "LatticeParameters"
    PROP_OUT_RWP = "Rwp"
    PROP_WORKSPACE_INDEX = "WorkspaceIndex"

    DEFAULT_REFINEMENT_PARAMS = {"set":
                                 {"Background": {"no.coeffs": 3,
                                                 "refine": True},
                                  "Sample Parameters": ["Scale"]}}
    LATTICE_TABLE_PARAMS = ["length_a", "length_b", "length_c", "angle_alpha", "angle_beta", "angle_gamma", "volume"]

    def category(self):
        return "Diffraction\\Engineering;Diffraction\\Fitting"

    def name(self):
        return "GSASIIRefineFitPeaks"

    def summary(self):
        return ("Perform Rietveld refinement of lattice parameters on a diffraction spectrum "
                "using GSAS-II scriptable API")

    def PyInit(self):
        self.declareProperty(FileProperty(name=self.PROP_PATH_TO_GSASII, defaultValue="", action=FileAction.Directory),
                             doc="Path to the directory containing GSASII executable on the user's machine")
        self.declareProperty(WorkspaceProperty(name=self.PROP_INPUT_WORKSPACE, defaultValue="",
                                               direction=Direction.Input), doc="Workspace with spectra to fit peaks")
        self.declareProperty(name=self.PROP_WORKSPACE_INDEX, defaultValue=0, direction=Direction.Input,
                             doc="Index of the spectrum in InputWorkspace to fit. By default, the first spectrum "
                                 "(ie the only one for a focused workspace) is used")
        self.declareProperty(FileProperty(name=self.PROP_PATH_TO_PHASE, defaultValue="", action=FileAction.Load,
                                          extensions=[".cif"]), doc="Location of the phase file")
        self.declareProperty(FileProperty(name=self.PROP_PATH_TO_INST_PARAMS, defaultValue="", action=FileAction.Load,
                                          extensions=[".prm"]), doc="Location of the phase file")

        self.declareProperty(FileProperty(name=self.PROP_GSAS_PROJ_PATH, defaultValue="", action=FileAction.Save,
                                          extensions=".gpx"), doc="GSASII Project to work on")
        self.declareProperty(name=self.PROP_OUT_GOF, defaultValue=0.0, direction=Direction.Output,
                             doc="Goodness of fit value (Chi squared)")
        self.declareProperty(name=self.PROP_OUT_RWP, defaultValue=0.0, direction=Direction.Output,
                             doc="Weight profile R-factor (Rwp) discrepancy index for the goodness of fit")
        self.declareProperty(ITableWorkspaceProperty(name=self.PROP_OUT_LATTICE_PARAMS, direction=Direction.Output,
                                                     defaultValue=self.PROP_OUT_LATTICE_PARAMS),
                             doc="Table to output the lattice parameters (refined)")

        self.setPropertyGroup(self.PROP_GSAS_PROJ_PATH, self.PROP_OUT_GROUP_RESULTS)
        self.setPropertyGroup(self.PROP_OUT_GOF, self.PROP_OUT_GROUP_RESULTS)
        self.setPropertyGroup(self.PROP_OUT_RWP, self.PROP_OUT_GROUP_RESULTS)
        self.setPropertyGroup(self.PROP_OUT_LATTICE_PARAMS, self.PROP_OUT_GROUP_RESULTS)

    def PyExec(self):
        gsas_proj = self._initialise_GSAS()

        spectrum = self._extract_spectrum_from_workspace()
        spectrum_path = self._save_temporary_fxye(spectrum=spectrum)
        mantid.DeleteWorkspace(Workspace=spectrum)

        inst_param_path = self.getPropertyValue(self.PROP_PATH_TO_INST_PARAMS)
        histogram = gsas_proj.add_powder_histogram(datafile=spectrum_path, iparams=inst_param_path)

        self._remove_temporary_fxye(spectrum_path=spectrum_path)

        phase_path = self.getPropertyValue(self.PROP_PATH_TO_PHASE)
        phase = gsas_proj.add_phase(phasefile=phase_path, histograms=[histogram])

        gsas_proj.set_refinement(refinement=self.DEFAULT_REFINEMENT_PARAMS)
        gsas_proj.do_refinements([{}])

        covariance_rvals = gsas_proj.values()[2]["data"]["Rvals"]
        self.setProperty(self.PROP_OUT_RWP, covariance_rvals["Rwp"])
        self.setProperty(self.PROP_OUT_GOF, covariance_rvals["GOF"])

        lattice_params = gsas_proj.phases()[0].get_cell()
        lattice_param_table = self._build_output_lattice_table(lattice_params)
        self.setProperty(self.PROP_OUT_LATTICE_PARAMS, lattice_param_table)

    def _initialise_GSAS(self):
        gsas_path = self.getPropertyValue(self.PROP_PATH_TO_GSASII)
        sys.path.append(gsas_path)
        import GSASIIscriptable as GSASII

        gsas_proj = self.getPropertyValue(self.PROP_GSAS_PROJ_PATH)
        return GSASII.G2Project(filename=gsas_proj)

    def _build_output_lattice_table(self, lattice_params):
        alg = self.createChildAlgorithm('CreateEmptyTableWorkspace')
        alg.execute()
        table = alg.getProperty('OutputWorkspace').value

        for param in self.LATTICE_TABLE_PARAMS:
            table.addColumn("double", param.split("_")[-1])

        table.addRow([float(lattice_params[param]) for param in self.LATTICE_TABLE_PARAMS])
        return table

    def _extract_spectrum_from_workspace(self):
        ws = self.getPropertyValue(self.PROP_INPUT_WORKSPACE)
        ws_index = self.getPropertyValue(self.PROP_WORKSPACE_INDEX)
        spectrum = mantid.ExtractSpectra(InputWorkspace=ws, StartWorkspaceIndex=ws_index, EndWorkspaceIndex=ws_index)
        mantid.DeleteWorkspace(Workspace=ws)
        return spectrum

    def _save_temporary_fxye(self, spectrum):
        workspace_index = self.getPropertyValue(self.PROP_WORKSPACE_INDEX)
        temp_dir = tempfile.gettempdir()
        # Output file MUST end with "-n.fxye" where n is a number
        # If you see "Runtime error: Rvals" from GSASIIscriptable.py, it may be because this name is badly formatted
        file_path = os.path.join(temp_dir, "{}_focused_spectrum-{}.fxye".format(self.name(), workspace_index))
        mantid.SaveFocusedXYE(Filename=file_path, InputWorkspace=spectrum, SplitFiles=False)
        return file_path

    def _remove_temporary_fxye(self, spectrum_path):
        try:
            os.remove(spectrum_path)
        except Exception as e:
            raise Warning("Couldn't remove temporary spectrum file at location \"{}\":\n{}".format(spectrum_path, e))


AlgorithmFactory.subscribe(GSASIIRefineFitPeaks)