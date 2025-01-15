# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from typing import Dict

import numpy as np

from mantid.api import AlgorithmFactory, FileAction, FileProperty, PythonAlgorithm, Progress
from mantid.api import WorkspaceFactory, AnalysisDataService

# noinspection PyProtectedMember
from mantid.simpleapi import ConvertUnits, GroupWorkspaces, Load
from mantid.kernel import Direction, StringListValidator
import abins
from abins.abinsalgorithm import AbinsAlgorithm, AtomInfo


# noinspection PyPep8Naming,PyMethodMayBeStatic
class Abins(AbinsAlgorithm, PythonAlgorithm):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self._sample_form = None

        self._experimental_file = None
        self._scale = None
        self._setting = None

        # Save a copy of bin_width for cleanup after it is mutated
        self._initial_parameters_bin_width = abins.parameters.sampling["bin_width"]

    @staticmethod
    def category() -> str:
        return "Simulation"

    @staticmethod
    def summary() -> str:
        return "Calculates inelastic neutron scattering against 1-D ω axis."

    @staticmethod
    def version() -> int:
        return 1

    def seeAlso(self):
        return ["Abins2D"]

    def PyInit(self) -> None:
        from abins.constants import ALL_SAMPLE_FORMS, ONE_DIMENSIONAL_INSTRUMENTS

        # Declare properties for all Abins Algorithms
        self.declare_common_properties()

        # Soon-to-be-deprecated properties (i.e. already removed from 2D)
        self.declareProperty(name="BinWidthInWavenumber", defaultValue=1.0, doc="Width of bins used during rebinning.")

        self.declareProperty(
            name="SampleForm",
            direction=Direction.Input,
            defaultValue="Powder",
            validator=StringListValidator(ALL_SAMPLE_FORMS),
            doc="Form of the sample: Powder.",
        )

        # Declare properties specific to 1D
        self.declareProperty(
            FileProperty("ExperimentalFile", "", action=FileAction.OptionalLoad, direction=Direction.Input, extensions=["raw", "dat"]),
            doc="File with the experimental inelastic spectrum to compare.",
        )

        self.declareProperty(name="Scale", defaultValue=1.0, doc="Scale the intensity by the given factor. Default is no scaling.")

        # Declare Instrument-related properties
        self.declare_instrument_properties(
            default="TOSCA",
            choices=ONE_DIMENSIONAL_INSTRUMENTS,
            multiple_choice_settings=[("Setting", "settings", "Setting choice for this instrument (e.g. monochromator)")],
        )

    def validateInputs(self) -> Dict[str, str]:
        issues = dict()
        issues = self.validate_common_inputs(issues)
        issues.update(self._validate_instrument_settings())

        scale = self.getProperty("Scale").value
        if scale < 0:
            issues["Scale"] = "Scale must be positive."

        bin_width = self.getProperty("BinWidthInWavenumber").value
        if not (isinstance(bin_width, float) and 1.0 <= bin_width <= 10.0):
            issues["BinWidthInWavenumber"] = "Invalid bin width. Valid range is [1.0, 10.0] cm^-1"

        self._check_advanced_parameter()

        return issues

    def PyExec(self):
        # 0) Create reporter to report progress
        # Before calculating S, we use 10% of the bar for two steps
        begin, end, steps = 0, 0.1, 2
        prog_reporter = Progress(self, begin, end, steps)

        # 1) get input parameters from a user
        self._get_properties()
        prog_reporter.report("Input data from the user has been collected.")

        # 2) read ab initio data
        ab_initio_data = abins.AbinsData.from_calculation_data(
            self._vibrational_or_phonon_data_file, self._ab_initio_program, cache_directory=self._cache_directory
        )
        prog_reporter.report("Vibrational/phonon data has been read.")

        # 3) calculate S
        # Reset reporter to span range 10%-80%; s_calculator will decide how many steps are appropriate
        # so insert placeholder "1" for now.
        prog_reporter.resetNumSteps(1, 0.1, 0.8)

        if self._autoconvolution:
            autoconvolution_max = self._max_event_order
        else:
            autoconvolution_max = 0

        s_calculator = abins.SCalculatorFactory.init(
            filename=self._vibrational_or_phonon_data_file,
            temperature=self._temperature,
            sample_form=self._sample_form,
            abins_data=ab_initio_data,
            instrument=self._instrument,
            quantum_order_num=self._num_quantum_order_events,
            autoconvolution_max=autoconvolution_max,
        )
        s_calculator.progress_reporter = prog_reporter
        s_data = s_calculator.get_formatted_data()

        # Clean up parameter modified by _get_properties()
        abins.parameters.sampling["bin_width"] = self._initial_parameters_bin_width

        # Hold reporter at 80% for this message
        prog_reporter.resetNumSteps(1, 0.8, 0.80000001)
        prog_reporter.report("Dynamical structure factors have been determined.")
        # Now determine number of remaining messages and set reporter for rest of run:
        n_messages = 3 + bool(self._sum_contributions) + bool(self._experimental_file) + bool(self._save_ascii)
        prog_reporter.resetNumSteps(n_messages, 0.8, 1)

        # 4) get atoms for which S should be plotted
        atoms_data = ab_initio_data.get_atoms_data()
        atom_numbers, atom_symbols = self.get_atom_selection(atoms_data=atoms_data, selection=self._atoms)
        prog_reporter.report("Atoms, for which dynamical structure factors should be plotted, have been determined.")

        # 5) create workspaces for atoms in interest
        workspaces = []

        workspaces.extend(
            self.create_workspaces(
                atoms_symbols=atom_symbols, s_data=s_data, atoms_data=atoms_data, max_quantum_order=self._max_event_order
            )
        )
        workspaces.extend(
            self.create_workspaces(atom_numbers=atom_numbers, s_data=s_data, atoms_data=atoms_data, max_quantum_order=self._max_event_order)
        )
        prog_reporter.report("Workspaces with partial dynamical structure factors have been constructed.")

        # 6) Create a workspace with sum of all atoms if required
        if self._sum_contributions:
            self.create_total_workspace(workspaces)
            prog_reporter.report("Workspace with total S has been constructed.")

        # 7) add experimental data if available to the collection of workspaces
        if self._experimental_file != "":
            workspaces.insert(0, self._create_experimental_data_workspace().name())
            prog_reporter.report("Workspace with the experimental data has been constructed.")

        gws = GroupWorkspaces(InputWorkspaces=workspaces, OutputWorkspace=self._out_ws_name)

        # 7b) Convert units
        if self._energy_units == "meV":
            ConvertUnits(InputWorkspace=gws, OutputWorkspace=gws, EMode="Indirect", Target="DeltaE")

        # 8) save workspaces to ascii_file
        if self._save_ascii:
            self.write_workspaces_to_ascii(ws_name=self._out_ws_name, scale=(1.0 / self._bin_width))
            prog_reporter.report("All workspaces have been saved to ASCII files.")

        # 9) set  OutputWorkspace
        self.setProperty("OutputWorkspace", self._out_ws_name)
        prog_reporter.report("Group workspace with all required  dynamical structure factors has been constructed.")

    def _fill_s_workspace(self, *, s_points: np.ndarray, workspace: str, species: AtomInfo | None = None):
        """
        Puts S into workspace(s).

        :param s_points: dynamical factor for the given atom
        :param workspace:  workspace to be filled with S
        :param species: atom/isotope identity and data
        """

        from abins.constants import FUNDAMENTALS, ONE_DIMENSIONAL_INSTRUMENTS, ONE_DIMENSIONAL_SPECTRUM

        if self._instrument.get_name() not in ONE_DIMENSIONAL_INSTRUMENTS:
            raise ValueError("Instrument {self._instrument_name} is not supported by this version of Abins")

        # only FUNDAMENTALS [data is 2d with one row]
        if s_points.shape[0] == FUNDAMENTALS:
            self._fill_s_1d_workspace(s_points=s_points[0], workspace=workspace, species=species)

        # total workspaces [data is 1d vector]
        elif len(s_points.shape) == ONE_DIMENSIONAL_SPECTRUM:
            self._fill_s_1d_workspace(s_points=s_points, workspace=workspace, species=species)

        # quantum order events (fundamentals  or  overtones + combinations for the given order)
        # [data is 2d table of S with a row for each quantum order]
        else:
            dim = s_points.shape[0]
            partial_wrk_names = []

            for n in range(dim):
                wrk_name = f"{workspace}_quantum_event_{n + 1}"
                partial_wrk_names.append(wrk_name)

                self._fill_s_1d_workspace(s_points=s_points[n], workspace=wrk_name, species=species)

            GroupWorkspaces(InputWorkspaces=partial_wrk_names, OutputWorkspace=workspace)

    def _fill_s_1d_workspace(self, *, s_points: np.ndarray, workspace: str, species: AtomInfo | None = None):
        """
        Puts 1D S into workspace.
        :param s_points: dynamical factor for the given atom
        :param workspace: workspace to be filled with S
        :param species: atom/isotope identity and data
        """
        if species is not None:
            s_points = s_points * self._scale * self.get_cross_section(scattering=self._scale_by_cross_section, species=species)
        dim = 1
        length = s_points.size

        wrk = WorkspaceFactory.create("Workspace2D", NVectors=dim, XLength=length + 1, YLength=length)
        for i in range(dim):
            wrk.getSpectrum(i).setDetectorID(i + 1)
        wrk.setX(0, self._bins)
        wrk.setY(0, s_points)
        AnalysisDataService.addOrReplace(workspace, wrk)

        # Set correct units on workspace
        self.set_workspace_units(workspace, layout="1D")

    def _create_experimental_data_workspace(self):
        """
        Loads experimental data into workspaces.
        :returns: workspace with experimental data
        """
        experimental_wrk = Load(self._experimental_file)
        self.set_workspace_units(experimental_wrk.name(), energy_units=self._energy_units)

        return experimental_wrk

    def _check_advanced_parameter(self):
        """
        Checks if parameters from abins.parameters are valid. If any parameter is invalid then RuntimeError is thrown
        with meaningful message.
        """

        message = " in abins.parameters. "

        self._check_common_advanced_parameters(message)

        self._check_tosca_parameters(message)

    def _check_tosca_parameters(self, message_end=""):
        """
        Checks TOSCA parameters.
        :param message_end: closing part of the error message.
        """

        # TOSCA final energy in cm^-1
        tosca_parameters = abins.parameters.instruments["TOSCA"]
        final_energy = tosca_parameters["final_neutron_energy"]
        if not (isinstance(final_energy, float) and final_energy > 0.0):
            raise RuntimeError("Invalid value of final_neutron_energy for TOSCA" + message_end)

        angle = tosca_parameters["cos_scattering_angle"]
        if not isinstance(angle, float):
            raise RuntimeError("Invalid value of cosines scattering angle for TOSCA" + message_end)

        resolution_const_a = tosca_parameters["a"]
        if not isinstance(resolution_const_a, float):
            raise RuntimeError("Invalid value of constant A for TOSCA (used by the resolution TOSCA function)" + message_end)

        resolution_const_b = tosca_parameters["b"]
        if not isinstance(resolution_const_b, float):
            raise RuntimeError("Invalid value of constant B for TOSCA (used by the resolution TOSCA function)" + message_end)

        resolution_const_c = tosca_parameters["c"]
        if not isinstance(resolution_const_c, float):
            raise RuntimeError("Invalid value of constant C for TOSCA (used by the resolution TOSCA function)" + message_end)

    def _get_properties(self):
        """
        Loads all properties to object's attributes.
        """
        self.get_common_properties()

        self._bin_width = self.getProperty("BinWidthInWavenumber").value
        self._sample_form = self.getProperty("SampleForm").value

        self._instrument_kwargs = {"setting": self.getProperty("Setting").value}
        self.set_instrument()

        self._autoconvolution = self.getProperty("Autoconvolution").value
        self._experimental_file = self.getProperty("ExperimentalFile").value
        self._scale = self.getProperty("Scale").value

        abins.parameters.sampling["bin_width"] = self._bin_width
        self._bins = self.get_instrument().get_energy_bins()

        # Increase max event order if using autoconvolution
        if self._autoconvolution:
            self._max_event_order = abins.parameters.autoconvolution["max_order"]
        else:
            self._max_event_order = self._num_quantum_order_events


AlgorithmFactory.subscribe(Abins)
