from mantid.kernel import *
from mantid.api import *


class GSASIIRefineFitPeaks(PythonAlgorithm):
    """
    Mantid algorithm to use the powder diffraction and related data
    from the powder diffraction module of GSAS-II
    (https://subversion.xray.aps.anl.gov/trac/pyGSAS)
    """

    PROP_GSAS_PROJ_PATH = "GSASProjectPath"
    PROP_PATH_TO_GSASII = "GSASIIPath"
    PROP_PATH_TO_INST_PARAMS = "InstrumentParameterPath"
    PROP_PATH_TO_PHASE = "PhasePath"
    PROP_PATH_TO_SPECTRUM = "SpectrumPath"
    PROP_OUT_GOF = "GOF"
    PROP_OUT_GROUP_RESULTS = "Results"
    PROP_OUT_LATTICE_PARAMS = "LatticeParameters"
    PROP_OUT_RWP = "Rwp"

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
        self.declareProperty(FileProperty(name=self.PROP_PATH_TO_SPECTRUM, defaultValue="", action=FileAction.Load,
                                          extensions=[".fxye"]), doc="Location of the spectrum to do a refinement on")
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

        self.setPropertyGroup(self.PROP_OUT_GOF, self.PROP_OUT_GROUP_RESULTS)
        self.setPropertyGroup(self.PROP_OUT_RWP, self.PROP_OUT_GROUP_RESULTS)
        self.setPropertyGroup(self.PROP_OUT_LATTICE_PARAMS, self.PROP_OUT_GROUP_RESULTS)

    def PyExec(self):
        gsas_proj = self._initialise_GSAS()

        spectrum_path = self.getPropertyValue(self.PROP_PATH_TO_SPECTRUM)
        inst_param_path = self.getPropertyValue(self.PROP_PATH_TO_INST_PARAMS)
        histogram = gsas_proj.add_powder_histogram(datafile=spectrum_path, iparams=inst_param_path)

        phase_path = self.getPropertyValue(self.PROP_PATH_TO_PHASE)
        phase = gsas_proj.add_phase(phasefile=phase_path, histograms=[histogram])

        gsas_proj.set_refinement(refinement=self.DEFAULT_REFINEMENT_PARAMS)
        gsas_proj.do_refinements([{}])

        covariance_rvals = gsas_proj.values()[2]["data"]["Rvals"]
        self.setProperty(self.PROP_OUT_RWP, covariance_rvals["Rwp"])
        self.setProperty(self.PROP_OUT_GOF, covariance_rvals["GOF"])

        lattice_params = gsas_proj.phases()[0].get_cell()
        lattice_param_table = self._build_output_lattice_table(**lattice_params)
        self.setProperty(self.PROP_OUT_LATTICE_PARAMS, lattice_param_table)

    def _initialise_GSAS(self):
        gsas_path = self.getPropertyValue(self.PROP_PATH_TO_GSASII)
        import sys
        sys.path.append(gsas_path)
        import GSASIIscriptable as GSASII

        gsas_proj = self.getPropertyValue(self.PROP_GSAS_PROJ_PATH)
        return GSASII.G2Project(filename=gsas_proj)

    def _build_output_lattice_table(self, **kwargs):
        alg = self.createChildAlgorithm('CreateEmptyTableWorkspace')
        alg.execute()
        table = alg.getProperty('OutputWorkspace').value

        for param in self.LATTICE_TABLE_PARAMS:
            table.addColumn("double", param.split("_")[-1])

        table.addRow([float(kwargs.get(param)) for param in self.LATTICE_TABLE_PARAMS])
        return table


AlgorithmFactory.subscribe(GSASIIRefineFitPeaks)