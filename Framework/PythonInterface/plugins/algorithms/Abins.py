# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import numpy as np
from typing import Dict

from mantid.api import AlgorithmFactory, FileAction, FileProperty, PythonAlgorithm, Progress
from mantid.api import WorkspaceFactory, AnalysisDataService

# noinspection PyProtectedMember
from mantid.simpleapi import ConvertUnits, GroupWorkspaces, Load
from mantid.kernel import Direction, StringListValidator
import abins
from abins.abinsalgorithm import AbinsAlgorithm


# noinspection PyPep8Naming,PyMethodMayBeStatic
class Abins(PythonAlgorithm, AbinsAlgorithm):
    _ab_initio_program = None
    _vibrational_or_phonon_data_file = None
    _experimental_file = None
    _temperature = None
    _bin_width = None
    _scale = None
    _sample_form = None
    _instrument_name = None
    _setting = None
    _atoms = None
    _sum_contributions = None
    _save_ascii = None
    _scale_by_cross_section = None
    _calc_partial = None
    _out_ws_name = None
    _num_quantum_order_events = None
    _max_event_order = None
    _energy_units = None
    _autoconvolution = None

    def category(self) -> str:
        return "Simulation"

    def summary(self) -> str:
        return "Calculates inelastic neutron scattering against 1-D ω axis."

    def PyInit(self) -> None:
        from abins.constants import ONE_DIMENSIONAL_INSTRUMENTS
        # Declare properties for all Abins Algorithms
        self.declare_common_properties()

        # Declare properties specific to 1D
        self.declareProperty(name="Autoconvolution", defaultValue=False,
                             doc="Estimate higher quantum orders by convolution with fundamental spectrum.")

        self.declareProperty(FileProperty("ExperimentalFile", "",
                                          action=FileAction.OptionalLoad,
                                          direction=Direction.Input,
                                          extensions=["raw", "dat"]),
                             doc="File with the experimental inelastic spectrum to compare.")

        self.declareProperty(name="Scale", defaultValue=1.0,
                             doc='Scale the intensity by the given factor. Default is no scaling.')

        self.declareProperty(name="EnergyUnits",
                             defaultValue="cm-1",
                             direction=Direction.Input,
                             validator=StringListValidator(["cm-1", "meV"]),
                             doc="Energy units for output workspace and experimental file")

        # Declare Instrument-related properties
        self.declare_instrument_properties(
            default="TOSCA", choices=ONE_DIMENSIONAL_INSTRUMENTS,
            multiple_choice_settings=[('Setting', 'settings',
                                       'Setting choice for this instrument (e.g. monochromator)')],
                )

    def validateInputs(self) -> Dict[str,str]:
        issues = dict()
        issues = self.validate_common_inputs(issues)
        issues.update(self._validate_instrument_settings())

        scale = self.getProperty("Scale").value
        if scale < 0:
            issues["Scale"] = "Scale must be positive."

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
        ab_initio_data = abins.AbinsData.from_calculation_data(self._vibrational_or_phonon_data_file,
                                                               self._ab_initio_program)
        prog_reporter.report("Vibrational/phonon data has been read.")

        # 3) calculate S
        # Reset reporter to span range 10%-80%; s_calculator will decide how many steps are appropriate
        # so insert placeholder "1" for now.
        prog_reporter.resetNumSteps(1, 0.1, 0.8)

        s_calculator = abins.SCalculatorFactory.init(filename=self._vibrational_or_phonon_data_file,
                                                     temperature=self._temperature,
                                                     sample_form=self._sample_form,
                                                     abins_data=ab_initio_data,
                                                     instrument=self._instrument,
                                                     quantum_order_num=self._num_quantum_order_events,
                                                     autoconvolution=self._autoconvolution,
                                                     bin_width=self._bin_width)
        s_calculator.progress_reporter = prog_reporter
        s_data = s_calculator.get_formatted_data()

        # Hold reporter at 80% for this message
        prog_reporter.resetNumSteps(1, 0.8, 0.80000001)
        prog_reporter.report("Dynamical structure factors have been determined.")
        # Now determine number of remaining messages and set reporter for rest of run:
        n_messages = 3 + bool(self._sum_contributions) + bool(self._experimental_file) + bool(self._save_ascii)
        prog_reporter.resetNumSteps(n_messages, 0.8, 1)

        # 4) get atoms for which S should be plotted
        atoms_data = ab_initio_data.get_atoms_data()
        atom_numbers, atom_symbols = self.get_atom_selection(atoms_data=atoms_data,
                                                             selection=self._atoms)
        prog_reporter.report("Atoms, for which dynamical structure factors should be plotted, have been determined.")

        # 5) create workspaces for atoms in interest
        workspaces = []

        workspaces.extend(self.create_workspaces(atoms_symbols=atom_symbols, s_data=s_data, atoms_data=atoms_data,
                                                 max_quantum_order=self._max_event_order))
        workspaces.extend(self.create_workspaces(atom_numbers=atom_numbers, s_data=s_data, atoms_data=atoms_data,
                                                 max_quantum_order=self._max_event_order))
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
        if self._energy_units == 'meV':
            ConvertUnits(InputWorkspace=gws, OutputWorkspace=gws, EMode='Indirect', Target='DeltaE')

        # 8) save workspaces to ascii_file
        if self._save_ascii:
            self.write_workspaces_to_ascii(ws_name=self._out_ws_name, scale=(1.0 / self._bin_width))
            prog_reporter.report("All workspaces have been saved to ASCII files.")

        # 9) set  OutputWorkspace
        self.setProperty('OutputWorkspace', self._out_ws_name)
        prog_reporter.report("Group workspace with all required  dynamical structure factors has been constructed.")

    def _fill_s_workspace(self, s_points=None, workspace=None, protons_number=None, nucleons_number=None):
        """
        Puts S into workspace(s).

        :param s_points: dynamical factor for the given atom
        :param workspace:  workspace to be filled with S
        :param protons_number: number of protons in the given type fo atom
        :param nucleons_number: number of nucleons in the given type of atom
        """

        from abins.constants import (FUNDAMENTALS, ONE_DIMENSIONAL_INSTRUMENTS, ONE_DIMENSIONAL_SPECTRUM)
        if self._instrument.get_name() not in ONE_DIMENSIONAL_INSTRUMENTS:
            raise ValueError("Instrument {self._instrument_name} is not supported by this version of Abins")

        # only FUNDAMENTALS [data is 2d with one row]
        if s_points.shape[0] == FUNDAMENTALS:
            self._fill_s_1d_workspace(s_points=s_points[0], workspace=workspace, protons_number=protons_number,
                                      nucleons_number=nucleons_number)

        # total workspaces [data is 1d vector]
        elif len(s_points.shape) == ONE_DIMENSIONAL_SPECTRUM:
            self._fill_s_1d_workspace(s_points=s_points, workspace=workspace, protons_number=protons_number,
                                      nucleons_number=nucleons_number)

        # quantum order events (fundamentals  or  overtones + combinations for the given order)
        # [data is 2d table of S with a row for each quantum order]
        else:
            dim = s_points.shape[0]
            partial_wrk_names = []

            for n in range(dim):
                seed = "quantum_event_%s" % (n + 1)
                wrk_name = workspace + "_" + seed
                partial_wrk_names.append(wrk_name)

                self._fill_s_1d_workspace(s_points=s_points[n], workspace=wrk_name, protons_number=protons_number,
                                          nucleons_number=nucleons_number)

            GroupWorkspaces(InputWorkspaces=partial_wrk_names, OutputWorkspace=workspace)

    def _fill_s_1d_workspace(self, s_points=None, workspace=None, protons_number=None, nucleons_number=None):
        """
        Puts 1D S into workspace.
        :param protons_number: number of protons in the given type of atom
        :param nucleons_number: number of nucleons in the given type of atom
        :param s_points: dynamical factor for the given atom
        :param workspace: workspace to be filled with S
        """
        if protons_number is not None:
            s_points = s_points * self._scale * self.get_cross_section(scattering=self._scale_by_cross_section,
                                                                       protons_number=protons_number,
                                                                       nucleons_number=nucleons_number)
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

    def _check_tosca_parameters(self, message_end=None):
        """
        Checks TOSCA parameters.
        :param message_end: closing part of the error message.
        """

        # TOSCA final energy in cm^-1
        tosca_parameters = abins.parameters.instruments['TOSCA']
        final_energy = tosca_parameters['final_neutron_energy']
        if not (isinstance(final_energy, float) and final_energy > 0.0):
            raise RuntimeError("Invalid value of final_neutron_energy for TOSCA" + message_end)

        angle = tosca_parameters['cos_scattering_angle']
        if not isinstance(angle, float):
            raise RuntimeError("Invalid value of cosines scattering angle for TOSCA" + message_end)

        resolution_const_a = tosca_parameters['a']
        if not isinstance(resolution_const_a, float):
            raise RuntimeError("Invalid value of constant A for TOSCA (used by the resolution TOSCA function)"
                               + message_end)

        resolution_const_b = tosca_parameters['b']
        if not isinstance(resolution_const_b, float):
            raise RuntimeError("Invalid value of constant B for TOSCA (used by the resolution TOSCA function)"
                               + message_end)

        resolution_const_c = tosca_parameters['c']
        if not isinstance(resolution_const_c, float):
            raise RuntimeError("Invalid value of constant C for TOSCA (used by the resolution TOSCA function)"
                               + message_end)

    def _get_properties(self):
        """
        Loads all properties to object's attributes.
        """
        from abins.constants import FLOAT_TYPE

        self.get_common_properties()
        self._instrument_kwargs = {"setting": self.getProperty("Setting").value}
        self.set_instrument()

        self._autoconvolution = self.getProperty("Autoconvolution").value
        self._experimental_file = self.getProperty("ExperimentalFile").value
        self._scale = self.getProperty("Scale").value
        self._energy_units = self.getProperty("EnergyUnits").value

        # Sampling mesh is determined by
        # abins.parameters.sampling['min_wavenumber']
        # abins.parameters.sampling['max_wavenumber']
        # and abins.parameters.sampling['bin_width']
        step = abins.parameters.sampling['bin_width'] = self._bin_width
        start = abins.parameters.sampling['min_wavenumber']
        stop = abins.parameters.sampling['max_wavenumber'] + step
        self._bins = np.arange(start=start, stop=stop, step=step, dtype=FLOAT_TYPE)

        # Increase max event order if using autoconvolution
        if self._autoconvolution:
            self._max_event_order = abins.parameters.autoconvolution['max_order']
        else:
            self._max_event_order = self._num_quantum_order_events


AlgorithmFactory.subscribe(Abins)
