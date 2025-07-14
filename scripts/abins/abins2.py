# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from typing import Dict, Optional

import numpy as np

from mantid.api import AlgorithmFactory, PythonAlgorithm, Progress
from mantid.api import WorkspaceFactory, AnalysisDataService

# noinspection PyProtectedMember
from mantid.simpleapi import ConvertUnits, GroupWorkspaces
import abins
from abins.abinsalgorithm import AbinsAlgorithm
from abins.atominfo import AtomInfo
from abins.logging import get_logger, Logger


# noinspection PyPep8Naming,PyMethodMayBeStatic
class Abins(AbinsAlgorithm, PythonAlgorithm):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self._setting = None

        # Save a copy of bin_width for cleanup after it is mutated
        self._initial_parameters_bin_width = abins.parameters.sampling["bin_width"]

    @classmethod
    def subscribe(cls, logger: Optional[Logger] = None) -> None:
        """Register Abins v2 with Algorithm Factory

        You may need to importlib.reload(mantid.simpleapi) to make v2 the default version of the Algorithm.
        Otherwise, provide a ``Version=2`` parameter when calling Abins to use this version.
        """
        logger = get_logger(logger=logger)
        logger.warning("Registering Abins v2 algorithm. This is work-in-progress: breaking changes are expected until full release.")
        AlgorithmFactory.subscribe(cls)

    def category(self) -> str:
        return "Simulation"

    def version(self) -> int:
        return 2

    def summary(self) -> str:
        return "Calculates inelastic neutron scattering against 1-D ω axis."

    def seeAlso(self):
        return ["Abins2D"]

    def PyInit(self) -> None:
        from abins.constants import ONE_DIMENSIONAL_INSTRUMENTS

        # Declare properties for all Abins Algorithms
        self.declare_common_properties(version=2)

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

        # Usually order 2 is enumerated over mode pairs, but if autoconvolution min_order is set to 2
        # then this is skipped and we obtain any orders above 1 by convolution
        if abins.parameters.autoconvolution["min_order"] not in (2, 3):
            raise ValueError("abins.parameters.autoconvolution['min_order'] must be 2 or 3")
        convolve_order_2 = abins.parameters.autoconvolution["min_order"] == 2

        match self._num_quantum_order_events, convolve_order_2:
            case 1, _:
                autoconvolution_max = 0
                quantum_order_num = 1
            case 2, True:
                autoconvolution_max = 2
                quantum_order_num = 1
            case 2, False:
                autoconvolution_max = 0
                quantum_order_num = 2
            case max_order, True:
                autoconvolution_max = max_order
                quantum_order_num = 1
            case max_order, False:
                autoconvolution_max = max_order
                quantum_order_num = 2

        s_calculator = abins.SCalculatorFactory.init(
            filename=self._vibrational_or_phonon_data_file,
            temperature=self._temperature,
            sample_form="Powder",
            abins_data=ab_initio_data,
            instrument=self._instrument,
            quantum_order_num=quantum_order_num,
            autoconvolution_max=autoconvolution_max,
            cache_directory=self._cache_directory,
        )
        s_calculator.progress_reporter = prog_reporter
        s_data = s_calculator.get_formatted_data()

        # Hold reporter at 80% for this message
        prog_reporter.resetNumSteps(1, 0.8, 0.80000001)
        prog_reporter.report("Dynamical structure factors have been determined.")
        # Now determine number of remaining messages and set reporter for rest of run:
        n_messages = 3 + bool(self._sum_contributions) + bool(self._save_ascii)
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

        # 7) Convert units
        gws = GroupWorkspaces(InputWorkspaces=workspaces, OutputWorkspace=self._out_ws_name)
        if self._energy_units == "meV":
            ConvertUnits(InputWorkspace=gws, OutputWorkspace=gws, EMode="Indirect", Target="DeltaE")

        # 8) save workspaces to ascii_file
        if self._save_ascii:
            self.write_workspaces_to_ascii(ws_name=self._out_ws_name, scale=(1.0 / self._instrument.get_energy_bin_width()))
            prog_reporter.report("All workspaces have been saved to ASCII files.")

        # 9) set  OutputWorkspace
        self.setProperty("OutputWorkspace", self._out_ws_name)
        prog_reporter.report("Group workspace with all required  dynamical structure factors has been constructed.")

    def _fill_s_workspace(self, *, s_points: np.ndarray, workspace: str, species: AtomInfo | None = None) -> None:
        """
        Puts S into workspace(s).

        :param s_points: dynamical factor for the given atom
        :param workspace: workspace to be filled with S
        :param species: Element/isotope data
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

    def _fill_s_1d_workspace(self, s_points=None, workspace=None, species: AtomInfo | None = None):
        """
        Puts 1D S into workspace.
        :param protons_number: number of protons in the given type of atom
        :param nucleons_number: number of nucleons in the given type of atom
        :param s_points: dynamical factor for the given atom
        :param workspace: workspace to be filled with S
        """
        if species is not None:
            s_points = s_points * self.get_cross_section(scattering=self._scale_by_cross_section, species=species)
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

    def _check_advanced_parameter(self):
        """
        Checks if parameters from abins.parameters are valid. If any parameter is invalid then RuntimeError is thrown
        with meaningful message.
        """

        message = " in abins.parameters. "

        self._check_common_advanced_parameters(message)

    def _get_properties(self):
        """
        Loads all properties to object's attributes.
        """
        self.get_common_properties()

        self._instrument_kwargs = {"setting": self.getProperty("Setting").value}
        self.set_instrument()

        self._bins = self.get_instrument().get_energy_bins()

        self._max_event_order = self._num_quantum_order_events
