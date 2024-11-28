# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

# Supporting functions for the Abins Algorithm that don't belong in
# another part of AbinsModules.
import dataclasses
from functools import cached_property
from math import isnan
import os
from pathlib import Path
import re
from typing import Dict, Iterable, List, Literal, Tuple, Union

import yaml

try:
    from yaml import CSafeLoader as SafeLoader
except ImportError:
    from yaml import SafeLoader

import numpy as np
from mantid.api import mtd, FileAction, FileProperty, WorkspaceGroup, WorkspaceProperty
from mantid.kernel import Atom, Direction, StringListValidator, StringArrayProperty, logger
from mantid.simpleapi import CloneWorkspace, SaveAscii, Scale

from abins.constants import AB_INITIO_FILE_EXTENSIONS, ALL_INSTRUMENTS, ATOM_PREFIX
from abins.input.jsonloader import abins_supported_json_formats, JSONLoader
from abins.instruments import get_instrument, Instrument
import abins.parameters


@dataclasses.dataclass
class AtomInfo:
    symbol: str
    mass: float

    @cached_property
    def name(self):
        if self.nucleons_number:
            return f"{self.nucleons_number}{self.symbol}"
        return self.symbol

    @property
    def z_number(self):
        return self._mantid_atom.z_number

    @property
    def nucleons_number(self):
        return self._mantid_atom.a_number

    @cached_property
    def neutron_data(self):
        return self._mantid_atom.neutron()

    @cached_property
    def _mantid_atom(self):
        nearest_int = int(round(self.mass))
        nearest_isotope = Atom(symbol=self.symbol, a_number=nearest_int)
        standard_mix = Atom(symbol=self.symbol)

        if abs(nearest_isotope.mass - standard_mix.mass) < 1e-12:
            # items are the same: standard mix is more likely to contain data
            # (e.g. Atom('F', 19) has no neutron data but Atom('F') does)
            return standard_mix

        # Return data closest to requested mass
        return min((nearest_isotope, standard_mix), key=(lambda atom: abs(atom.mass - self.mass)))


