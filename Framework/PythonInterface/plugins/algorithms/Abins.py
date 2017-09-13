from __future__ import (absolute_import, division, print_function)
try:
    import pathos.multiprocessing as mp
    PATHOS_FOUND = True
except ImportError:
    PATHOS_FOUND = False

import numpy as np
import six
import os
import pylab as plt

from mantid.api import AlgorithmFactory, FileAction, FileProperty, PythonAlgorithm, Progress, WorkspaceProperty, mtd
from mantid.api import WorkspaceFactory, AnalysisDataService, NumericAxis

# noinspection PyProtectedMember
from mantid.api._api import WorkspaceGroup
from mantid.simpleapi import CloneWorkspace, GroupWorkspaces, SaveAscii, Load
from mantid.kernel import logger, StringListValidator, Direction, StringArrayProperty, Atom, ConfigService
import AbinsModules


# noinspection PyPep8Naming,PyMethodMayBeStatic
class Abins(PythonAlgorithm):

    _dft_program = None
    _phonon_file = None
    _experimental_file = None
    _temperature = None
    _scale = None
    _sample_form = None
    _atoms = None
    _sum_contributions = None
    _scale_by_cross_section = None
    _calc_partial = None
    _out_ws_name = None
    _num_quantum_order_events = None
    _extracted_dft_data = None

    def category(self):
        return "Simulation"

        # ----------------------------------------------------------------------------------------

    def summary(self):
        return "Calculates inelastic neutron scattering."

        # ----------------------------------------------------------------------------------------
    def PyInit(self):

        # Declare all properties
        self.declareProperty(name="DFTprogram",
                             direction=Direction.Input,
                             defaultValue="CASTEP",
                             validator=StringListValidator(["CASTEP", "CRYSTAL", "DMOL3", "GAUSSIAN"]),
                             doc="DFT program which was used for a phonon calculation.")

        self.declareProperty(FileProperty("PhononFile", "",
                             action=FileAction.Load,
                             direction=Direction.Input,
                             extensions=["phonon", "out", "outmol", "log"]),
                             doc="File with the data from a phonon calculation.")

        self.declareProperty(FileProperty("ExperimentalFile", "",
                             action=FileAction.OptionalLoad,
                             direction=Direction.Input,
                             extensions=["raw", "dat"]),
                             doc="File with the experimental inelastic spectrum to compare.")

        self.declareProperty(name="Temperature",
                             direction=Direction.Input,
                             defaultValue=10.0,
                             doc="Temperature in K for which dynamical structure factor S should be calculated.")

        self.declareProperty(name="Scale", defaultValue=1.0,
                             doc='Scale the intensity by the given factor. Default is no scaling.')

        self.declareProperty(name="SampleForm",
                             direction=Direction.Input,
                             defaultValue="Powder",
                             validator=StringListValidator(AbinsModules.AbinsConstants.ALL_SAMPLE_FORMS),
                             # doc="Form of the sample: SingleCrystal or Powder.")
                             doc="Form of the sample: Powder.")

        self.declareProperty(name="Instrument",
                             direction=Direction.Input,
                             defaultValue="TOSCA",
                             validator=StringListValidator(AbinsModules.AbinsConstants.ALL_INSTRUMENTS),
                             doc="Name of an instrument for which analysis should be performed.")

        self.declareProperty(StringArrayProperty("Atoms", Direction.Input),
                             doc="List of atoms to use to calculate partial S."
                                 "If left blank, workspaces with S for all types of atoms will be calculated.")

        self.declareProperty(name="SumContributions", defaultValue=False,
                             doc="Sum the partial dynamical structure factors into a single workspace.")

        self.declareProperty(name="ScaleByCrossSection", defaultValue='Incoherent',
                             validator=StringListValidator(['Total', 'Incoherent', 'Coherent']),
                             doc="Scale the partial dynamical structure factors by the scattering cross section.")

        self.declareProperty(name="QuantumOrderEventsNumber", defaultValue='1',
                             validator=StringListValidator(['1', '2', '3', '4']),
                             doc="Number of quantum order effects included in the calculation "
                                 "(1 -> FUNDAMENTALS, 2-> first overtone + FUNDAMENTALS + "
                                 "2nd order combinations, 3-> FUNDAMENTALS + first overtone + second overtone + 2nd "
                                 "order combinations + 3rd order combinations etc...)")

        self.declareProperty(WorkspaceProperty("OutputWorkspace", '', Direction.Output),
                             doc="Name to give the output workspace.")

    def validateInputs(self):
        """
        Performs input validation. Use to ensure the user has defined a consistent set of parameters.
        """

        input_file_validators = {"CASTEP": self._validate_castep_input_file,
                                 "CRYSTAL": self._validate_crystal_input_file,
                                 "DMOL3": self._validate_dmol3_input_file,
                                 "GAUSSIAN": self._validate_gaussian_input_file}

        issues = dict()

        temperature = self.getProperty("Temperature").value
        if temperature < 0:
            issues["Temperature"] = "Temperature must be positive."

        scale = self.getProperty("Scale").value
        if scale < 0:
            issues["Scale"] = "Scale must be positive."

        dft_program = self.getProperty("DFTprogram").value
        phonon_filename = self.getProperty("PhononFile").value
        output = input_file_validators[dft_program](filename_full_path=phonon_filename)
        if output["Invalid"]:
            issues["PhononFile"] = output["Comment"]

        workspace_name = self.getPropertyValue("OutputWorkspace")
        # list of special keywords which cannot be used in the name of workspace
        forbidden_keywords = ["total"]
        if workspace_name in mtd:
            issues["OutputWorkspace"] = "Workspace with name " + workspace_name + " already in use; please give " \
                                                                                  "a different name for workspace."
        elif workspace_name == "":
            issues["OutputWorkspace"] = "Please specify name of workspace."
        for word in forbidden_keywords:

            if word in workspace_name:
                issues["OutputWorkspace"] = "Keyword: " + word + " cannot be used in the name of workspace."
                break

        self._check_advanced_parameter()

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

        # 2) read DFT data
        dft_loaders = {"CASTEP": AbinsModules.LoadCASTEP, "CRYSTAL": AbinsModules.LoadCRYSTAL,
                       "DMOL3": AbinsModules.LoadDMOL3, "GAUSSIAN": AbinsModules.LoadGAUSSIAN}
        dft_reader = dft_loaders[self._dft_program](input_dft_filename=self._phonon_file)
        dft_data = dft_reader.get_formatted_data()
        prog_reporter.report("Phonon data has been read.")

        # 3) calculate S
        s_calculator = AbinsModules.CalculateS.init(filename=self._phonon_file, temperature=self._temperature,
                                                    sample_form=self._sample_form, abins_data=dft_data,
                                                    instrument=self._instrument,
                                                    quantum_order_num=self._num_quantum_order_events)
        s_data = s_calculator.get_formatted_data()
        prog_reporter.report("Dynamical structure factors have been determined.")

        # 4) get atoms for which S should be plotted
        self._extracted_dft_data = dft_data.get_atoms_data().extract()
        num_atoms = len(self._extracted_dft_data)
        all_atms_smbls = list(set([self._extracted_dft_data["atom_%s" % atom]["symbol"] for atom in range(num_atoms)]))
        all_atms_smbls.sort()

        if len(self._atoms) == 0:  # case: all atoms
            atoms_symbol = all_atms_smbls
        else:  # case selected atoms
            if len(self._atoms) != len(set(self._atoms)):  # only different types
                raise ValueError("Not all user defined atoms are unique.")

            for atom_symbol in self._atoms:
                if atom_symbol not in all_atms_smbls:
                    raise ValueError("User defined atom not present in the system.")
            atoms_symbol = self._atoms
        prog_reporter.report("Atoms, for which dynamical structure factors should be plotted, have been determined.")

        # at the moment only types of atom, e.g, for  benzene three options -> 1) C, H;  2) C; 3) H
        # 5) create workspaces for atoms in interest
        workspaces = []
        if self._sample_form == "Powder":
            workspaces.extend(self._create_partial_s_per_type_workspaces(atoms_symbols=atoms_symbol, s_data=s_data))
        prog_reporter.report("Workspaces with partial dynamical structure factors have been constructed.")

        # 6) Create a workspace with sum of all atoms if required
        if self._sum_contributions:
            total_atom_workspaces = []
            for ws in workspaces:
                if "total" in ws:
                    total_atom_workspaces.append(ws)
            total_workspace = self._create_total_workspace(partial_workspaces=total_atom_workspaces)
            workspaces.insert(0, total_workspace)
            prog_reporter.report("Workspace with total S  has been constructed.")

        # 7) add experimental data if available to the collection of workspaces
        if self._experimental_file != "":
            workspaces.insert(0, self._create_experimental_data_workspace().name())
            prog_reporter.report("Workspace with the experimental data has been constructed.")

        GroupWorkspaces(InputWorkspaces=workspaces, OutputWorkspace=self._out_ws_name)

        # 8) save workspaces to ascii_file
        num_workspaces = mtd[self._out_ws_name].getNumberOfEntries()
        for wrk_num in range(num_workspaces):
            wrk = mtd[self._out_ws_name].getItem(wrk_num)
            SaveAscii(InputWorkspace=wrk, Filename=wrk.name() + ".dat", Separator="Space", WriteSpectrumID=False)
        prog_reporter.report("All workspaces have been saved to ASCII files.")

        # 9) save workspaces to matplotlib figures
        if self._instrument.get_name() in AbinsModules.AbinsConstants.TWO_DIMENSIONAL_INSTRUMENTS:
            self._make_matplotlib_figs()
            prog_reporter.report("Figures of all workspaces have been saved.")

        # 10) set  OutputWorkspace
        self.setProperty('OutputWorkspace', self._out_ws_name)
        prog_reporter.report("Group workspace with all required  dynamical structure factors has been constructed.")

    def _make_matplotlib_figs(self):
        """
        Makes nice pictures for 2D map using matplotlib.
        """
        # default path for saving
        path = ConfigService.getString("defaultsave.directory")

        figure_format = AbinsModules.AbinsParameters.figure_format

        # switch on latex
        plt.rc('font', family='serif')
        plt.rc('text', usetex=True)

        begin_energy = self._bins[0]
        end_energy = self._bins[-1]
        step_energy = AbinsModules.AbinsConstants.ENERGY_PLOT_STEP
        x_ticks = np.arange(begin_energy, end_energy, step_energy)
        x_ticks_labels = [str(i).split(".")[0] for i in x_ticks]

        begin_q = 0
        end_q = AbinsModules.AbinsParameters.q_size
        step_q = AbinsModules.AbinsParameters.q_size / AbinsModules.AbinsConstants.Q_PLOT_STEP
        y_ticks = np.arange(begin_q, end_q, step_q)
        y_ticks_labels = [str(i * (AbinsModules.AbinsConstants.Q_PLOT_STEP - 1)).split(".")[0]
                          for i in range(len(y_ticks))]

        cmap = plt.get_cmap(AbinsModules.AbinsParameters.colormap)
        cmap.set_under(color=AbinsModules.AbinsConstants.BACKGROUND)
        interpolation = AbinsModules.AbinsParameters.interpolation

        num_workspaces = mtd[self._out_ws_name].getNumberOfEntries()
        for wrk_num in range(num_workspaces):

            wrk = mtd[self._out_ws_name].getItem(wrk_num)
            y_val = wrk.extractY()
            wrk_name = wrk.name()
            im = plt.imshow(y_val, cmap=cmap, aspect="auto", origin='lower', interpolation=interpolation,
                            vmin=AbinsModules.AbinsConstants.S_PLOT_THRESHOLD)

            plt.ylabel(r' \textbf{Momentum transfer ($\AA^{-1}$)}')
            plt.xlabel(r'\textbf{Energy transfer [cm$^{-1}$]}')

            axes = im.axes
            axes.set_xticks(x_ticks)
            axes.set_xticklabels(x_ticks_labels)
            axes.set_yticks(y_ticks)
            axes.set_yticklabels(y_ticks_labels)

            # colorbar
            max_s = np.max(y_val)
            min_s = np.min(y_val)
            cbar_ticks = np.linspace(min_s, max_s, AbinsModules.AbinsConstants.S_PLOT_SPACING)
            cbar = plt.colorbar(im, ticks=cbar_ticks)

            plt.savefig(os.path.join(path, wrk_name + "." + figure_format),
                        bbox_inches=AbinsModules.AbinsConstants.BBOX,
                        dpi=AbinsModules.AbinsConstants.DPI)
            cbar.remove()
            del im

    def _round(self, number):
        """
        Nicely rounds number which is in the scientific notation.
        :param number: number to round
        :return: rounded number
        """
        number_str = str(number)
        parts = number_str.split("e")
        if len(parts) == 1:
            result = str(round(float(parts[0]), AbinsModules.AbinsConstants.DIGITS_NUM))
        else:
            result = str(round(float(parts[0]), AbinsModules.AbinsConstants.DIGITS_NUM)) + "e" + parts[1]
        return result

    def _create_workspaces(self, atoms_symbols=None, s_data=None):
        """
        Creates workspaces for all types of atoms. Creates both partial and total workspaces for all types of atoms.

        :param atoms_symbols: list of atom types for which S should be created
        :param s_data: dynamical factor data of type SData
        :returns: workspaces for list of atoms types, S for the particular type of atom
        """
        s_data_extracted = s_data.extract()
        shape = [self._num_quantum_order_events]
        shape.extend(list(s_data_extracted["atom_0"]["s"]["order_1"].shape))

        s_atom_data = np.zeros(shape=tuple(shape), dtype=AbinsModules.AbinsConstants.FLOAT_TYPE)
        shape.pop(0)

        num_atoms = len([key for key in s_data_extracted.keys() if "atom" in key])
        temp_s_atom_data = np.copy(s_atom_data)

        result = []

        for atom_symbol in atoms_symbols:

            # create partial workspaces for the given type of atom
            atom_workspaces = []
            s_atom_data.fill(0.0)

            for atom in range(num_atoms):
                if self._extracted_dft_data["atom_%s" % atom]["symbol"] == atom_symbol:

                    temp_s_atom_data.fill(0.0)

                    for order in range(AbinsModules.AbinsConstants.FUNDAMENTALS,
                                       self._num_quantum_order_events + AbinsModules.AbinsConstants.S_LAST_INDEX):

                        order_indx = order - AbinsModules.AbinsConstants.PYTHON_INDEX_SHIFT
                        temp_s_order = s_data_extracted["atom_%s" % atom]["s"]["order_%s" % order]
                        temp_s_atom_data[order_indx] = temp_s_order

                    s_atom_data += temp_s_atom_data  # sum S over the atoms of the same type

            total_s_atom_data = np.sum(s_atom_data, axis=0)

            atom_workspaces.append(
                self._create_workspace(atom_name=atom_symbol, s_points=np.copy(total_s_atom_data),
                                       optional_name="_total"))

            atom_workspaces.append(
                self._create_workspace(atom_name=atom_symbol, s_points=np.copy(s_atom_data)))

            result.extend(atom_workspaces)

        return result

    def _create_partial_s_per_type_workspaces(self, atoms_symbols=None, s_data=None):
        """
        Creates workspaces for all types of atoms. Each workspace stores quantum order events for S for the given
        type of atom. It also stores total workspace for the given type of atom.

        :param atoms_symbols: list of atom types for which quantum order events of S  should be calculated
        :param s_data: dynamical factor data of type SData
        :returns: workspaces for list of atoms types, each workspace contains  quantum order events of
                 S for the particular atom type
        """

        return self._create_workspaces(atoms_symbols=atoms_symbols, s_data=s_data)

    def _fill_s_workspace(self, s_points=None, workspace=None, atom_name=None):
        """
        Puts S into workspace(s).
        :param s_points: dynamical factor for the given atom
        :param workspace:  workspace to be filled with S
        """
        if self._instrument.get_name() in AbinsModules.AbinsConstants.ONE_DIMENSIONAL_INSTRUMENTS:
            # only FUNDAMENTALS
            if s_points.shape[0] == AbinsModules.AbinsConstants.FUNDAMENTALS:

                self._fill_s_1d_workspace(s_points=s_points[0], workspace=workspace, atom_name=atom_name)

            # total workspaces
            elif len(s_points.shape) == AbinsModules.AbinsConstants.ONE_DIMENSIONAL_SPECTRUM:

                self._fill_s_1d_workspace(s_points=s_points, workspace=workspace, atom_name=atom_name)

            # quantum order events (fundamentals  or  overtones + combinations for the given order)
            else:

                dim = s_points.shape[0]
                partial_wrk_names = []

                for n in range(dim):
                    seed = "quantum_event_%s" % (n + 1)
                    wrk_name = workspace + "_" + seed
                    partial_wrk_names.append(wrk_name)

                    self._fill_s_1d_workspace(s_points=s_points[n], workspace=wrk_name, atom_name=atom_name)

                GroupWorkspaces(InputWorkspaces=partial_wrk_names, OutputWorkspace=workspace)
        # 2D S map
        else:

            q = np.linspace(start=AbinsModules.AbinsConstants.Q_BEGIN,
                                   stop=AbinsModules.AbinsConstants.Q_END,
                                   num=AbinsModules.AbinsParameters.q_size)

            # only FUNDAMENTALS
            if s_points.shape[0] == AbinsModules.AbinsConstants.FUNDAMENTALS:

                self._fill_s_2d_workspace(s_points=s_points[0], workspace=workspace, atom_name=atom_name, q=q)

            # total workspaces
            elif s_points.shape[0] == AbinsModules.AbinsParameters.q_size and len(s_points.shape) == 2:

                self._fill_s_2d_workspace(s_points=s_points, workspace=workspace, atom_name=atom_name, q=q)

            # quantum order events (fundamentals  or  overtones + combinations for the given order)
            else:

                dim = s_points.shape[0]
                partial_wrk_names = []

                for n in range(dim):
                    seed = "quantum_event_%s" % (n + 1)
                    wrk_name = workspace + "_" + seed
                    partial_wrk_names.append(wrk_name)

                    self._fill_s_2d_workspace(s_points=s_points[n], workspace=wrk_name, atom_name=atom_name, q=q)

                group = ','.join(partial_wrk_names)
                GroupWorkspaces(group, OutputWorkspace=workspace)

    def _fill_s_1d_workspace(self, s_points=None, workspace=None, atom_name=None):
        """
        Puts 1D S into workspace.
        :param s_points: dynamical factor for the given atom
        :param workspace: workspace to be filled with S
        :param atom_name: name of atom (for example H for hydrogen)
        """
        if atom_name is not None:

            s_points = s_points * self._scale * self._get_cross_section(atom_name=atom_name)

        dim = 1
        length = s_points.size
        wrk = WorkspaceFactory.create("Workspace2D", NVectors=dim, XLength=length + 1, YLength=length)
        wrk.setX(0, self._bins)
        wrk.setY(0, s_points)
        AnalysisDataService.addOrReplace(workspace, wrk)

        # Set correct units on workspace
        self._set_workspace_units(wrk=workspace)

    def _fill_s_2d_workspace(self, s_points=None, workspace=None, atom_name=None, q=None):
        """
        Puts 2D S into workspace.
        :param s_points: dynamical factor for the given atom
        :param workspace:  workspace to be filled with S
        :param atom_name: name of atom (for example H for hydrogen)
        :param q: momentum transfer
        """
        if atom_name is not None:
            s_points = s_points * self._scale * self._get_cross_section(atom_name=atom_name)

        # create "Workspace2D"
        length = s_points[0].size
        dim = q.size
        wrk = WorkspaceFactory.create("Workspace2D", NVectors=dim, XLength=length + 1, YLength=length)

        # set VerticalAxisValues for workspace
        y_axis = NumericAxis.create(dim)
        y_axis.setUnit("MomentumTransfer")
        for idx in range(dim):
            y_axis.setValue(idx, q[idx])
        wrk.replaceAxis(1, y_axis)

        # fill workspace
        for i in range(dim):
            wrk.setX(i, self._bins)
            wrk.setY(i, s_points[i])

        # make workspace accessible from outside function
        AnalysisDataService.addOrReplace(workspace, wrk)

        # set correct units on workspace
        self._set_workspace_units(wrk=workspace)

    def _get_cross_section(self, atom_name=None):
        """
        Calculates cross section for the given element.
        :param atom_name: symbol of element
        :returns: cross section for that element
        """
        atom = Atom(symbol=atom_name)
        cross_section = None
        if self._scale_by_cross_section == 'Incoherent':
            cross_section = atom.neutron()["inc_scatt_xs"]
        elif self._scale_by_cross_section == 'Coherent':
            cross_section = atom.neutron()["coh_scatt_xs"]
        elif self._scale_by_cross_section == 'Total':
            cross_section = atom.neutron()["tot_scatt_xs"]

        return cross_section

    def _create_total_workspace(self, partial_workspaces=None):

        """
        Sets workspace with total S.
        :param partial_workspaces: list of workspaces which should be summed up to obtain total workspace
        :returns: workspace with total S from partial_workspaces
                """
        total_workspace = self._out_ws_name + "_total"

        if isinstance(mtd[partial_workspaces[0]], WorkspaceGroup):
            local_partial_workspaces = mtd[partial_workspaces[0]].names()
        else:
            local_partial_workspaces = partial_workspaces

        # num_freq - number of frequencies
        # num_q - number of q points

        if len(local_partial_workspaces) > 1:

            # get frequencies
            ws = mtd[local_partial_workspaces[0]]

            # dim = num_q
            dim = ws.getNumberHistograms()

            # initialize S
            if self._instrument.get_name() in AbinsModules.AbinsConstants.ONE_DIMENSIONAL_INSTRUMENTS:
                s_atoms = np.zeros_like(ws.dataY(0))
            else:
                # s_atoms[num_freq, q]
                shape = (dim, ws.blocksize())
                s_atoms = np.zeros(shape=shape, dtype=AbinsModules.AbinsConstants.FLOAT_TYPE)

            # collect all S
            for partial_ws in local_partial_workspaces:
                if self._instrument.get_name() in AbinsModules.AbinsConstants.ONE_DIMENSIONAL_INSTRUMENTS:
                    s_atoms += mtd[partial_ws].dataY(0)
                else:
                    for i in range(dim):
                        s_atoms[i] += mtd[partial_ws].dataY(i)

            # create workspace with S
            self._fill_s_workspace(s_atoms, total_workspace)

        # Otherwise just repackage the workspace we have as the total
        else:
            CloneWorkspace(InputWorkspace=local_partial_workspaces[0], OutputWorkspace=total_workspace)

        return total_workspace

    def _create_workspace(self, atom_name=None, s_points=None, optional_name=""):

        """
        Creates workspace for the given frequencies and s_points with S data. After workspace is created it is rebined,
        scaled by cross-section factor and optionally multiplied by the user defined scaling factor.

        :param atom_name: symbol of atom for which workspace should be created
        :param frequencies: frequencies in the form of numpy array for which S(Q, omega) can be plotted
        :param s_points: S(Q, omega)
        :param optional_name: optional part of workspace name
        :returns: workspace for the given frequency and S data
        """

        ws_name = self._out_ws_name + "_" + atom_name + optional_name
        self._fill_s_workspace(s_points=s_points, workspace=ws_name, atom_name=atom_name)

        return ws_name

    def _create_experimental_data_workspace(self):
        """
        Loads experimental data into workspaces.
        :returns: workspace with experimental data
        """
        experimental_wrk = Load(self._experimental_file)
        self._set_workspace_units(wrk=experimental_wrk.name())

        return experimental_wrk

    def _set_workspace_units(self, wrk=None):
        """
        Sets x and y units for a workspace.
        :param wrk: workspace which units should be set
        """
        mtd[wrk].getAxis(0).setUnit("DeltaE_inWavenumber")
        mtd[wrk].setYUnitLabel("S /Arbitrary Units")
        mtd[wrk].setYUnit("Arbitrary Units")

    def _check_advanced_parameter(self):

        """
        Checks if parameters from AbinsParameters.py are valid. If any parameter is invalid then RuntimeError is thrown
        with meaningful message.
        """

        message = " in AbinsParameters.py. "

        self._check_general_resolution(message)
        self._check_2d_parameters(message)
        self._check_tosca_parameters(message)
        self._check_folder_names(message)
        self._check_rebining(message)
        self._check_threshold(message)
        self._check_chunk_size(message)
        self._check_threads(message)

    def _check_general_resolution(self, message_end=None):
        """
        Checks general parameters used in construction resolution functions.
        :param message_end: closing part of the error message.
        """
        # check fwhm
        fwhm = AbinsModules.AbinsParameters.fwhm
        if not (isinstance(fwhm, float) and 0.0 < fwhm < 10.0):
            raise RuntimeError("Invalid value of fwhm" + message_end)

    def _check_2d_parameters(self, message_end=None):
        """
        Checks if parameters for 2D instruments have valid values.
        :param message_end: closing part of the error message.
        """
        # check direct instrument resolution
        direct_instrument_resolution = AbinsModules.AbinsParameters.direct_instrument_resolution
        min_res = AbinsModules.AbinsConstants.MIN_DIRECT_RESOLUTION
        max_res = AbinsModules.AbinsConstants.MAX_DIRECT_RESOLUTION
        if not (isinstance(direct_instrument_resolution, float) and min_res <= direct_instrument_resolution <= max_res):
            raise RuntimeError("Invalid value of direct_instrument_resolution" + message_end)

        # angles -- numpy array in which all elements are floats
        angles = AbinsModules.AbinsParameters.angles
        if not (isinstance(angles, np.ndarray) and angles.dtype.num == AbinsModules.AbinsConstants.FLOAT_ID):
            raise ValueError("Invalid value of angles" + message_end)

        # e_init -- list in which all elements are floats
        e_init = AbinsModules.AbinsParameters.e_init
        if not (isinstance(e_init, list) and all([isinstance(i, float) for i in e_init])):
            raise ValueError("Invalid value of incident energies" + message_end)

        # q_size -- this is int
        q_size = AbinsModules.AbinsParameters.q_size
        if not isinstance(q_size, int):
            raise ValueError("Invalid number of q slices" + message_end)

        # interpolation
        interpolation = AbinsModules.AbinsParameters.interpolation
        if interpolation not in AbinsModules.AbinsConstants.ALL_INTERPOLATIONS:
            raise ValueError("Invalid interpolation.")

        colormap = AbinsModules.AbinsParameters.colormap
        if colormap not in AbinsModules.AbinsConstants.ALL_COLORMAPS:
            raise ValueError("Invalid colormap.")

        figure_format = AbinsModules.AbinsParameters.figure_format
        if figure_format not in AbinsModules.AbinsConstants.ALL_FIG_FORMATS:
            raise ValueError("Invalid format of figure.")

    def _check_tosca_parameters(self, message_end=None):
        """
        Checks TOSCA parameters.
        :param message_end: closing part of the error message.
        """

        # TOSCA final energy in cm^-1
        final_energy = AbinsModules.AbinsParameters.tosca_final_neutron_energy
        if not (isinstance(final_energy, float) and final_energy > 0.0):
            raise RuntimeError("Invalid value of final_neutron_energy for TOSCA" + message_end)

        angle = AbinsModules.AbinsParameters.tosca_cos_scattering_angle
        if not isinstance(angle, float):
            raise RuntimeError("Invalid value of cosines scattering angle for TOSCA" + message_end)

        resolution_const_a = AbinsModules.AbinsParameters.tosca_a
        if not isinstance(resolution_const_a, float):
            raise RuntimeError("Invalid value of constant A for TOSCA (used by the resolution TOSCA function)" +
                               message_end)

        resolution_const_b = AbinsModules.AbinsParameters.tosca_b
        if not isinstance(resolution_const_b, float):
            raise RuntimeError("Invalid value of constant B for TOSCA (used by the resolution TOSCA function)" +
                               message_end)

        resolution_const_c = AbinsModules.AbinsParameters.tosca_c
        if not isinstance(resolution_const_c, float):
            raise RuntimeError("Invalid value of constant C for TOSCA (used by the resolution TOSCA function)" +
                               message_end)

    def _check_folder_names(self, message_end=None):
        """
        Checks folders names.
        :param message_end: closing part of the error message.
        """
        folder_names = []
        dft_group = AbinsModules.AbinsParameters.dft_group
        if not isinstance(dft_group, str) or dft_group == "":
            raise RuntimeError("Invalid name for folder in which the DFT data should be stored.")
        folder_names.append(dft_group)

        powder_data_group = AbinsModules.AbinsParameters.powder_data_group
        if not isinstance(powder_data_group, str) or powder_data_group == "":
            raise RuntimeError("Invalid value of powder_data_group" + message_end)
        elif powder_data_group in folder_names:
            raise RuntimeError("Name for powder_data_group  already used by as name of another folder.")
        folder_names.append(powder_data_group)

        crystal_data_group = AbinsModules.AbinsParameters.crystal_data_group
        if not isinstance(crystal_data_group, str) or crystal_data_group == "":
            raise RuntimeError("Invalid value of crystal_data_group" + message_end)
        elif crystal_data_group in folder_names:
            raise RuntimeError("Name for crystal_data_group already used as a name of another folder.")

        s_data_group = AbinsModules.AbinsParameters.s_data_group
        if not isinstance(s_data_group, str) or s_data_group == "":
            raise RuntimeError("Invalid value of s_data_group" + message_end)
        elif s_data_group in folder_names:
            raise RuntimeError("Name for s_data_group already used as a name of another folder.")

    def _check_rebining(self, message_end=None):
        """
        Checks rebinning parameters.
        :param message_end: closing part of the error message.
        """
        pkt_per_peak = AbinsModules.AbinsParameters.pkt_per_peak
        if not (isinstance(pkt_per_peak, six.integer_types) and 1 <= pkt_per_peak <= 1000):
            raise RuntimeError("Invalid value of pkt_per_peak" + message_end)

        # bin width is expressed in cm^-1
        bin_width = AbinsModules.AbinsParameters.bin_width
        if not (isinstance(bin_width, float) and 1.0 <= bin_width <= 10.0):
            raise RuntimeError("Invalid value of bin_width" + message_end)

        min_wavenumber = AbinsModules.AbinsParameters.min_wavenumber
        if not (isinstance(min_wavenumber, float) and min_wavenumber >= 0.0):
            raise RuntimeError("Invalid value of min_wavenumber" + message_end)

        max_wavenumber = AbinsModules.AbinsParameters.max_wavenumber
        if not (isinstance(max_wavenumber, float) and max_wavenumber > 0.0):
            raise RuntimeError("Invalid number of max_wavenumber" + message_end)

        if min_wavenumber > max_wavenumber:
            raise RuntimeError("Invalid energy window for rebinning.")

    def _check_threshold(self, message_end=None):
        """
        Checks acoustic phonon threshold.
        :param message_end: closing part of the error message.
        """
        acoustic_threshold = AbinsModules.AbinsParameters.acoustic_phonon_threshold
        if not (isinstance(acoustic_threshold, float) and acoustic_threshold >= 0.0):
            raise RuntimeError("Invalid value of acoustic_phonon_threshold" + message_end)

        # check s threshold
        s_absolute_threshold = AbinsModules.AbinsParameters.s_absolute_threshold
        if not (isinstance(s_absolute_threshold, float) and s_absolute_threshold > 0.0):
            raise RuntimeError("Invalid value of s_absolute_threshold" + message_end)

        s_relative_threshold = AbinsModules.AbinsParameters.s_relative_threshold
        if not (isinstance(s_relative_threshold, float) and s_relative_threshold > 0.0):
            raise RuntimeError("Invalid value of s_relative_threshold" + message_end)

    def _check_chunk_size(self, message_end=None):
        """
        Check optimal size of chunk
        :param message_end: closing part of the error message.
        """
        optimal_size = AbinsModules.AbinsParameters.optimal_size
        if not (isinstance(optimal_size, six.integer_types) and optimal_size > 0):
            raise RuntimeError("Invalid value of optimal_size" + message_end)

    def _check_threads(self, message_end=None):
        """
        Checks number of threads
        :param message_end: closing part of the error message.
        """
        if PATHOS_FOUND:
            threads = AbinsModules.AbinsParameters.threads
            if not (isinstance(threads, six.integer_types) and 1 <= threads <= mp.cpu_count()):
                raise RuntimeError("Invalid number of threads for parallelisation over atoms" + message_end)

    def _validate_dft_file_extension(self, filename_full_path=None, expected_file_extension=None):
        """
        Checks consistency between name of DFT program and extension.
        :param dft_program: name of DFT program in the form of string
        :param expected_file_extension: file extension
        :returns: dictionary with error message
        """
        dft_program = self.getProperty("DFTprogram").value
        msg_err = "Invalid %s file. " % filename_full_path
        msg_rename = "Please rename your file and try again."

        # check  extension of a file
        found_filename_ext = os.path.splitext(filename_full_path)[1]
        if found_filename_ext != expected_file_extension:
            return dict(Invalid=True,
                        Comment=msg_err + "Output from DFT program " + dft_program + " is expected." +
                                          " The expected extension of file is ." + expected_file_extension +
                                          ".  Found: " + found_filename_ext + ". " + msg_rename)
        else:
            return dict(Invalid=False, Comment="")

    def _validate_dmol3_input_file(self, filename_full_path=None):
        """
        Method to validate input file for DMOL3 DFT program.
        :param filename_full_path: full path of a file to check.
        :returns: True if file is valid otherwise false.
        """
        logger.information("Validate DMOL3 phonon file: ")
        return self._validate_dft_file_extension(filename_full_path=filename_full_path,
                                                 expected_file_extension=".outmol")

    def _validate_gaussian_input_file(self, filename_full_path=None):
        """
        Method to validate input file for GAUSSIAN DFT program.
        :param filename_full_path: full path of a file to check.
        :returns: True if file is valid otherwise false.
        """
        logger.information("Validate GAUSSIAN file with vibration data: ")
        return self._validate_dft_file_extension(filename_full_path=filename_full_path,
                                                 expected_file_extension=".log")

    def _validate_crystal_input_file(self, filename_full_path=None):
        """
        Method to validate input file for CRYSTAL DFT program.
        :param filename_full_path: full path of a file to check.
        :returns: True if file is valid otherwise false.
        """
        logger.information("Validate CRYSTAL phonon file: ")
        return self._validate_dft_file_extension(filename_full_path=filename_full_path,
                                                 expected_file_extension=".out")

    def _validate_castep_input_file(self, filename_full_path=None):
        """
        Check if input DFT phonon file has been produced by CASTEP. Currently the crucial keywords in the first few
        lines are checked (to be modified if a better validation is found...)


        :param filename_full_path: full path of a file to check
        :returns: Dictionary with two entries "Invalid", "Comment". Valid key can have two values: True/ False. As it
                 comes to "Comment" it is an empty string if Valid:True, otherwise stores description of the problem.
        """
        logger.information("Validate CASTEP phonon file: ")
        msg_err = "Invalid %s file. " % filename_full_path
        output = self._validate_dft_file_extension(filename_full_path=filename_full_path,
                                                   expected_file_extension=".phonon")
        if output["Invalid"]:
            return output

        # check a structure of the header part of file.
        # Here fortran convention is followed: case of letter does not matter
        with open(filename_full_path) as castep_file:

            line = self._get_one_line(castep_file)
            if not self._compare_one_line(line, "beginheader"):  # first line is BEGIN header
                return dict(Invalid=True, Comment=msg_err + "The first line should be 'BEGIN header'.")

            line = self._get_one_line(castep_file)
            if not self._compare_one_line(one_line=line, pattern="numberofions"):
                return dict(Invalid=True, Comment=msg_err + "The second line should include 'Number of ions'.")

            line = self._get_one_line(castep_file)
            if not self._compare_one_line(one_line=line, pattern="numberofbranches"):
                return dict(Invalid=True, Comment=msg_err + "The third line should include 'Number of branches'.")

            line = self._get_one_line(castep_file)
            if not self._compare_one_line(one_line=line, pattern="numberofwavevectors"):
                return dict(Invalid=True, Comment=msg_err + "The fourth line should include 'Number of wavevectors'.")

            line = self._get_one_line(castep_file)
            if not self._compare_one_line(one_line=line,
                                          pattern="frequenciesin"):
                return dict(Invalid=True, Comment=msg_err + "The fifth line should be 'Frequencies in'.")

        return output

    def _get_one_line(self, file_obj=None):
        """

        :param file_obj:  file object from which reading is done
        :returns: string containing one non empty line
        """
        line = file_obj.readline().replace(" ", "").lower()

        while line and line == "":
            line = file_obj.readline().replace(" ", "").lower()

        return line

    def _compare_one_line(self, one_line, pattern):
        """
        compares line in the the form of string with a pattern.
        :param one_line:  line in the for mof string to be compared
        :param pattern: string which should be present in the line after removing white spaces and setting all
                        letters to lower case
        :returns:  True is pattern present in the line, otherwise False
        """
        return one_line and pattern in one_line.replace(" ", "")

    def _get_properties(self):
        """
        Loads all properties to object's attributes.
        """

        self._dft_program = self.getProperty("DFTprogram").value
        self._phonon_file = self.getProperty("PhononFile").value
        self._experimental_file = self.getProperty("ExperimentalFile").value
        self._temperature = self.getProperty("Temperature").value
        self._scale = self.getProperty("Scale").value
        self._sample_form = self.getProperty("SampleForm").value

        instrument_name = self.getProperty("Instrument").value
        instrument_producer = AbinsModules.InstrumentProducer()
        self._instrument = instrument_producer.produce_instrument(name=instrument_name)

        self._atoms = self.getProperty("Atoms").value
        self._sum_contributions = self.getProperty("SumContributions").value

        # conversion from str to int
        self._num_quantum_order_events = int(self.getProperty("QuantumOrderEventsNumber").value)

        self._scale_by_cross_section = self.getPropertyValue('ScaleByCrossSection')
        self._out_ws_name = self.getPropertyValue('OutputWorkspace')
        self._calc_partial = (len(self._atoms) > 0)

        # user defined interval is exclusive with respect to
        # AbinsModules.AbinsParameters.min_wavenumber
        # AbinsModules.AbinsParameters.max_wavenumber
        # with bin width AbinsModules.AbinsParameters.bin_width
        step = AbinsModules.AbinsParameters.bin_width
        start = AbinsModules.AbinsParameters.min_wavenumber + step / 2.0
        stop = AbinsModules.AbinsParameters.max_wavenumber + step / 2.0
        self._bins = np.arange(start=start, stop=stop, step=step, dtype=AbinsModules.AbinsConstants.FLOAT_TYPE)


try:
    AlgorithmFactory.subscribe(Abins)
except ImportError:
    logger.debug('Failed to subscribe algorithm SimulatedDensityOfStates; The python package may be missing.')
