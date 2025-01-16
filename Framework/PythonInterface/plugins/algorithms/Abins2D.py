# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import numbers
from typing import Dict

from mantid.api import AlgorithmFactory, PythonAlgorithm, Progress
from mantid.api import WorkspaceFactory, AnalysisDataService
from mantid.kernel import logger

# noinspection PyProtectedMember
from mantid.simpleapi import GroupWorkspaces
import abins
from abins.abinsalgorithm import AbinsAlgorithm


# noinspection PyPep8Naming,PyMethodMayBeStatic
class Abins2D(AbinsAlgorithm, PythonAlgorithm):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._q_bins = None

    @staticmethod
    def category() -> str:
        return "Simulation"

    @staticmethod
    def summary() -> str:
        return "Calculates inelastic neutron scattering over 2D (q,ω) space."

    @staticmethod
    def version() -> int:
        return 1

    @staticmethod
    def seeAlso():
        return ["Abins"]

    def PyInit(self) -> None:
        from abins.constants import TWO_DIMENSIONAL_INSTRUMENTS

        # Declare properties for all Abins Algorithms
        self.declare_common_properties()

        # Declare Abins2D-specific properties
        # ... (there aren't any at the moment!)

        # Declare instrument properties
        self.declare_instrument_properties(
            default="Ideal2D",
            choices=TWO_DIMENSIONAL_INSTRUMENTS,
            multiple_choice_settings=[("Chopper", "settings", "Chopper package")],
            freeform_settings=[
                ("IncidentEnergy", "4100", "Incident energy in EnergyUnits"),
                ("ChopperFrequency", "", "Chopper frequency in Hz"),
            ],
        )

    def validateInputs(self) -> Dict[str, str]:
        """
        Performs input validation. Use to ensure the user has defined a consistent set of parameters.
        """
        from abins.constants import MILLI_EV_TO_WAVENUMBER, TWO_DIMENSIONAL_CHOPPER_INSTRUMENTS

        issues = dict()
        issues = self.validate_common_inputs(issues)
        issues.update(self._validate_instrument_settings(name="Chopper", parameter="settings"))

        self._check_advanced_parameter()

        instrument_name = self.getProperty("Instrument").value
        if instrument_name in TWO_DIMENSIONAL_CHOPPER_INSTRUMENTS:
            allowed_frequencies = abins.parameters.instruments[instrument_name]["chopper_allowed_frequencies"]
            default_frequency = abins.parameters.instruments[instrument_name].get("chopper_frequency_default", None)

            if (self.getProperty("ChopperFrequency").value) == "" and (default_frequency is None):
                issues["ChopperFrequency"] = "This instrument does not have a default chopper frequency"
            elif self.getProperty("ChopperFrequency").value and int(self.getProperty("ChopperFrequency").value) not in allowed_frequencies:
                issues["ChopperFrequency"] = (
                    f"This chopper frequency is not valid for the instrument {instrument_name}. "
                    "Valid frequencies: " + ", ".join([str(freq) for freq in allowed_frequencies])
                )

        if not isinstance(float(self.getProperty("IncidentEnergy").value), numbers.Real):
            issues["IncidentEnergy"] = "Incident energy must be a real number"

        if "max_wavenumber" in abins.parameters.instruments[instrument_name]:
            max_energy = abins.parameters.instruments[instrument_name]["max_wavenumber"]
        else:
            max_energy = abins.parameters.sampling["max_wavenumber"]

        energy_units = self.getProperty("EnergyUnits").value

        if energy_units == "meV":
            max_energy = max_energy / MILLI_EV_TO_WAVENUMBER
        else:
            max_energy = max_energy

        if float(self.getProperty("IncidentEnergy").value) > max_energy:
            issues["IncidentEnergy"] = f"Incident energy cannot be greater than {max_energy:.3f} {energy_units} for this instrument."

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
        ab_initio_data = abins.AbinsData.from_calculation_data(self._vibrational_or_phonon_data_file, self._ab_initio_program)
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
            sample_form="Powder",
            abins_data=ab_initio_data,
            autoconvolution_max=autoconvolution_max,
            instrument=self._instrument,
            quantum_order_num=self._num_quantum_order_events,
        )
        s_calculator.progress_reporter = prog_reporter
        s_data = s_calculator.get_formatted_data()
        self._q_bins = s_data.get_q_bins()

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
        self.setProperty("OutputWorkspace", self._out_ws_name)

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

        GroupWorkspaces(InputWorkspaces=workspaces, OutputWorkspace=self._out_ws_name)

        # 8) save workspaces to ascii_file
        if self._save_ascii:
            self.write_workspaces_to_ascii(ws_name=self._out_ws_name, scale=(1.0 / self._instrument.get_energy_bin_width()))
            prog_reporter.report("All workspaces have been saved to ASCII files.")

        # 9) set  OutputWorkspace
        self.setProperty("OutputWorkspace", self._out_ws_name)
        prog_reporter.report("Group workspace with all required  dynamical structure factors has been constructed.")

    def _fill_s_workspace(self, s_points=None, workspace=None, protons_number=None, nucleons_number=None):
        """
        Puts S into workspace(s).

        :param s_points: dynamical factor for the given atom
        :param workspace:  workspace to be filled with S
        :param protons_number: number of protons in the given type fo atom
        :param nucleons_number: number of nucleons in the given type of atom
        """
        from abins.constants import FUNDAMENTALS, TWO_DIMENSIONAL_INSTRUMENTS

        if self._instrument.get_name() not in TWO_DIMENSIONAL_INSTRUMENTS:
            raise ValueError("Instrument {self._instrument_name} is not supported by this version of Abins")

        # only FUNDAMENTALS [data is 3d with length 1 in axis 0]
        if s_points.shape[0] == FUNDAMENTALS:
            self._fill_s_2d_workspace(
                s_points=s_points[0], workspace=workspace, protons_number=protons_number, nucleons_number=nucleons_number
            )

        # total workspaces [data is 2d array of S]
        elif s_points.shape[0] == abins.parameters.instruments[self._instrument.get_name()]["q_size"]:
            self._fill_s_2d_workspace(
                s_points=s_points, workspace=workspace, protons_number=protons_number, nucleons_number=nucleons_number
            )

        # Multiple quantum order events [data is 3d table of S using axis 0 for quantum orders]
        else:
            dim = s_points.shape[0]
            partial_wrk_names = []

            for n in range(dim):
                wrk_name = f"{workspace}_quantum_event_{n + 1}"
                partial_wrk_names.append(wrk_name)

                self._fill_s_2d_workspace(
                    s_points=s_points[n], workspace=wrk_name, protons_number=protons_number, nucleons_number=nucleons_number
                )

                GroupWorkspaces(InputWorkspaces=partial_wrk_names, OutputWorkspace=workspace)

    def _create_dummy_workspace(self, name):
        wrk = WorkspaceFactory.create("Workspace2D", NVectors=1, XLength=2, YLength=1)
        wrk.setX(0, [0, 1])
        wrk.setY(0, [0])
        AnalysisDataService.addOrReplace(name, wrk)
        return wrk

    def _fill_s_2d_workspace(self, s_points=None, workspace=None, protons_number=None, nucleons_number=None):
        from mantid.api import NumericAxis
        from abins.constants import MILLI_EV_TO_WAVENUMBER

        if protons_number is not None:
            s_points = s_points * self.get_cross_section(
                scattering=self._scale_by_cross_section, protons_number=protons_number, nucleons_number=nucleons_number
            )

        n_q_values, n_freq_bins = s_points.shape
        n_q_bins = self._q_bins.size
        assert n_q_values + 1 == n_q_bins

        if self._energy_units == "meV":
            energy_bins = self._bins / MILLI_EV_TO_WAVENUMBER
        else:
            energy_bins = self._bins

        wrk = WorkspaceFactory.create("Workspace2D", NVectors=n_freq_bins, XLength=n_q_bins, YLength=n_q_values)

        freq_axis = NumericAxis.create(n_freq_bins)
        freq_offset = (energy_bins[1] - energy_bins[0]) / 2
        for i, freq in enumerate(energy_bins[1:]):
            wrk.setX(i, self._q_bins)
            wrk.setY(i, s_points[:, i].T)
            freq_axis.setValue(i, freq + freq_offset)
        wrk.replaceAxis(1, freq_axis)

        AnalysisDataService.addOrReplace(workspace, wrk)

        self.set_workspace_units(workspace, layout="2D", energy_units=self._energy_units)

    def _check_advanced_parameter(self):
        """
        Checks if parameters from abins.parameters are valid. If any parameter
        is invalid then RuntimeError is thrown with meaningful message.
        """

        message = " in scripts/abins/parameters.py. "

        self._check_common_advanced_parameters(message)

    def _get_properties(self):
        """
        Loads all properties to object's attributes.
        """
        from abins.constants import TWO_DIMENSIONAL_CHOPPER_INSTRUMENTS

        self.get_common_properties()
        self._autoconvolution = self.getProperty("Autoconvolution").value

        self._instrument_kwargs = {"setting": self.getProperty("Chopper").value}

        instrument_name = self.getProperty("Instrument").value
        if instrument_name in TWO_DIMENSIONAL_CHOPPER_INSTRUMENTS:
            chopper_frequency = self.getProperty("ChopperFrequency").value
            chopper_frequency = int(chopper_frequency) if chopper_frequency else None

            self._instrument_kwargs.update({"chopper_frequency": chopper_frequency})
        elif self.getProperty("ChopperFrequency").value:
            logger.warning("The selected instrument does not use a chopper: chopper frequency will be ignored.")

        self.set_instrument()

        self._instrument.set_incident_energy(float(self.getProperty("IncidentEnergy").value), units=self._energy_units)

        self._bins = self.get_instrument().get_energy_bins()

        # Increase max event order if using autoconvolution
        if self._autoconvolution:
            self._max_event_order = abins.parameters.autoconvolution["max_order"]
        else:
            self._max_event_order = self._num_quantum_order_events


AlgorithmFactory.subscribe(Abins2D)