class AbinsAlgorithm:
    """Class providing shared utility for multiple inheritence by 1D, 2D implementations"""

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)  # i.e. forward everything to PythonAlgorithm

        # User input private properties
        self._instrument_name = None

        self._vibrational_or_phonon_data_file = None
        self._ab_initio_program = None
        self._out_ws_name = None
        self._temperature = None
        self._atoms = None
        self._sum_contributions = None
        self._save_ascii = None
        self._scale_by_cross_section = None

        self._num_quantum_order_events = None
        self._autoconvolution = None
        self._energy_units = None

        # Interally-used private properties
        self._max_event_order = None
        self._bin_width = None

    def get_common_properties(self) -> None:
        """From user input, set properties common to Abins 1D and 2D versions"""
        self._ab_initio_program = self.getProperty("AbInitioProgram").value
        self._vibrational_or_phonon_data_file = self.getProperty("VibrationalOrPhononFile").value
        self._out_ws_name = self.getPropertyValue("OutputWorkspace")

        self._temperature = self.getProperty("TemperatureInKelvin").value

        self._atoms = self.getProperty("Atoms").value
        self._sum_contributions = self.getProperty("SumContributions").value
        self._save_ascii = self.getProperty("SaveAscii").value
        self._scale_by_cross_section = self.getPropertyValue("ScaleByCrossSection")

        self._energy_units = self.getProperty("EnergyUnits").value

        # conversion from str to int
        self._num_quantum_order_events = int(self.getProperty("QuantumOrderEventsNumber").value)
        self._max_event_order = self._num_quantum_order_events  # This default can be replaced in child class

    def set_instrument(self) -> None:
        """Instantiate self._instrument using name and self._instrument_kwargs"""
        instrument_name = self.getProperty("Instrument").value
        if instrument_name in ALL_INSTRUMENTS:
            self._instrument_name = instrument_name
            self._instrument = get_instrument(self._instrument_name, **self._instrument_kwargs)
        else:
            raise ValueError("Unknown instrument %s" % instrument_name)

    def get_instrument(self) -> Union[Instrument, None]:
        return self._instrument

    def declare_common_properties(self, version: int = 1) -> None:
        """Declare properties common to Abins 1D and 2D versions"""
        self.declareProperty(
            FileProperty(
                "VibrationalOrPhononFile", "", action=FileAction.Load, direction=Direction.Input, extensions=AB_INITIO_FILE_EXTENSIONS
            ),
            doc="File with the data from a vibrational or phonon calculation.",
        )

        self.declareProperty(
            name="AbInitioProgram",
            direction=Direction.Input,
            defaultValue="CASTEP",
            validator=StringListValidator(["CASTEP", "CRYSTAL", "DMOL3", "FORCECONSTANTS", "GAUSSIAN", "JSON", "VASP"]),
            doc="An ab initio program which was used for vibrational or phonon calculation.",
        )

        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", Direction.Output), doc="Name to give the output workspace.")

        self.declareProperty(
            name="TemperatureInKelvin",
            direction=Direction.Input,
            defaultValue=10.0,
            doc="Temperature in K for which dynamical structure factor S should be calculated.",
        )

        self.declareProperty(
            StringArrayProperty("Atoms", Direction.Input),
            doc="List of atoms to use to calculate partial S."
            "If left blank, workspaces with S for all types of atoms will be calculated. "
            "Element symbols will be interpreted as a sum of all atoms of that element in the "
            "cell. 'atomN' or 'atom_N' (where N is a positive integer) will be interpreted as "
            "individual atoms, indexing from 1 following the order of the input data.",
        )

        self.declareProperty(
            name="SumContributions",
            defaultValue=(False if version == 1 else True),
            doc="Sum the partial dynamical structure factors into a single workspace.",
        )

        self.declareProperty(name="SaveAscii", defaultValue=False, doc="Write workspaces to .ascii files after computing them.")

        self.declareProperty(
            name="ScaleByCrossSection",
            defaultValue="Incoherent",
            validator=StringListValidator(["Total", "Incoherent", "Coherent"]),
            doc="Scale the partial dynamical structure factors by the scattering cross section.",
        )

        if version == 1:
            self.declareProperty(
                name="QuantumOrderEventsNumber",
                defaultValue="1",
                validator=StringListValidator(["1", "2"]),
                doc="Number of quantum order effects included in the calculation "
                "(1 -> FUNDAMENTALS, 2-> first overtone + FUNDAMENTALS + 2nd order combinations",
            )

            self.declareProperty(
                name="Autoconvolution", defaultValue=False, doc="Estimate higher quantum orders by convolution with fundamental spectrum."
            )

        else:
            autoconvolution_max = str(abins.parameters.autoconvolution["max_order"])
            self.declareProperty(
                name="QuantumOrderEventsNumber",
                defaultValue=autoconvolution_max,
                validator=StringListValidator(["1", "2", autoconvolution_max]),
                doc="Number of quantum order effects included in the calculation "
                "(1 -> Fundamentals, 2-> add first overtone and 2nd order combinations, 10-> add 8 orders by self-convolution",
            )

        self.declareProperty(
            name="EnergyUnits",
            defaultValue="cm-1",
            direction=Direction.Input,
            validator=StringListValidator(["cm-1", "meV"]),
            doc="Energy units for output workspace and experimental file",
        )

    def declare_instrument_properties(
        self,
        default: str = "TOSCA",
        choices: Iterable = ALL_INSTRUMENTS,
        multiple_choice_settings: List[Tuple[str, str, str]] = [],
        freeform_settings: List[Tuple[str, str, str]] = [],
    ):
        """Declare properties related to instrument

        Args:
            default: default instrument
            choices: Iterable of available instruments for "Instrument" combo box
            multiple_choice_settings:
                List of field names, corresponding parameter and tooltip for additional instrument settings, e.g.::

                    [('Setting', 'settings', 'Setting choice for this instrument (e.g. monochromator)'), ...]

                This should correspond to a dict in abins.parameters.instruments[instrument]


            freeform_settings:
                List of field names, defaults and tooltips for additional instrument settings, e.g.::

                    [('IncidentEnergy', '4100', 'Incident energy in wavenumber'), ...]
        """

        self.declareProperty(
            name="Instrument",
            direction=Direction.Input,
            defaultValue=default,
            validator=StringListValidator(choices),
            doc="Name of an instrument for which analysis should be performed.",
        )

        for property_name, parameter_name, doc in multiple_choice_settings:
            # Populate list of possible instrument settings
            valid_choices = [""]
            for instrument in choices:
                if (instrument in abins.parameters.instruments) and (parameter_name in abins.parameters.instruments[instrument]):
                    valid_choices += list(abins.parameters.instruments[instrument][parameter_name])
            valid_choices = sorted(list(set(valid_choices)))

            self.declareProperty(
                name=property_name, direction=Direction.Input, defaultValue="", validator=StringListValidator(valid_choices), doc=doc
            )

        for property_name, default, doc in freeform_settings:
            self.declareProperty(name=property_name, direction=Direction.Input, defaultValue=default, doc=doc)

    def validate_common_inputs(self, issues: dict = None) -> Dict[str, str]:
        """Validate inputs common to Abins 1D and 2D versions

        Args:
            abins: Algorithm instance for validation
            issues: Collection of validation issues to append to. (This will be mutated without copying.)

        Returns:
            issues dict including any new issues
        """
        if issues is None:
            issues = {}

        input_file_validators = {
            "CASTEP": self._validate_castep_input_file,
            "CRYSTAL": self._validate_crystal_input_file,
            "DMOL3": self._validate_dmol3_input_file,
            "FORCECONSTANTS": self._validate_euphonic_input_file,
            "GAUSSIAN": self._validate_gaussian_input_file,
            "JSON": self._validate_json_input_file,
            "VASP": self._validate_vasp_input_file,
        }
        ab_initio_program = self.getProperty("AbInitioProgram").value
        vibrational_or_phonon_data_filename = self.getProperty("VibrationalOrPhononFile").value

        output = input_file_validators[ab_initio_program](vibrational_or_phonon_data_filename)
        if output["Invalid"]:
            issues["VibrationalOrPhononFile"] = output["Comment"]

        workspace_name = self.getPropertyValue("OutputWorkspace")
        # list of special keywords which cannot be used in the name of workspace
        forbidden_keywords = {
            "total",
        }
        if workspace_name in mtd:
            issues["OutputWorkspace"] = (
                "Workspace with name " + workspace_name + " already in use; please give " "a different name for workspace."
            )
        elif workspace_name == "":
            issues["OutputWorkspace"] = "Please specify name of workspace."
        for word in forbidden_keywords:
            if word in workspace_name:
                issues["OutputWorkspace"] = "Keyword: " + word + " cannot be used in the name of workspace."
                break

        temperature = self.getProperty("TemperatureInKelvin").value
        if temperature < 0:
            issues["TemperatureInKelvin"] = "Temperature must be positive."

        return issues

    def _validate_instrument_settings(self, name="Setting", parameter="settings") -> Dict[str, str]:
        """Check that multiple-choice parameter is compatible with selected Instrument"""
        instrument_name = self.getProperty("Instrument").value
        setting = self.getProperty(name).value

        if instrument_name not in abins.parameters.instruments:
            # If an instrument lacks an entry in abins.parameters, we cannot
            # reason about how appropriate the setting is; assume good.
            return {}

        parameters = abins.parameters.instruments.get(instrument_name)

        if setting == "":
            if parameter + "_default" in parameters:
                return {}
            else:
                return {
                    "Setting": f'Instrument "{instrument_name}" does not have a default '
                    + "setting, and no setting was specified. Accepted settings: "
                    + ", ".join(parameters[parameter].keys())
                }

        downcased_settings = {s.lower(): s for s in parameters[parameter]}
        if setting.lower() in downcased_settings:
            return {}

        return {
            name: f'{name}: "{setting}" is unknown for instrument '
            + f"{instrument_name}. Supported values: "
            + ", ".join(sorted(parameters[parameter].keys()))
        }

    @staticmethod
    def get_atom_selection(*, atoms_data: abins.AtomsData, selection: list) -> Tuple[list, list]:
        """Interpret the user 'Atoms' input as a set of elements and atom indices

        (These atom indices match the user-facing convention and begin at 1.)"""

        num_atoms = len(atoms_data)
        all_atms_smbls = list(set([atoms_data[atom_index]["symbol"] for atom_index in range(num_atoms)]))
        all_atms_smbls.sort()

        if len(selection) == 0:  # case: all atoms
            atom_symbols = all_atms_smbls
            atom_numbers = []
        else:  # case selected atoms
            # Specific atoms are identified with prefix and integer index, e.g 'atom_5'. Other items are element symbols
            # A regular expression match is used to make the underscore separator optional and check the index format
            atom_symbols = [item for item in selection if item[: len(ATOM_PREFIX)] != ATOM_PREFIX]
            if len(atom_symbols) != len(set(atom_symbols)):  # only different types
                raise ValueError(
                    "User atom selection (by symbol) contains repeated species. This is not permitted as "
                    "Abins cannot create multiple workspaces with the same name."
                )

            numbered_atom_test = re.compile("^" + ATOM_PREFIX + r"_?(\d+)$")
            atom_numbers = [numbered_atom_test.findall(item) for item in selection]  # Matches will be lists of str
            atom_numbers = [int(match[0]) for match in atom_numbers if match]  # Remove empty matches, cast rest to int

            if len(atom_numbers) != len(set(atom_numbers)):
                raise ValueError(
                    "User atom selection (by number) contains repeated atom. This is not permitted as Abins"
                    " cannot create multiple workspaces with the same name."
                )

            for atom_symbol in atom_symbols:
                if atom_symbol not in all_atms_smbls:
                    raise ValueError("User defined atom selection (by element) '%s': not present in the system." % atom_symbol)

            for atom_number in atom_numbers:
                if atom_number < 1 or atom_number > num_atoms:
                    raise ValueError(
                        "Invalid user atom selection (by number) '%s%s': out of range (%s - %s)" % (ATOM_PREFIX, atom_number, 1, num_atoms)
                    )

            # Final sanity check that everything in "atoms" field was understood
            if len(atom_symbols) + len(atom_numbers) < len(selection):
                elements_report = " Symbols: " + ", ".join(atom_symbols) if len(atom_symbols) else ""
                numbers_report = " Numbers: " + ", ".join(atom_numbers) if len(atom_numbers) else ""
                raise ValueError("Not all user atom selections ('atoms' option) were understood." + elements_report + numbers_report)

        return atom_numbers, atom_symbols

    @staticmethod
    def get_masses_table(atoms_data):
        """
        Collect masses associated with each element in atoms_data

        :param num_atoms: Number of atoms in the system. (Saves time working out iteration.)
        :type num_atoms: int

        :returns: Mass data in form ``{el1: [m1, ...], ... }``
        """
        masses = {}
        for atom in atoms_data:
            symbol = atom["symbol"]
            mass = atom["mass"]
            if symbol not in masses:
                masses[symbol] = set()
            masses[symbol].add(mass)

        # convert set to list to fix order
        for s in masses:
            masses[s] = sorted(list(set(masses[s])))

        return masses

    def create_workspaces(self, atoms_symbols=None, atom_numbers=None, *, s_data, atoms_data, max_quantum_order):
        """
        Creates workspaces for all types of atoms. Creates both partial and total workspaces for given types of atoms.

        :param atoms_symbols: atom types (i.e. element symbols) for which S should be created.
        :type iterable of str:

        :param atom_numbers:
            indices of individual atoms for which S should be created. (One-based numbering; 1 <= I <= NUM_ATOMS)
        :type iterable of int:

        :param s_data: dynamical factor data
        :type abins.SData

        :param atoms_data: atom positions/masses
        :type abins.AtomsData:

        :param max_quantum_order: maximum quantum order to include
        :type int:

        :returns: workspaces for list of atoms types, S for the particular type of atom
        """
        from abins.constants import FLOAT_TYPE

        # Create appropriately-shaped arrays to be used in-place by _atom_type_s - avoid repeated slow instantiation
        shape = [max_quantum_order]
        shape.extend(list(s_data[0]["order_1"].shape))
        s_atom_data = np.zeros(shape=tuple(shape), dtype=FLOAT_TYPE)
        temp_s_atom_data = np.copy(s_atom_data)

        num_atoms = len(s_data)
        masses = self.get_masses_table(atoms_data)

        result = []

        if atoms_symbols is not None:
            for symbol in atoms_symbols:
                for m in masses[symbol]:
                    result.extend(
                        self._atom_type_s(
                            num_atoms=num_atoms,
                            mass=m,
                            s_data=s_data,
                            atoms_data=atoms_data,
                            element_symbol=symbol,
                            temp_s_atom_data=temp_s_atom_data,
                            s_atom_data=s_atom_data,
                        )
                    )
        if atom_numbers is not None:
            for atom_number in atom_numbers:
                result.extend(self._atom_number_s(atom_number=atom_number, s_data=s_data, s_atom_data=s_atom_data, atoms_data=atoms_data))
        return result

    def _create_workspace(self, *, species: AtomInfo, s_points: np.ndarray, label: str | None = None):
        """
        Creates workspace for the given frequencies and s_points with S data. After workspace is created it is rebined,
        scaled by cross-section factor and optionally multiplied by the user defined scaling factor.


        :param species: Object identifying isotope (or mixture)
        :param s_points: S(Q, omega)
        :param label: species-specific part of workspace name. If None, take from species object.
        :returns: workspace for the given frequency and S data
        """

        label = species.name if label is None else label

        ws_name = self._out_ws_name + "_" + label
        self._fill_s_workspace(
            species=species,
            s_points=s_points,
            workspace=ws_name,
        )
        return ws_name

    def _atom_number_s(self, *, atom_number, s_data, s_atom_data, atoms_data):
        """
        Helper function for calculating S for the given atomic index

        :param atom_number: One-based index of atom in s_data e.g. 1 to select first element 'atom_1'
        :type atom_number: int

        :param s_data: Precalculated S for all atoms and quantum orders
        :type s_data: abins.SData

        :param s_atom_data: helper array to accumulate S (outer loop over atoms); does not transport
            information but is used in-place to save on time instantiating large arrays. First dimension is quantum
            order; following dimensions should match arrays in s_data.
        :type s_atom_data: numpy.ndarray

        :param

        :returns: mantid workspaces of S for atom (total) and individual quantum orders
        :returntype: list of Workspace2D
        """
        from abins.constants import ATOM_PREFIX, FUNDAMENTALS

        atom_workspaces = []
        s_atom_data.fill(0.0)
        output_atom_label = "%s_%d" % (ATOM_PREFIX, atom_number)
        atom_data = atoms_data[atom_number - 1]
        species = AtomInfo(symbol=atom_data["symbol"], mass=atom_data["mass"])

        for i, order in enumerate(range(FUNDAMENTALS, self._max_event_order + 1)):
            s_atom_data[i] = s_data[atom_number - 1]["order_%s" % order]

        total_s_atom_data = np.sum(s_atom_data, axis=0)

        atom_workspaces = []
        atom_workspaces.append(
            self._create_workspace(
                species=species,
                s_points=np.copy(total_s_atom_data),
                label=output_atom_label + "_total",
            )
        )
        atom_workspaces.append(self._create_workspace(species=species, s_points=np.copy(s_atom_data), label=output_atom_label))
        return atom_workspaces

    def _atom_type_s(
        self,
        num_atoms=None,
        mass=None,
        s_data=None,
        atoms_data=None,
        element_symbol=None,
        temp_s_atom_data=None,
        s_atom_data=None,
    ):
        """
        Helper function for calculating S for the given type of atom

        :param num_atoms: number of atoms in the system
        :param s_data: Precalculated S for all atoms and quantum orders
        :type s_data: abins.SData
        :param atoms_data: Atomic position/mass data
        :type atoms_data: abins.AtomsData
        :param element_symbol: label for the type of atom
        :param temp_s_atom_data: helper array to accumulate S (inner loop over quantum order); does not transport
            information but is used in-place to save on time instantiating large arrays.
        :param s_atom_data: helper array to accumulate S (outer loop over atoms); does not transport
            information but is used in-place to save on time instantiating large arrays.
        """
        from abins.constants import MASS_EPS

        atom_workspaces = []
        s_atom_data.fill(0.0)

        species = AtomInfo(symbol=element_symbol, mass=mass)

        for atom_index in range(num_atoms):
            # TODO this mass_eps could be smaller, we are checking against known masses from data
            if atoms_data[atom_index]["symbol"] == element_symbol and abs(atoms_data[atom_index]["mass"] - mass) < MASS_EPS:
                temp_s_atom_data.fill(0.0)

                for order in range(1, self._max_event_order + 1):
                    order_indx = order - 1
                    temp_s_order = s_data[atom_index]["order_%s" % order]
                    temp_s_atom_data[order_indx] = temp_s_order

                s_atom_data += temp_s_atom_data  # sum S over the atoms of the same type

        total_s_atom_data = np.sum(s_atom_data, axis=0)

        atom_workspaces.append(
            self._create_workspace(
                species=species,
                s_points=np.copy(total_s_atom_data),
                label=f"{species.name}_total",
            )
        )
        atom_workspaces.append(
            self._create_workspace(
                species=species,
                s_points=np.copy(s_atom_data),
            )
        )

        return atom_workspaces

    def create_total_workspace(self, workspaces: list) -> None:
        """Sum together elemental totals to make an additional Total workspace"""
        total_atom_workspaces = []
        for ws in workspaces:
            if "total" in ws:
                total_atom_workspaces.append(ws)
        total_workspace = self._create_total_workspace(partial_workspaces=total_atom_workspaces)
        workspaces.insert(0, total_workspace)

    def _create_total_workspace(self, partial_workspaces=None):
        """
        Sets workspace with total S.
        :param partial_workspaces: list of workspaces which should be summed up to obtain total workspace
        :returns: workspace with total S from partial_workspaces
        """
        from abins.constants import ONE_DIMENSIONAL_INSTRUMENTS, TWO_DIMENSIONAL_INSTRUMENTS

        total_workspace = self._out_ws_name + "_total"

        if isinstance(mtd[partial_workspaces[0]], WorkspaceGroup):
            local_partial_workspaces = mtd[partial_workspaces[0]].names()
        else:
            local_partial_workspaces = partial_workspaces

        if len(local_partial_workspaces) > 1:
            # get frequencies
            ws = mtd[local_partial_workspaces[0]]

            # initialize S
            if self._instrument.get_name() in ONE_DIMENSIONAL_INSTRUMENTS:
                s_atoms = np.zeros_like(ws.dataY(0))

            if self._instrument.get_name() in TWO_DIMENSIONAL_INSTRUMENTS:
                n_q = abins.parameters.instruments[self._instrument.get_name()]["q_size"]
                n_energy_bins = ws.getDimension(1).getNBins()
                s_atoms = np.zeros([n_q, n_energy_bins])

            # collect all S
            for partial_ws in local_partial_workspaces:
                if self._instrument.get_name() in ONE_DIMENSIONAL_INSTRUMENTS:
                    s_atoms += mtd[partial_ws].dataY(0)

                elif self._instrument.get_name() in TWO_DIMENSIONAL_INSTRUMENTS:
                    for i in range(n_energy_bins):
                        s_atoms[:, i] += mtd[partial_ws].dataY(i)

            # create workspace with S
            self._fill_s_workspace(s_points=s_atoms, workspace=total_workspace)

        # # Otherwise just repackage the workspace we have as the total
        else:
            CloneWorkspace(InputWorkspace=local_partial_workspaces[0], OutputWorkspace=total_workspace)

        return total_workspace

    @staticmethod
    def write_workspaces_to_ascii(scale: float = 1.0, *, ws_name: str) -> None:
        """Write all with given root name to ascii files

        :param ws_name: Workspace name (to be searched for in Mantid context)
        :param scale: Scale factor to apply to data (typically 1 / bin_width)
        """
        num_workspaces = mtd[ws_name].getNumberOfEntries()
        for wrk_num in range(num_workspaces):
            wrk = mtd[ws_name].getItem(wrk_num)
            SaveAscii(
                InputWorkspace=Scale(wrk, scale, "Multiply", StoreInADS=False),
                Filename=wrk.name() + ".dat",
                Separator="Space",
                WriteSpectrumID=False,
            )

    @staticmethod
    def get_cross_section(scattering: Literal["Total", "Incoherent", "Coherent"], species: AtomInfo) -> float:
        """
        Calculates cross section for the given element.
        :param scattering: Type of cross-section: 'Incoherent', 'Coherent' or 'Total'
        :param species: Data for atom/isotope type
        :returns: cross section for that element
        """
        scattering_keys = {"Incoherent": "inc_scatt_xs", "Coherent": "coh_scatt_xs", "Total": "tot_scatt_xs"}
        cross_section = species.neutron_data[scattering_keys[scattering]]

        if isnan(cross_section):
            raise ValueError(f"Found NaN cross-section for {species.symbol} with {species.nucleons_number} nucleons.")

        return cross_section

    @staticmethod
    def set_workspace_units(wrk, layout="1D", energy_units="cm-1"):
        """
        Sets x and y units for a workspace.

        :param wrk: workspace which units should be set
        :type Workspace2D:

        :param layout: layout of data in Workspace2D.
            - '1D' is a typical indirect spectrum, with energy transfer on Axis
              0 (X), S on Axis 1 (Y)
            - '2D' is a 2D S(q,omega) map with momentum transfer on Axis 0 (X),
              S on Axis 1 and energy transfer on Axis 2
        :type str:

        :param energy_units:
            Preferred units in user-friendly notation. ('cm-1' or 'meV')
        :type str:
        """

        energy_units_map = {"cm-1": "DeltaE_inWavenumber", "mev": "DeltaE"}
        energy_units_str = energy_units_map.get(energy_units.lower(), "")

        if not energy_units_str:
            raise ValueError(f"Energy unit {energy_units} not recognised.")

        if layout == "1D":
            mtd[wrk].getAxis(0).setUnit(energy_units_str)
            mtd[wrk].setYUnitLabel("S / Arbitrary Units")
            mtd[wrk].setYUnit("Arbitrary Units")
        elif layout == "2D":
            mtd[wrk].getAxis(0).setUnit("MomentumTransfer")
            mtd[wrk].setYUnitLabel("S / Arbitrary Units")
            mtd[wrk].setYUnit("Arbitrary Units")
            mtd[wrk].getAxis(1).setUnit(energy_units_str)
        else:
            raise ValueError('Unknown data/units layout "{}"'.format(layout))

    @staticmethod
    def _validate_ab_initio_file_extension(*, ab_initio_program: str, filename_full_path: str, expected_file_extension: str) -> dict:
        """
        Checks consistency between name of ab initio program and extension.
        :param expected_file_extension: file extension
        :returns: dictionary with error message
        """
        msg_err = "Invalid %s file. " % filename_full_path
        msg_rename = "Please rename your file and try again."

        # check  extension of a file
        found_filename_ext = os.path.splitext(filename_full_path)[1]
        if found_filename_ext.lower() != expected_file_extension:
            comment = "{}Output from ab initio program {} is expected." " The expected extension of file is {}. Found: {}. {}".format(
                msg_err, ab_initio_program, expected_file_extension, found_filename_ext, msg_rename
            )
            return dict(Invalid=True, Comment=comment)
        else:
            return dict(Invalid=False, Comment="")

    @classmethod
    def _validate_dmol3_input_file(cls, filename_full_path: str) -> dict:
        """
        Method to validate input file for DMOL3 ab initio program.
        :param filename_full_path: full path of a file to check.
        :returns: True if file is valid otherwise false.
        """
        logger.information("Validate DMOL3 file with vibrational data.")
        return cls._validate_ab_initio_file_extension(
            ab_initio_program="DMOL3", filename_full_path=filename_full_path, expected_file_extension=".outmol"
        )

    @classmethod
    def _validate_gaussian_input_file(cls, filename_full_path: str) -> dict:
        """
        Method to validate input file for GAUSSIAN ab initio program.
        :param filename_full_path: full path of a file to check.
        :returns: True if file is valid otherwise false.
        """
        logger.information("Validate GAUSSIAN file with vibration data.")
        return cls._validate_ab_initio_file_extension(
            ab_initio_program="GAUSSIAN", filename_full_path=filename_full_path, expected_file_extension=".log"
        )

    @classmethod
    def _validate_crystal_input_file(cls, filename_full_path: str) -> dict:
        """
        Method to validate input file for CRYSTAL ab initio program.
        :param filename_full_path: full path of a file to check.
        :returns: True if file is valid otherwise false.
        """
        logger.information("Validate CRYSTAL file with vibrational or phonon data.")
        return cls._validate_ab_initio_file_extension(
            ab_initio_program="CRYSTAL", filename_full_path=filename_full_path, expected_file_extension=".out"
        )

    @classmethod
    def _validate_castep_input_file(cls, filename_full_path: str) -> dict:
        """
        Check if ab initio input vibrational or phonon file has been produced by CASTEP. Currently the crucial
        keywords in the first few lines are checked (to be modified if a better validation is found...)
        :param filename_full_path: full path of a file to check
        :returns: Dictionary with two entries "Invalid", "Comment". Valid key can have two values: True/ False. As it
                  comes to "Comment" it is an empty string if Valid:True, otherwise stores description of the problem.
        """

        logger.information("Validate CASTEP file with vibrational or phonon data.")
        msg_err = "Invalid %s file. " % filename_full_path
        output = cls._validate_ab_initio_file_extension(
            ab_initio_program="CASTEP", filename_full_path=filename_full_path, expected_file_extension=".phonon"
        )
        if output["Invalid"]:
            return output
        else:
            # check a structure of the header part of file.
            # Here fortran convention is followed: case of letter does not matter
            with open(filename_full_path) as castep_file:
                line = cls._get_one_line(castep_file)
                if not cls._compare_one_line(line, "beginheader"):  # first line is BEGIN header
                    return dict(Invalid=True, Comment=msg_err + "The first line should be 'BEGIN header'.")

                line = cls._get_one_line(castep_file)
                if not cls._compare_one_line(one_line=line, pattern="numberofions"):
                    return dict(Invalid=True, Comment=msg_err + "The second line should include 'Number of ions'.")

                line = cls._get_one_line(castep_file)
                if not cls._compare_one_line(one_line=line, pattern="numberofbranches"):
                    return dict(Invalid=True, Comment=msg_err + "The third line should include 'Number of branches'.")

                line = cls._get_one_line(castep_file)
                if not cls._compare_one_line(one_line=line, pattern="numberofwavevectors"):
                    return dict(Invalid=True, Comment=msg_err + "The fourth line should include 'Number of wavevectors'.")

                line = cls._get_one_line(castep_file)
                if not cls._compare_one_line(one_line=line, pattern="frequenciesin"):
                    return dict(Invalid=True, Comment=msg_err + "The fifth line should be 'Frequencies in'.")

                return dict(Invalid=False, Comment="")

    @classmethod
    def _validate_euphonic_input_file(cls, filename_full_path: str) -> dict:
        logger.information("Validate force constants file for interpolation.")

        path = Path(filename_full_path)

        if (suffix := path.suffix) == ".castep_bin":
            # Assume any .castep_bin file is valid choice
            pass

        elif suffix == ".yaml":
            # Check .yaml files have expected keys for Phonopy force constants
            with open(filename_full_path, "r") as yaml_file:
                phonon_data = yaml.load(yaml_file, Loader=SafeLoader)

            if {"phonopy", "force_constants"}.issubset(phonon_data):
                pass

            elif "phonopy" in phonon_data:
                # Phonon file without force constants included: they could be in
                # a FORCE_CONSTANTS or force_constants.hdf5 file so check if one exists
                fc_filenames = ("FORCE_CONSTANTS", "force_constants.hdf5")
                if not any(map(lambda fc_filename: (path.parent / fc_filename).is_file(), fc_filenames)):
                    return dict(
                        Invalid=True,
                        Comment=f"Could not find force constants in {filename_full_path}, or find data file {' or '.join(fc_filenames)}",
                    )

        # Did not return already: No problems found
        return dict(Invalid=False, Comment="")

    @classmethod
    def _validate_json_input_file(cls, filename_full_path: str) -> dict:
        logger.information("Validate JSON file with vibrational or phonon data.")
        output = cls._validate_ab_initio_file_extension(
            ab_initio_program="JSON", filename_full_path=filename_full_path, expected_file_extension=".json"
        )
        if output["Invalid"]:
            output["Comment"] = ".json extension is expected for a JSON file"
            return output

        json_format = JSONLoader.check_json_format(filename_full_path)
        if json_format in abins_supported_json_formats:
            return dict(Invalid=False, Comment=f"Found JSON file format: {json_format.name}")

        return dict(Invalid=True, Comment=f"Found unsupported JSON file format: {json_format.name}")

    @classmethod
    def _validate_vasp_input_file(cls, filename_full_path: str) -> dict:
        logger.information("Validate VASP file with vibrational or phonon data.")

        if "OUTCAR" in os.path.basename(filename_full_path):
            return dict(Invalid=False, Comment="")
        else:
            output = cls._validate_ab_initio_file_extension(
                ab_initio_program="VASP", filename_full_path=filename_full_path, expected_file_extension=".xml"
            )
            if output["Invalid"]:
                output["Comment"] = (
                    "Invalid filename {}. Expected OUTCAR, *.OUTCAR or"
                    " *.xml for VASP calculation output. Please rename your file and try again. ".format(filename_full_path)
                )
        return output

    @staticmethod
    def _get_one_line(file_obj=None):
        """

        :param file_obj:  file object from which reading is done
        :returns: string containing one non empty line
        """
        line = file_obj.readline().replace(" ", "").lower()

        while line and line == "":
            line = file_obj.readline().replace(" ", "").lower()

        return line

    @staticmethod
    def _compare_one_line(one_line, pattern):
        """
        compares line in the form of string with a pattern.
        :param one_line:  line in the for mof string to be compared
        :param pattern: string which should be present in the line after removing white spaces and setting all
                        letters to lower case
        :returns:  True is pattern present in the line, otherwise False
        """
        return one_line and pattern in one_line.replace(" ", "")

    @classmethod
    def _check_common_advanced_parameters(cls, message=""):
        cls._check_general_resolution(message)
        cls._check_folder_names(message)
        cls._check_rebinning(message)
        cls._check_threshold(message)
        cls._check_chunk_size(message)
        cls._check_threads(message)

    def _check_general_resolution(self, message_end=""):
        """
        Checks general parameters used in construction resolution functions.
        :param message_end: closing part of the error message.
        """
        # check fwhm
        fwhm = abins.parameters.instruments["fwhm"]
        if not (isinstance(fwhm, float) and 0.0 < fwhm < 10.0):
            raise RuntimeError("Invalid value of fwhm" + message_end)

    @staticmethod
    def _check_folder_names(message_end=""):
        """
        Checks folders names.
        :param message_end: closing part of the error message.
        """
        folder_names = []
        ab_initio_group = abins.parameters.hdf_groups["ab_initio_data"]
        if not isinstance(ab_initio_group, str) or ab_initio_group == "":
            raise RuntimeError("Invalid name for folder in which the ab initio data should be stored.")
        folder_names.append(ab_initio_group)

        powder_data_group = abins.parameters.hdf_groups["powder_data"]
        if not isinstance(powder_data_group, str) or powder_data_group == "":
            raise RuntimeError("Invalid value of powder_data_group" + message_end)
        elif powder_data_group in folder_names:
            raise RuntimeError("Name for powder_data_group already used by as name of another folder.")
        folder_names.append(powder_data_group)

        crystal_data_group = abins.parameters.hdf_groups["crystal_data"]
        if not isinstance(crystal_data_group, str) or crystal_data_group == "":
            raise RuntimeError("Invalid value of crystal_data_group" + message_end)
        elif crystal_data_group in folder_names:
            raise RuntimeError("Name for crystal_data_group already used as a name of another folder.")

        s_data_group = abins.parameters.hdf_groups["s_data"]
        if not isinstance(s_data_group, str) or s_data_group == "":
            raise RuntimeError("Invalid value of s_data_group" + message_end)
        elif s_data_group in folder_names:
            raise RuntimeError("Name for s_data_group already used as a name of another folder.")

    @staticmethod
    def _check_rebinning(message_end=""):
        """
        Checks rebinning parameters.
        :param message_end: closing part of the error message.
        """
        min_wavenumber = abins.parameters.sampling["min_wavenumber"]
        if not (isinstance(min_wavenumber, float) and min_wavenumber >= 0.0):
            raise RuntimeError("Invalid value of min_wavenumber" + message_end)

        max_wavenumber = abins.parameters.sampling["max_wavenumber"]
        if not (isinstance(max_wavenumber, float) and max_wavenumber > 0.0):
            raise RuntimeError("Invalid value of max_wavenumber" + message_end)

        if min_wavenumber > max_wavenumber:
            raise RuntimeError("Invalid energy window for rebinning.")

    @staticmethod
    def _check_threshold(message_end=""):
        """
        Checks threshold for frequencies.
        :param message_end: closing part of the error message.
        """
        from abins.parameters import sampling

        freq_threshold = sampling["frequencies_threshold"]
        if not (isinstance(freq_threshold, float) and freq_threshold >= 0.0):
            raise RuntimeError("Invalid value of frequencies_threshold" + message_end)

        # check s threshold
        s_absolute_threshold = sampling["s_absolute_threshold"]
        if not (isinstance(s_absolute_threshold, float) and s_absolute_threshold > 0.0):
            raise RuntimeError("Invalid value of s_absolute_threshold" + message_end)

        s_relative_threshold = sampling["s_relative_threshold"]
        if not (isinstance(s_relative_threshold, float) and s_relative_threshold > 0.0):
            raise RuntimeError("Invalid value of s_relative_threshold" + message_end)

    @staticmethod
    def _check_chunk_size(message_end=""):
        """
        Check optimal size of chunk
        :param message_end: closing part of the error message.
        """
        optimal_size = abins.parameters.performance["optimal_size"]
        if not (isinstance(optimal_size, int) and optimal_size > 0):
            raise RuntimeError("Invalid value of optimal_size" + message_end)

    @staticmethod
    def _check_threads(message_end=""):
        """
        Checks number of threads
        :param message_end: closing part of the error message.
        """
        try:
            import pathos.multiprocessing as mp

            threads = abins.parameters.performance["threads"]
            if not (isinstance(threads, int) and 1 <= threads <= mp.cpu_count()):
                raise RuntimeError("Invalid number of threads for parallelisation over atoms" + message_end)

        except ImportError:
            pass
