# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import numpy as np
from typing import Dict

from mantid.api import mtd, AlgorithmFactory, PythonAlgorithm, Progress, WorkspaceGroup
from mantid.api import WorkspaceFactory, AnalysisDataService

# noinspection PyProtectedMember
from mantid.simpleapi import CloneWorkspace, GroupWorkspaces
from mantid.kernel import StringListValidator, Direction
import abins
from abins.abinsalgorithm import AbinsAlgorithm


# noinspection PyPep8Naming,PyMethodMayBeStatic
class Abins2D(PythonAlgorithm, AbinsAlgorithm):
    _ab_initio_program = None
    _vibrational_or_phonon_data_file = None
    _temperature = None
    _bin_width = None
    _sample_form = None
    _instrument_name = None
    _atoms = None
    _sum_contributions = None
    _save_ascii = None
    _scale_by_cross_section = None
    _calc_partial = None
    _out_ws_name = None
    _num_quantum_order_events = None
    _atoms_data = None

    def category(self) -> str:
        return "Simulation"

    def summary(self) -> str:
        return "Calculates inelastic neutron scattering over 2D (q,ω) space."

    def PyInit(self) -> None:
        from abins.constants import TWO_DIMENSIONAL_INSTRUMENTS
        # Declare properties for all Abins Algorithms
        self.declare_common_properties()

        self.declareProperty(name="Instrument",
                             direction=Direction.Input,
                             defaultValue="TwoDMap",
                             validator=StringListValidator(TWO_DIMENSIONAL_INSTRUMENTS),
                             doc="Name of an instrument for which analysis should be performed.")

        # Declare special properties for 2D instrument
        self.declareProperty(name="Resolution",
                             direction=Direction.Input,
                             defaultValue=abins.parameters.instruments['TwoDMap']['resolution'],
                             doc="Width of broadening function")

        self.declareProperty(name="Angles",
                             direction=Direction.Input,
                             defaultValue="3.0, 140.0, 50")

    def validateInputs(self) -> Dict[str,str]:
        """
        Performs input validation. Use to ensure the user has defined a consistent set of parameters.
        """
        issues = dict()
        issues = self.validate_common_inputs(issues)

        self._check_advanced_parameter()

        import numbers
        if not isinstance(self.getProperty("Resolution").value, numbers.Real):
            issues["Resolution"] = "Resolution must a real number"

        try:
            self._angle_string_to_list(self.getProperty("Angles").value)

        except ValueError:
            issues["Angles"] = ("Could not parse Angle input. Format should be "
                                "\"min, max, steps\" or \"angle1, angle2, angle3, ...\"")

        return issues

    def PyExec(self):
        # 0) Create reporter to report progress
        steps = 9
        begin = 0
        end = 1.0
        prog_reporter = Progress(self, begin, end, steps)

        # 1) get input parameters from a user
        self._get_properties()
        prog_reporter.report("Input data from the user has been collected.")

        # 2) read ab initio data
        ab_initio_data = abins.AbinsData.from_calculation_data(self._vibrational_or_phonon_data_file,
                                                               self._ab_initio_program)
        prog_reporter.report("Vibrational/phonon data has been read.")

        # 3) calculate S
        s_calculator = abins.SCalculatorFactory.init(filename=self._vibrational_or_phonon_data_file,
                                                     temperature=self._temperature,
                                                     sample_form=self._sample_form,
                                                     abins_data=ab_initio_data,
                                                     instrument=self._instrument,
                                                     quantum_order_num=self._num_quantum_order_events,
                                                     bin_width=self._bin_width)
        s_data = s_calculator.get_formatted_data()
        prog_reporter.report("Dynamical structure factors have been determined.")

        # 4) get atoms for which S should be plotted
        self._atoms_data = ab_initio_data.get_atoms_data()
        atom_numbers, atom_symbols = self.get_atom_selection(atoms_data=self._atoms_data,
                                                             selection=self._atoms)
        prog_reporter.report("Atoms, for which dynamical structure factors should be plotted, have been determined.")

        # 5) create workspaces for atoms in interest
        workspaces = []
        workspaces.extend(self.create_workspaces(atoms_symbols=atom_symbols, s_data=s_data,
                                                 max_quantum_order=self._num_quantum_order_events))
        workspaces.extend(self.create_workspaces(atom_numbers=atom_numbers, s_data=s_data,
                                                 max_quantum_order=self._num_quantum_order_events))
        prog_reporter.report("Workspaces with partial dynamical structure factors have been constructed.")

        # 6) Create a workspace with sum of all atoms if required
        if self._sum_contributions:
            self.create_total_workspace(workspaces)
            prog_reporter.report("Workspace with total S has been constructed.")

        GroupWorkspaces(InputWorkspaces=workspaces, OutputWorkspace=self._out_ws_name)

        # 8) save workspaces to ascii_file
        if self._save_ascii:
            self.write_workspaces_to_ascii()
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
        from abins.constants import FUNDAMENTALS, TWO_DIMENSIONAL_INSTRUMENTS

        if self._instrument.get_name() not in TWO_DIMENSIONAL_INSTRUMENTS:
            raise ValueError("Instrument {self._instrument_name} is not supported by this version of Abins")

        # only FUNDAMENTALS [data is 3d with length 1 in axis 0]
        if s_points.shape[0] == FUNDAMENTALS:
            self._fill_s_2d_workspace(s_points=s_points[0], workspace=workspace, protons_number=protons_number,
                                      nucleons_number=nucleons_number)

        # total workspaces [data is 2d array of S]
        elif s_points.shape[0] == abins.parameters.instruments[self._instrument.get_name()]['q_size']:
            self._fill_s_2d_workspace(s_points=s_points, workspace=workspace, protons_number=protons_number,
                                      nucleons_number=nucleons_number)

        # Multiple quantum order events [data is 3d table of S using axis 0 for quantum orders]
        else:
            dim = s_points.shape[0]
            partial_wrk_names = []

            for n in range(dim):
                seed = "quantum_event_%s" % (n + 1)
                wrk_name = workspace + "_" + seed
                partial_wrk_names.append(wrk_name)

                self._fill_s_2d_workspace(s_points=s_points[n], workspace=wrk_name, protons_number=protons_number,
                                          nucleons_number=nucleons_number)

                GroupWorkspaces(InputWorkspaces=partial_wrk_names, OutputWorkspace=workspace)

    def _fill_s_2d_workspace(self, s_points=None, workspace=None, protons_number=None, nucleons_number=None):
        from abins.constants import Q_BEGIN, Q_END
        from mantid.api import NumericAxis

        if protons_number is not None:
            s_points = s_points *  self.get_cross_section(protons_number=protons_number,
                                                          nucleons_number=nucleons_number)

        n_q_bins, n_freq_bins = s_points.shape

        wrk = WorkspaceFactory.create("Workspace2D", NVectors=n_freq_bins, XLength=n_q_bins + 1, YLength=n_q_bins)

        freq_axis = NumericAxis.create(n_freq_bins)

        q_size = abins.parameters.instruments[self._instrument.get_name()]['q_size']
        q_bins = np.linspace(start=Q_BEGIN, stop=Q_END, num=q_size + 1)

        freq_offset = (self._bins[1] - self._bins[0]) / 2
        for i, freq in enumerate(self._bins[1:]):
            wrk.setX(i, q_bins)
            wrk.setY(i, s_points[:, i].T)
            freq_axis.setValue(i, freq + freq_offset)
        freq_axis.setUnit("Energy_inWavenumber")
        wrk.replaceAxis(1, freq_axis)

        AnalysisDataService.addOrReplace(workspace, wrk)

        self.set_workspace_units(workspace, layout="2D")

    def _check_advanced_parameter(self):
        """
        Checks if parameters from abins.parameters are valid. If any parameter
        is invalid then RuntimeError is thrown with meaningful message.
        """

        message = " in scripts/abins/parameters.py. "

        self._check_common_advanced_parameters(message)
        self._check_2d_parameters(message)

    def _check_2d_parameters(self, message_end=None):
        # check 2D resolution
        resolution_2d = abins.parameters.instruments['TwoDMap']['resolution']
        if not (isinstance(resolution_2d, float) and resolution_2d > 0):
            raise RuntimeError("Invalid value of abins.parameters"
                               ".instruments['TwoDMap']['resolution']"
                               + message_end)

    @staticmethod
    def _angle_string_to_list(angle_string):
        angle_string = angle_string.strip()
        if angle_string[0] == '[' and angle_string[-1] == ']':
            angles = list(map(float, angle_string[1:-1].split(',')))
        else:
            linspace_spec = list(map(float, angle_string.split(',')))
            if len(linspace_spec) != 3:
                raise ValueError()

            angles = np.linspace(*linspace_spec).tolist()

        return angles

    def _get_properties(self):
        """
        Loads all properties to object's attributes.
        """
        self.get_common_properties()

        # Sampling mesh is determined by
        # abins.parameters.sampling['min_wavenumber']
        # abins.parameters.sampling['max_wavenumber']
        # and abins.parameters.sampling['bin_width']
        step = self._bin_width
        start = abins.parameters.sampling['min_wavenumber']
        stop = abins.parameters.sampling['max_wavenumber'] + step
        self._bins = np.arange(start=start, stop=stop, step=step, dtype=abins.constants.FLOAT_TYPE)

        instrument_params = abins.parameters.instruments[self._instrument.get_name()]

        instrument_params['resolution'] = self.getProperty("Resolution").value
        instrument_params['angles'] = self._angle_string_to_list(self.getProperty("Angles").value)


AlgorithmFactory.subscribe(Abins2D)
