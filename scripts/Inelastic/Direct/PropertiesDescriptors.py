# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-lines
# pylint: disable=invalid-name
"""File contains collection of Descriptors used to define complex
properties in NonIDF_Properties and PropertyManager classes
"""

import os
import numpy as np
import math
import re
from collections.abc import Iterable, Callable
from mantid.api import mtd, FileFinder, Workspace
from mantid.simpleapi import DeleteWorkspace, GetAllEi, GetEi

import Direct.ReductionHelpers as prop_helpers
from Direct.AbsorptionShapes import anAbsorptionShape


# -----------------------------------------------------------------------------------------
# Descriptors, providing overloads for complex properties in NonIDF_Properties
# class
# -----------------------------------------------------------------------------------------
class PropDescriptor(object):
    """Class provides common custom interface for property descriptors"""

    def dependencies(self):
        """Returns the list of other properties names, this property depends on"""
        return []

    # pylint: disable=unused-argument

    def validate(self, instance, owner):
        """Interface to validate property descriptor,
        provided to check properties interaction before long run

        Return validation result, errors severity (0 -- fine, 1 -- warning, 2-- error)
        and error message if any
        """
        return (True, 0, "")


# end PropDescriptor

# -----------------------------------------------------------------------------------------
# -----------------------------------------------------------------------------------------
# -----------------------------------------------------------------------------------------
# -----------------------------------------------------------------------------------------


class SumRuns(PropDescriptor):
    """Boolean property specifies if list of files provided as input for sample_run property
    should be summed.
    """

    def __init__(self, sample_run_prop):
        # internal reference to sample run property
        self._sample_run = sample_run_prop
        # class containing this property
        #
        self._sum_runs = False

    #

    def __get__(self, instance, holder_class):
        if instance is None:
            return self
        return self._sum_runs

    #

    def __set__(self, instance, value):
        old_value = self._sum_runs
        if isinstance(value, bool):
            self._sum_runs = value
        elif isinstance(value, int):
            if value > 0:
                self._sum_runs = True
            else:
                self._sum_runs = False
        else:
            self._sum_runs = bool(value)
        #
        if old_value != self._sum_runs:
            self._sample_run.notify_sum_runs_changed(old_value, self._sum_runs)


# --------------------------------------------------------------------------------------------------------------------


class AvrgAccuracy(PropDescriptor):
    """Property-helper to round-of data nicely to provide consistent binning
    in auto-ei mode.
    """

    def __init__(self):
        self._accuracy = 2

    def __get__(self, instance, owner=None):
        """return current number of significant digits"""
        if instance is None:
            return self
        else:
            return self._accuracy

    def __set__(self, instance, value):
        """Set up incident energy or range of energies in various formats
        or define autoEi"""
        val = int(value)
        if val < 1:
            raise ValueError("Averaging accuracy can be only a positive number >= 1")
        self._accuracy = val

    def roundoff(self, value):
        """Round specified sequence for specified number of significant digits"""
        if isinstance(value, Iterable):
            vallist = value
        else:
            vallist = [value]
        rez = []
        lim = 10 ** (self._accuracy - 1)

        def out(a, b, mult):
            if mult > 1:
                return a < b
            else:
                return False

        for val in vallist:
            if abs(val) > lim:
                rez.append(round(val, 0))
                continue
            elif abs(val) < lim:
                mult = 10
            else:
                mult = 1

            tv = abs(val)
            fin_mult = 1
            while out(tv, lim, mult):
                fin_mult *= mult
                tv *= mult
            fin_rez = math.copysign(round(tv, 0) / fin_mult, val)
            rez.append(fin_rez)
        if len(rez) == 1:
            return rez[0]
        else:
            return rez


# --------------------------------------------------------------------------------------------------------------------
class IncidentEnergy(PropDescriptor):
    """Provide incident energy or range of incident energies to be processed.

    Set it up to list of values (even with single value i.e. prop_man.incident_energy=[10]),
    if the energy_bins property value to be treated as relative energy ranges.

    Set it up to single value (e.g. prop_man.incident_energy=10) to treat energy_bins
    as absolute energy values.
    """

    def __init__(self):
        self._incident_energy = 0
        self._num_energies = 1
        self._cur_iter_en = 0
        # Properties related to autoEi
        self._use_autoEi = False
        self._autoEiRunNumber = None
        self._autoEiCalculated = False

    def __get__(self, instance, owner=None):
        """return  incident energy or list of incident energies"""
        if instance is None:
            return self
        if self._use_autoEi:
            return "AUTO"
        else:
            return self._incident_energy
            # pylint: disable=too-many-branches

    def __set__(self, instance, value):
        """Set up incident energy or range of energies in various formats
        or define autoEi"""
        if value is not None:
            if isinstance(value, str):
                # Check autoEi
                if value.lower() == "auto":
                    self._use_autoEi = True
                    currentRun = instance.sample_run
                    if isinstance(currentRun, Workspace):
                        currentRun = currentRun.getRunNumber()
                    if currentRun != self._autoEiRunNumber or currentRun is None:
                        self._autoEiRunNumber = currentRun
                        self._autoEiCalculated = False
                        self._incident_energy = 0
                    self._cur_iter_en = 0
                    return
                # Ei in string form is provided
                else:
                    if value.find("[") > -1:
                        energy_list = True
                        value = value.replace("[", "").replace("]", "").strip()
                    else:
                        energy_list = False
                    en_list = value.split(",")
                    if len(en_list) > 1:
                        rez = []
                        for en_str in en_list:
                            val = float(en_str)
                            rez.append(val)
                        self._incident_energy = rez
                    else:
                        if energy_list:
                            self._incident_energy = [float(value)]
                        else:
                            self._incident_energy = float(value)
            else:
                if isinstance(value, list):
                    rez = []
                    for val in value:
                        en_val = float(val)
                        if en_val <= 0:
                            raise KeyError("Incident energy has to be positive, but is: {0} ".format(en_val))
                        else:
                            rez.append(en_val)
                    self._incident_energy = rez
                else:
                    self._incident_energy = float(value)
        else:
            raise KeyError("Incident energy have to be positive number of list of positive numbers. Got None")
        # here we have set specific energy or range of incident energies
        self._use_autoEi = False

        if isinstance(self._incident_energy, list):
            self._num_energies = len(self._incident_energy)
        else:
            self._num_energies = 1
        self._cur_iter_en = 0
        # pylint: disable=unused-variable
        ok, sev, message = self.validate(instance)
        if not ok:
            raise KeyError(message)

    #

    def autoEi_mode(self):
        """Return true if energies should be calculated in autoEi mode"""
        return self._use_autoEi

    #

    def multirep_mode(self):
        """return true if energy is defined as list of energies or multirep m and false otherwise"""
        if isinstance(self._incident_energy, list) or self._use_autoEi:
            return True
        else:
            return False

    #

    def getAllEiList(self):
        """Return incident energy(ies) range, defined by the property"""
        if isinstance(self._incident_energy, list):
            return self._incident_energy
        else:
            if self._incident_energy == 0:
                return []
            else:
                return [self._incident_energy]

    #

    def get_current(self):
        """Return current energy out of range of energies"""
        if isinstance(self._incident_energy, list):
            ind = self._cur_iter_en
            return self._incident_energy[ind]
        else:
            return self._incident_energy

    #

    # Python 3 compatibility
    def __next__(self):
        return self.next()

    def next(self):
        if isinstance(self._incident_energy, list):
            self._cur_iter_en += 1
            if self._cur_iter_en >= len(self._incident_energy):
                raise StopIteration
        else:
            raise StopIteration

    #
    def set_current_ind(self, ind=None):
        """set current energy value (used in multirep mode) as
        energy estimate for the reduction

        ind -- if provided, the number of the value in the list of
        values (can be used together with enumerate)
        """
        if isinstance(self._incident_energy, list):
            if ind is None:
                self._cur_iter_en = 0
            else:
                if ind > len(self._incident_energy):
                    raise IndexError("Index exceed number of energies")
                self._cur_iter_en = ind
        else:
            self._cur_iter_en = 0

    #

    def set_auto_Ei(self, monitor_ws, instance, ei_mon_spec=None):
        """Calculate autoEi and set it as input for iterations over energy"""
        if not self._use_autoEi:
            return
        newRunNum = monitor_ws.getRunNumber()
        if self._autoEiCalculated and newRunNum == self._autoEiRunNumber:
            return
        # Calculate autoEi
        self._autoEiCalculated = False
        if ei_mon_spec is None:
            ei_mon_spec = instance.ei_mon_spectra
        guess_ei_ws = GetAllEi(Workspace=monitor_ws, Monitor1SpecID=ei_mon_spec[0], Monitor2SpecID=ei_mon_spec[1])
        guessEi = guess_ei_ws.readX(0)
        fin_ei = []
        for ei in guessEi:
            try:
                ei_ref, _, _, _ = GetEi(
                    InputWorkspace=monitor_ws, Monitor1Spec=ei_mon_spec[0], Monitor2Spec=ei_mon_spec[1], EnergyEstimate=ei
                )
                fin_ei.append(ei_ref)
            # pylint: disable=bare-except
            except:
                instance.log("Can not refine guess energy {0:f}. Ignoring it.".format(ei), "warning")
        if len(fin_ei) == 0:
            raise RuntimeError("Was not able to identify auto-energies for workspace: {0}".format(monitor_ws.name()))
        # Success! Set up estimated energies
        self._autoEiCalculated = True
        self._autoEiRunNumber = newRunNum
        self._incident_energy = fin_ei
        self._num_energies = len(fin_ei)
        self._cur_iter_en = 0
        # Clear dataservice from unnecessary workspace
        DeleteWorkspace(guess_ei_ws)

    def validate(self, instance, owner=None):
        #
        if self._use_autoEi:  # nothing much to validate. The ei will be auto if possible
            return (True, 0, "")

        inc_en = self._incident_energy
        if isinstance(inc_en, list):
            for ind, en in enumerate(inc_en):
                if en <= 0:
                    return (
                        False,
                        2,
                        "Incident energy have to be positive number or list of positive numbers.\n"
                        "For input argument {0} got negative energy {1}".format(ind, en),
                    )
        else:
            if inc_en <= 0:
                return (
                    False,
                    2,
                    "Incident energy have to be positive number or list of positive numbers.\n"
                    "Got single negative incident energy {0} ".format(inc_en),
                )
        return (True, 0, "")


# end IncidentEnergy
# -----------------------------------------------------------------------------------------


class EnergyBins(PropDescriptor):
    """Energy binning, expected in final converted to energy transfer workspace.

    Provide it in the form:
    propman.energy_bins = [min_energy,step,max_energy]
    if energy to process (incident_energy property) has a single value,
    or
    propman.energy_bins = [min_rel_enrgy,rel_step,max_rel_energy]
    where all values are relative to the incident energy,
    if energy(ies) to process (incident_energy(ies)) are list of energies.
    The list of energies can contain only single value.
    (e.g. prop_man.incident_energy=[100])/
    """

    def __init__(self, IncidentEnergyProp, AccuracyProp):
        self._incident_energy = IncidentEnergyProp
        self._averager = AccuracyProp
        self._energy_bins = None
        # how close you are ready to rebin w.r.t.  the incident energy
        self._range = 0.99999

    def __get__(self, instance, owner=None):
        """Binning range for the result of convertToenergy procedure or list of such ranges"""
        if instance is None:
            return self
        return self._energy_bins

    def __set__(self, instance, values):
        if values is not None:
            if isinstance(values, str):
                values = values.replace("[", "").replace("]", "").strip()
                lst = values.split(",")
                self.__set__(instance, lst)
                return
            else:
                value = values
                if len(value) != 3:
                    raise KeyError("Energy_bin value has to be a tuple of 3 elements or string of 3 comma-separated numbers")
                value = (float(value[0]), float(value[1]), float(value[2]))
        else:
            value = None
            # TODO: implement single value settings according to rebin?
        self._energy_bins = value

    def get_abs_range(self, instance=None):
        """return energies related to incident energies."""
        if self._incident_energy.multirep_mode():  # Relative energy
            ei = self._incident_energy.get_current()
            if self._incident_energy.autoEi_mode():
                # we need to average ei nicely, as it will give energy jitter otherwise
                ei = self._averager.roundoff(ei)

            if self._energy_bins:
                if self.is_range_valid():
                    rez = self._calc_relative_range(ei)
                else:
                    if instance:
                        instance.log(
                            "*** WARNING! Got energy_bins specified as absolute values in multirep mode.\n"
                            "          Will normalize these values by max value and treat as relative values",
                            "warning",
                        )
                    mult = self._range / self._energy_bins[2]
                    rez = self._calc_relative_range(ei, mult)
                if self._incident_energy.autoEi_mode():
                    rez = self._averager.roundoff(rez)
                return rez
            else:
                return None
        else:  # Absolute energy ranges
            if self.is_range_valid():
                return self._energy_bins
            else:
                if instance:
                    instance.log(
                        "*** WARNING! Requested maximum binning range exceeds incident energy!\n"
                        "             Will normalize binning range by max value and treat as relative range",
                        "warning",
                    )
                    mult = self._range / self._energy_bins[2]
                    ei = self._incident_energy.get_current()
                    return self._calc_relative_range(ei, mult)

    def is_range_valid(self):
        """Method verifies if binning range is consistent with incident energy"""
        if self._incident_energy.multirep_mode():
            return self._energy_bins[2] <= self._range
        else:
            return self._energy_bins[2] <= self._incident_energy.get_current()

    def _calc_relative_range(self, ei, range_mult=1):
        """ """
        mult = range_mult * ei
        return (self._energy_bins[0] * mult, self._energy_bins[1] * mult, self._energy_bins[2] * mult)

    def validate(self, instance, owner):
        """function verifies if the energy binning is consistent with incident energies"""
        ei = instance.incident_energy
        ebin = instance.energy_bins
        if isinstance(ei, list) or owner.incident_energy.autoEi_mode():  # ebin expected to be relative
            if ebin[2] > 1:
                return (
                    False,
                    1,
                    "Binning for multiple energy range should be relative to incident energy. Got ebin_max={0} > 1\n"
                    + "Energy range will be normalized and treated as relative range",
                )
        else:
            if not owner.incident_energy.autoEi_mode() and ebin[2] > ei:
                return (False, 2, "Max rebin range {0:f} exceeds incident energy {1:f}".format(ebin[2], ei))
        return (True, 0, "")


# -----------------------------------------------------------------------------------------
# end EnergyBins
# -----------------------------------------------------------------------------------------


class SaveFileName(PropDescriptor):
    """Property defines default file name to save result.

    By default the name is calculated from incident energy, run number and
    other properties values, but user can assign its own name, which will be
    used instead.
    """

    def __init__(self, Name=None):
        self._file_name = Name
        self._custom_print = None

    def __get__(self, instance, owner=None):
        # getter functional interface.
        if instance is None:
            return self

        # if custom file name provided, use it
        if self._file_name:
            return self._file_name

        # if custom function to generate file name is proivede, use this function
        if self._custom_print is not None:
            return self._custom_print()

        # user provided nothing.
        # calculate default target file name from
        # instrument, energy and run number.
        if instance.instr_name:
            name = instance.short_inst_name
        else:
            name = "_EMPTY"

        sr = owner.sample_run.run_number()
        if not sr:
            sr = 0
        try:
            ei = owner.incident_energy.get_current()
            name += "{0:0<5}Ei{1:_<4.2f}meV".format(sr, ei)
            if instance.sum_runs:
                name += "sum"
            if owner.monovan_run.run_number():
                name += "_Abs"
            name = name.replace(".", "d")
        # pylint: disable=bare-except
        except:
            name = None
        return name

    def __set__(self, instance, value):
        if value is None:
            self._file_name = None
        elif isinstance(value, Callable):
            self._custom_print = value
        else:
            self._file_name = str(value)

    def set_custom_print(self, routine):
        self._custom_print = routine


# end SaveFileName
# -----------------------------------------------------------------------------------------


class InstrumentDependentProp(PropDescriptor):
    """Generic property describing some aspects of instrument (e.g. name, short name etc),
    which are undefined if no instrument is defined.
    """

    def __init__(self, prop_name):
        self._prop_name = prop_name

    def __get__(self, instance, owner=None):
        if instance is None:
            return self
        # pylint: disable=protected-access
        if instance._pInstrument is None:
            raise KeyError("Attempt to use uninitialized property manager")
        else:
            return getattr(instance, self._prop_name)

    def __set__(self, instance, values):
        raise AttributeError("Property {0} can not be assigned. It defined by instrument".format(self._prop_name))


# end InstrumentDependentProp
# -----------------------------------------------------------------------------------------


class VanadiumRMM(PropDescriptor):
    """Defines constant static rmm for vanadium."""

    def __get__(self, instance, owner=None):
        """return rmm for vanadium"""

        return 50.9415

    def __set__(self, instance, value):
        raise AttributeError("Can not change vanadium rmm")


# end VanadiumRMM
# -----------------------------------------------------------------------------------------
# END Descriptors for NonIDF_Properties class
# -----------------------------------------------------------------------------------------

# -----------------------------------------------------------------------------------------
# Descriptors, providing overloads for some complex properties in
# PropertyManager
# -----------------------------------------------------------------------------------------


class mon2NormalizationEnergyRange(PropDescriptor):
    """Energy range to integrate signal on monitor 2 when normalized by this monitor.

    This class contains relative range of energies in which the monitor-2 signal should
    be integrated, and returns the energy range for integration according to
    formula: range = [min_range*ei,max_range*ei] where ei is incident monitor energy.

    To find the actual integration ranges one should convert these values into TOF (or
    convert monitor signal to energy).
    """

    def __init__(self):
        # default range
        self._relative_range = [0.8, 1.2]

    def __get__(self, instance, owner):
        """Return actual energy range from internal relative range and incident energy"""
        if instance is None:
            return self
        ei = owner.incident_energy.get_current()
        return [self._relative_range[0] * ei, self._relative_range[1] * ei]

    def __set__(self, instance, val):
        """set detector calibration file using various formats"""
        if isinstance(val, list):
            self._relative_range = self._check_range(val, instance)
        elif isinstance(val, str):
            val = self._parce_string2list(val)
            self.__set__(instance, val)
        else:
            raise KeyError(
                "mon2_norm_energy_range needs to be initialized by two values.\nTrying to assign value {0} of unknown type {1}".format(
                    val, type(val)
                )
            )

    #

    def _check_range(self, val, instance):
        """method to check if list of values acceptable as ranges"""

        if len(val) != 2:
            raise KeyError("mon2_norm_energy_range needs to be initialized by lost of two values. Got {0}".format(len(val)))
        self._relative_range = (float(val[0]), float(val[1]))
        ok, sev, message = self.validate(instance)
        if not ok:
            if sev == 1:
                instance.log(message, "warning")
            else:
                raise KeyError(message)

        return self._relative_range

    #

    def _parce_string2list(self, val):
        """method splits input string containing comma into list of strings"""
        value = val.strip("[]()")
        val = value.split(",")
        return val

    def validate(self, instance, owner=None):
        """function verifies if the energy range is consistent with incident energies"""
        en_range = self._relative_range
        if len(en_range) != 2:
            return (
                False,
                2,
                "mon2_normalization_energy_range can be initialized by list of two values only. Got {0} values".format(len(range)),
            )

        result = (True, 0, "")

        val1 = float(en_range[0])
        if val1 < 0.1 or val1 > 0.9:
            message = (
                "Lower mon2_norm_energy_range describes lower limit of energy to integrate neutron signal after"
                " the chopper.\nThe limit is defined as (this value)*incident_energy."
                " Are you sure you want to set this_value to {0}?\n".format(val1)
            )
            if val1 > 1:
                return (False, 2, message)
            else:
                result = (False, 1, message)

        val2 = float(en_range[1])
        if val2 < 1.1 or val2 > 1.9:
            message = (
                "Upper mon2_norm_energy_range describes upper limit of energy to integrate neutron signal after"
                " the chopper.\nThe limit is defined as (this value)*incident_energy."
                " Are you sure you want to set this_value to {0}?\n".format(val2)
            )
            if val2 > 1:
                if result[0]:
                    result = (False, 1, message)
                else:
                    result = (False, 1, result[2] + message)
            else:
                return (False, 2, message)

        return result


# -----------------------------------------------------------------------------------------


class PropertyFromRange(PropDescriptor):
    """Generic descriptor for property, which can have one value from a list of values."""

    def __init__(self, availible_values, default_value):
        self._availible_values = availible_values
        self.__set__(None, default_value)

    def __get__(self, instance, owner):
        """Return current value for the property with range of values."""
        if instance is None:
            return self
        return self._current_value

    def __set__(self, instance, val):
        if val in self._availible_values:
            # pylint: disable=attribute-defined-outside-init
            self._current_value = val
        else:
            raise KeyError(" Property can not have value {0}".format(val))


# -----------------------------------------------------------------------------------------


class DetCalFile(PropDescriptor):
    """Provide a source of the detector calibration information.

    A source can be a file, present on a data search path, a workspace
    or a run number, corresponding to a file to be loaded as a
    workspace.
    """

    def __init__(self):
        self._det_cal_file = None
        self._calibrated_by_run = False

    def __get__(self, instance, owner):
        if instance is None:
            return self

        return self._det_cal_file

    def __set__(self, instance, val):
        """set detector calibration file using various formats"""

        if val is None or isinstance(val, Workspace):
            # nothing provided or workspace provided or filename probably provided
            if str(val) in mtd:
                # workspace name provided
                val = mtd[str(val)]
            self._det_cal_file = val
            self._calibrated_by_run = False
            return
        if isinstance(val, str):
            if val in mtd:
                val = mtd[val]
                self._det_cal_file = val
                self._calibrated_by_run = False
                return
            try:
                intVal = int(val)
            except ValueError:
                self._det_cal_file = val
                self._calibrated_by_run = False
                return
            val = intVal

        if isinstance(val, int):
            # if val in instance.all_run_numbers: TODO: retrieve workspace from
            # run numbers
            self._det_cal_file = val
            self._calibrated_by_run = True
            return
        raise NameError("Detector calibration file name can be a workspace name present in Mantid or string describing an file name")

    # if Reducer.det_cal_file != None :
    #    if isinstance(Reducer.det_cal_file,str) and not Reducer.det_cal_file
    #    in mtd : # it is a file
    #        Reducer.log('Setting detector calibration file to
    #        '+Reducer.det_cal_file)
    #    else:
    #       Reducer.log('Setting detector calibration to {0}, which is probably
    #       a workspace '.format(str(Reducer.det_cal_file)))
    # else:
    #    Reducer.log('Setting detector calibration to detector block info from
    #    '+str(sample_run))

    def calibrated_by_run(self):
        """reports if the detector calibration is in a run-file or separate file(workspace)"""
        return self._calibrated_by_run

    # pylint: disable=unused-argument

    def find_file(self, instance, **kwargs):
        """Method to find file, correspondent to
        current _det_cal_file file hint
        """
        if self._det_cal_file is None:
            # nothing to look for
            return (True, "No Detector calibration file defined")

        if isinstance(self._det_cal_file, Workspace):
            # nothing to do.  Workspace used for calibration
            return (True, "Workspace {0} used for detectors calibration".format(self._det_cal_file.name()))

        dcf_val = self._det_cal_file
        if isinstance(dcf_val, str):  # it may be string representation of runN
            try:
                dcf_val = int(dcf_val)
            except ValueError:
                pass

        if isinstance(dcf_val, int):  # this is a run number
            inst_short_name = instance.short_inst_name
            fac = instance.facility
            zero_padding = fac.instrument(inst_short_name).zeroPadding(dcf_val)
            file_hint = inst_short_name + str(dcf_val).zfill(zero_padding)
            try:
                file_name = FileFinder.findRuns(file_hint)[0]
            # pylint: disable=bare-except
            except:
                return (False, "Can not find run file corresponding to run N: {0}".format(file_hint))
            self._det_cal_file = file_name
            return (True, file_name)

            # string can be a run number or a file name:
        file_name = prop_helpers.findFile(self._det_cal_file)
        if len(file_name) == 0:  # it still can be a run number as string
            try:
                file_name = FileFinder.findRuns(self._det_cal_file)[0]
            # pylint: disable=bare-except
            except:
                return (False, "Can not find file or run file corresponding to name : {0}".format(self._det_cal_file))
        else:
            pass
        self._det_cal_file = file_name
        return (True, file_name)


# end DetCalFile
# -----------------------------------------------------------------------------------------


class MapMaskFile(PropDescriptor):
    """common method to wrap around an auxiliary file name"""

    def __init__(self, prop_name, file_ext, doc_string=None):
        self._file_name = None
        self._file_ext = file_ext
        self._prop_name = prop_name

        if doc_string is not None:
            self.__doc__ = doc_string

    def __get__(self, instance, class_type=None):
        if instance is None:
            return self

        return self._file_name

    def __set__(self, instance, value):
        if value is not None:
            # pylint: disable=unused-variable
            fileName, fileExtension = os.path.splitext(value)
            if not fileExtension:
                value = value + self._file_ext
        self._file_name = value

    # pylint: disable=unused-argument

    def find_file(self, instance, **kwargs):
        """Method to find file, correspondent to
        current MapMaskFile file hint
        """
        if self._file_name is None:
            return (True, "No file for {0} is defined".format(self._prop_name))

        file_name = prop_helpers.findFile(self._file_name)
        if len(file_name) == 0:  # it still can be a run number as string
            return (False, "No file for {0} corresponding to guess: {1} found".format(self._prop_name, self._file_name))
        else:
            self._file_name = file_name
            return (True, file_name)


# end MapMaskFile

# -----------------------------------------------------------------------------------------


class HardMaskPlus(prop_helpers.ComplexProperty):
    """Sets diagnostics algorithm to use hard mask file
    together with all other diagnostics routines.

    Assigning a mask file name to this property sets up hard_mask_file property
    to the file name provided, and use_hard_mask_only property to False,
    so that both hard mask file and the diagnostics procedures are used to identify
    and exclude failing detectors.
    """

    def __init__(self):
        prop_helpers.ComplexProperty.__init__(self, ["use_hard_mask_only", "run_diagnostics"])

    def __get__(self, instance, class_type=None):
        if instance is None:
            return self

        return instance.hard_mask_file

    def __set__(self, instance, value):
        if value is not None:
            # pylint: disable=unused-variable
            fileName, fileExtension = os.path.splitext(value)
            if not fileExtension:
                value = value + ".msk"
            instance.hard_mask_file = value
            prop_helpers.ComplexProperty.__set__(self, instance.__dict__, [False, True])
        else:
            prop_helpers.ComplexProperty.__set__(self, instance.__dict__, [True, False])
        try:
            # pylint: disable=protected-access
            del instance.__changed_properties["hardmaskOnly"]
        # pylint: disable=bare-except
        except:
            pass


# -----------------------------------------------------------------------------------------


class HardMaskOnly(prop_helpers.ComplexProperty):
    """Sets diagnostics algorithm to use hard mask file and to disable all other diagnostics.

    Assigning a mask file name to this property sets up hard_mask_file property
    to the file name provided and use_hard_mask_only property to True, so that
    only hard mask file provided is used to exclude failing detectors.
    """

    def __init__(self):
        prop_helpers.ComplexProperty.__init__(self, ["use_hard_mask_only", "run_diagnostics"])

    def __get__(self, instance, owner=None):
        if instance is None:
            return self

        return prop_helpers.gen_getter(instance.__dict__, "use_hard_mask_only")

    def __set__(self, instance, value):
        if isinstance(value, bool) or isinstance(value, int):
            use_hard_mask_only = bool(value)
            hard_mask_file = instance.hard_mask_file
        elif isinstance(value, str):
            if value.lower() in ["true", "yes"]:
                use_hard_mask_only = True
            elif value.lower() in ["false", "no"]:
                use_hard_mask_only = False
            else:  # it is probably a hard mask file provided:
                instance.hard_mask_file = value
                use_hard_mask_only = True
            hard_mask_file = instance.hard_mask_file
        else:
            use_hard_mask_only = False
            instance.hard_mask_file = None
            hard_mask_file = None

        # if no hard mask file is there and use_hard_mask_only is True,
        # diagnostics should not run
        if use_hard_mask_only and hard_mask_file is None:
            run_diagnostics = False
        else:
            run_diagnostics = True
        prop_helpers.ComplexProperty.__set__(self, instance.__dict__, [use_hard_mask_only, run_diagnostics])
        try:
            # pylint: disable=protected-access
            del instance.__changed_properties["hardmaskPlus"]
        # pylint: disable=bare-except
        except:
            pass


# end HardMaskOnly
# -----------------------------------------------------------------------------------------


class MonovanIntegrationRange(prop_helpers.ComplexProperty):
    """Integration range for monochromatic vanadium.

    The calculated integral is used to estimate relative detector's efficiency.
    The range can be defined either directly or as the function of the incident energy(s).
    (incident_energy property).

    The default settings are the range, relative to the incident energy.

    To define range as relative, one needs to set this property to None.
    In this case, the actual boundaries are calculated from monovan_lo_frac, monovan_lo_frac
    and incident_energy properties values according to the expression:
    integration_range = current_incident_energy*[monovan_lo_frac,monovan_hi_frac].

    The range should be changed by changing monovan_lo_frac, monovan_hi_frac separately.

    To define range as absolute, one needs to assign this property with the range requested.
    In this case, the dependent properties monovan_lo_value,monovan_hi_value are set and the
    actual range is calculated according to the expression:
    integration_range = [monovan_lo_value,monovan_hi_value].
    """

    def __init__(self, DepType=None):
        if DepType:
            self._rel_range = False
            prop_helpers.ComplexProperty.__init__(self, ["monovan_lo_value", "monovan_hi_value"])
        else:
            self._rel_range = True
            prop_helpers.ComplexProperty.__init__(self, ["monovan_lo_frac", "monovan_hi_frac"])

    def __get__(self, instance, owner=None):
        if instance is None:
            return self

        if isinstance(instance, dict):
            ei = 1
            tDict = instance
        else:
            ei = owner.incident_energy.get_current()
            tDict = instance.__dict__

        if self._rel_range:  # relative range
            if ei is None:
                raise AttributeError("Attempted to obtain relative to ei monovan integration range, but incident energy has not been set")
            rel_range = prop_helpers.ComplexProperty.__get__(self, tDict)
            the_range = [rel_range[0] * ei, rel_range[1] * ei]
            return the_range
        else:  # absolute range
            return prop_helpers.ComplexProperty.__get__(self, tDict)

    def __set__(self, instance, value):
        if isinstance(instance, dict):
            tDict = instance
        else:
            tDict = instance.__dict__
        if value is None:
            if not self._rel_range:
                self._rel_range = True
                self._other_prop = ["monovan_lo_frac", "monovan_hi_frac"]
        else:
            if self._rel_range:
                self._rel_range = False
                self._other_prop = ["monovan_lo_value", "monovan_hi_value"]

            if isinstance(value, str):
                values = value.split(",")
                result = []
                for val in values:
                    result.append(int(val))
                value = result
            if len(value) != 2:
                raise KeyError(
                    "monovan_integr_range has to be list of two values, "
                    "defining min/max values of integration range or None "
                    "to use relative to incident energy limits"
                )

            prop_helpers.ComplexProperty.__set__(self, tDict, value)

    def validate(self, instance, owner):
        """check if monovan integration range has reasonable value"""

        if instance.monovan_run is None:
            return (True, 0, "")

        the_range = self.__get__(instance, owner)
        ei = instance.incident_energy
        if the_range[0] >= the_range[1]:
            return False, 2, "monovan integration range limits = [{0}:{1}] are wrong".format(the_range[0], the_range[1])
        if the_range[0] < -100 * ei or the_range[0] > 100 * ei:
            return (
                False,
                1,
                "monovan integration is suspiciously wide: [{0}:{1}]. This may be incorrect".format(the_range[0], the_range[1]),
            )
        return True, 0, ""


# end MonovanIntegrationRange


class EiMonSpectra(prop_helpers.ComplexProperty):
    """Property defines list of spectra, used to calculate incident energy.
    It defines two monitor spectra for GetEi algorithm to work
    or two spectra lists to sum and obtain workspace with two combined spectra
    for GetEi algorithm to work.
    """

    def __init__(self):
        """Ei mon spectra is defined as function of two other complex properties"""
        prop_helpers.ComplexProperty.__init__(self, ["ei-mon1-spec", "ei-mon2-spec"])

    def __get__(self, instance, owner=None):
        if instance is None:
            return self

        if isinstance(instance, dict):
            tDict = instance
        else:
            tDict = instance.__dict__

        mon_range = prop_helpers.ComplexProperty.__get__(self, tDict)

        # Return monitors range, converted into the standard form.
        # namely tuple of two monitors or monitors list
        monitors = [0, 0]
        for index, mon_val in enumerate(mon_range):
            monitors[index] = self._process_monitors_spectra(mon_val)
        return (monitors[0], monitors[1])

    def __set__(self, instance, value):
        if value is None:  # Do nothing
            return

        if isinstance(instance, dict):
            tDict = instance
        else:
            tDict = instance.__dict__

        if isinstance(value, str):
            val = value.replace("[", "").replace("]", "").strip()
            if val.find(":") > -1:
                val = val.split(":")
            else:
                val = val.split(",")
        else:
            val = value
        # end if
        if len(val) != 2:
            raise KeyError(
                "ei_mon_spectra may be either tuple defining two spectra "
                "lists or string, which can be transformed to such lists. Got {0}".format(value)
            )

        # Handle self-assignment. Otherwise suppose that the values are meaningful
        if val[0] == "ei-mon1-spec":
            val[0] = tDict["ei-mon1-spec"]
        if val[1] == "ei-mon2-spec":
            val[1] = tDict["ei-mon2-spec"]
        #
        prop_helpers.ComplexProperty.__set__(self, tDict, val)

    def need_to_sum_monitors(self, instance):
        """Returns True if some monitors are defined as range of spectra to sum
        False -- if all monitors are related to one spectra each.
        """
        tDict = instance.__dict__
        mon_range = prop_helpers.ComplexProperty.__get__(self, tDict)
        # Return monitors range, converted into the standard form.
        need_to_sum = False
        for mon_val in mon_range:
            mon_list = self._process_monitors_spectra(mon_val)
            if isinstance(mon_list, Iterable):
                need_to_sum = True
                break
        return need_to_sum

    #
    def _process_monitors_spectra(self, mon_range):
        """A method to process any form of monitor spectra list
           representation
        Called on get rather then on set operation as monitor lists
        can be set separately through ei-mon1-spec or 'ei-mon2-spec
        properties and these properties are currently standard properties
        """

        if isinstance(mon_range, str):
            mon_val = mon_range.split(",")
        else:
            mon_val = mon_range
        if isinstance(mon_val, list) or isinstance(mon_val, tuple):
            rez_spectra = []
            for mon in mon_val:
                rez_spectra.append(int(mon))
            if len(rez_spectra) == 1:
                rez_spectra = rez_spectra[0]
        else:
            rez_spectra = int(mon_range)
        return rez_spectra


# def validate(self,instance, owner):
#        """ """
#        return (True,0,'')

# -----------------------------------------------------------------------------------------


class SpectraToMonitorsList(PropDescriptor):
    """Define list of spectra, with detectors used as monitors to estimate incident energy
    in a direct scattering experiment.

    Necessary when a detector, especially a detector working in event mode, is used as a monitor.
    Specifying this number would copy correspondent spectra to monitor workspace and rebin the spectra
    according to monitors binning.

    Written to work with old IDF too, where this property is absent and property defaults to empty list.
    """

    def __init__(self):
        self._spectra_to_monitors_list = None

    def __get__(self, instance, owner=None):
        if instance is None:
            return self
        return self._spectra_to_monitors_list

    def __set__(self, instance, spectra_list):
        """Sets copy spectra to monitors variable as a list of monitors using different forms of input"""
        self._spectra_to_monitors_list = self._convert_to_list(spectra_list)

    def _convert_to_list(self, spectra_list):
        """convert any spectra_list representation into a list"""
        if spectra_list is None:
            return None

        if isinstance(spectra_list, str):
            if spectra_list.lower() == "none":
                result = None
            else:
                spectra = spectra_list.split(",")
                result = []
                for spectum in spectra:
                    result.append(int(spectum))

        else:
            if isinstance(spectra_list, list):
                if len(spectra_list) == 0:
                    result = None
                else:
                    result = []
                    for i in range(0, len(spectra_list)):
                        result.append(int(spectra_list[i]))
            else:
                result = [int(spectra_list)]
        return result


# end SpectraToMonitorsList

# -----------------------------------------------------------------------------------------


class SaveFormat(PropDescriptor):
    """The format to save reduced results using internal save procedure.

    Can be one name or list of supported format names. Currently supported formats
    are: spe, nxspe and nxs data formats.
    See Mantid documentation for detailed description of the formats.
    If set to None, internal saving procedure is not used.
    """

    # formats available for saving the data
    save_formats = ["spe", "nxspe", "nxs"]

    def __init__(self):
        self._save_format = set()

    def __get__(self, instance, owner=None):
        if instance is None:
            return self

        return self._save_format

    def __set__(self, instance, value):
        """user can clear save formats by setting save_format=None or save_format = [] or save_format=''
        if empty string or empty list is provided as part of the list, all save_format-s set up earlier are cleared"""

        # clear format by using None
        if value is None:
            self._save_format = set()
            return

        # check string
        if isinstance(value, str):
            value = value.strip("[]().")
            subformats = value.split(",")
            if len(subformats) > 1:
                self.__set__(instance, subformats)
                return
            else:
                value = subformats[0]

                if value not in SaveFormat.save_formats:
                    instance.log('Trying to set saving in unknown format: "' + str(value) + '" No saving will occur for this format')
                    return
        else:
            try:
                # set single default save format recursively
                for val in value:
                    self.__set__(instance, val)
                return
            except:
                raise KeyError(
                    " Attempting to set unknown saving format {0} of type {1}. Allowed values can be spe, nxspe or nxs".format(
                        value, type(value)
                    )
                )
        # end if different types
        self._save_format.add(value)

    def validate(self, instance, owner):
        n_formats = len(self._save_format)
        if n_formats == 0:
            return (False, 1, "No internal save format is defined. Results may be lost")
        else:
            return (True, 0, "")


# end SaveFormat

# -----------------------------------------------------------------------------------------


class DiagSpectra(PropDescriptor):
    """Provide groups of spectra where each group used in diagnostics separately.
    Final mask is calculated as sum of spectra, failing diagnostics in each group.

    Consist of tuples list where each tuple contains the numbers
    specifying first and last spectra in the group.

    if set to None or not set, all spectra are used in diagnostics together.
    """

    def __init__(self):
        self._diag_spectra = None

    def __get__(self, instance, owner=None):
        if instance is None:
            return self

        return self._diag_spectra

    def __set__(self, instance, spectra_list):
        self._diag_spectra = self._process_spectra_list(spectra_list)

    def _process_spectra_list(self, specta_sring):
        """process IDF description of the spectra string"""
        if specta_sring is None:
            return None
        if isinstance(specta_sring, str):
            if specta_sring.lower() in ["none", "no"]:
                return None
            else:
                banks = specta_sring.split(";")
                bank_spectra = []
                for b in banks:
                    token = b.split(",")  # b = "(,)"
                    if len(token) != 2:
                        raise ValueError("Invalid bank spectra specification in diagnostics properties %s" % specta_sring)
                    start = int(token[0].lstrip("("))
                    end = int(token[1].rstrip(")"))
                    bank_spectra.append((start, end))
            return bank_spectra
        else:
            raise ValueError("Spectra For diagnostics can be a string inthe form (num1,num2);(num3,num4) etc. or None")


# end class DiagSpectra

# -----------------------------------------------------------------------------------------


class BackbgroundTestRange(PropDescriptor):
    """The TOF range used in diagnostics to reject high background spectra.

    Usually it is the same range as the TOF range used to remove
    background (usually in powders) though it may be set up separately.
    """

    def __init__(self):
        self._background_test_range = None

    def __get__(self, instance, owner=None):
        if instance is None:
            return self

        if self._background_test_range:
            return self._background_test_range
        else:
            return instance.bkgd_range

    def __set__(self, instance, value):
        if value is None:
            self._background_test_range = None
            return
        if isinstance(value, str):
            value = value.split(",")
        if len(value) != 2:
            raise ValueError("background test range can be only a 2 element list of floats")
        self._background_test_range = (float(value[0]), float(value[1]))

    def validate(self, instance, owner=None):
        """validate background test range"""
        test_range = self.__get__(instance, owner)
        if test_range is None:
            return True, 0, ""
        if test_range[0] >= test_range[1]:
            return False, 2, " Background test range: [{0}:{1}] is incorrect ".format(test_range[0], test_range[1])
        if test_range[0] < 0:
            return False, 2, " Background test range is TOF range, so it can not be negative={0}".format(test_range[0])
        if test_range[1] > 20000:
            return False, 1, " Background test range is TOF range, its max value looks suspiciously big={0}".format(test_range[1])
        return True, 0, ""


# end BackbgroundTestRange

# -----------------------------------------------------------------------------------------


class MultirepTOFSpectraList(PropDescriptor):
    """Specify list of spectra numbers used to calculate
    TOF range corresponding to the particular energy range.

    Usually it is list of two numbers, specifying spectra with detectors placed
    closest and furthest from the sample.
    """

    def __init__(self):
        self._spectra_list = None

    def __get__(self, instance, owner=None):
        if instance is None:
            return self

        return self._spectra_list

    def __set__(self, instance, value):
        if value is None:
            self._spectra_list = None
            return
        if isinstance(value, str):
            value = value.split(",")
            self.__set__(instance, value)
            return
        if isinstance(value, list):
            rez = []
            for val in value:
                rez.append(int(val))
        else:
            rez = [int(value)]
        self._spectra_list = rez


# end MultirepTOFSpectraList


class MonoCorrectionFactor(PropDescriptor):
    """Provide correction factor to convert
    experimental scattering cross-section into absolute
    units ( mb/str/mev/fu).

    Two independent sources for this factor can be defined:
    1) if user explicitly specifies correction value.
        This value then will be applied to all subsequent runs
        without any checks if the correction is appropriate.
        Set to None to disable this behavior.
    2) set/get cashed value correspondent to current monovan
       run number, incident energy and integration range.
       This value is cashed at first run and reapplied if
       no changes to the values it depends on were identified.
    """

    def __init__(self, ei_prop, monovan_run_prop):
        self._cor_factor = None
        self._mono_run_number = None
        self._ei_prop = ei_prop
        self.cashed_values = {}
        self._mono_run_prop = monovan_run_prop

    def __get__(self, instance, owner=None):
        if instance is None:
            return self

        return self._cor_factor

    def __set__(self, instance, value):
        self._cor_factor = value
        #
        if value is None:
            # pylint: disable=protected-access
            self._mono_run_prop._in_cash = False  # enable monovan run validation if any

    #

    def set_val_to_cash(self, instance, value):
        """ """
        mono_int_range = instance.monovan_integr_range
        cash_id = self._build_cash_val_id(mono_int_range)
        self.cashed_values[cash_id] = value
        # tell property manager that mono_correction_factor has been modified
        # to avoid automatic resetting this property from any workspace
        cp = getattr(instance, "_PropertyManager__changed_properties")
        cp.add("mono_correction_factor")

    def get_val_from_cash(self, instance):
        mono_int_range = instance.monovan_integr_range
        cash_id = self._build_cash_val_id(mono_int_range)
        if cash_id in self.cashed_values:
            return self.cashed_values[cash_id]
        else:
            return None

    def set_cash_mono_run_number(self, new_value):
        if new_value is None:
            self.cashed_values = {}
            self._mono_run_number = None
            return
        if self._mono_run_number != int(new_value):
            self.cashed_values = {}
            self._mono_run_number = int(new_value)

    def _build_cash_val_id(self, mono_int_range):
        ei = self._ei_prop.get_current()
        cash_id = "Ei={0:0>9.4e}:Int({1:0>9.4e}:{2:0>9.5e}):Run{3}".format(ei, mono_int_range[0], mono_int_range[1], self._mono_run_number)
        return cash_id

    def validate(self, instance, owner=None):
        if self._cor_factor is None:
            return (True, 0, "")
        if self._cor_factor <= 0:
            return (False, 2, "Mono-correction factor has to be positive if specified: {0}".format(self._cor_factor))
        return (True, 0, "")


# end MonoCorrectionFactor


class MotorLogName(PropDescriptor):
    """The list of possible log names, with logs containing information
    on rotation of crystal.

    First log found on current workspace is used together with motor_offset
    property to identify crystal rotation (psi in Horace) and
    to write this psi value into nxspe file.
    """

    def __init__(self):
        self._log_names = []

    def __get__(self, instance, owner=None):
        if instance is None:
            return self
        return self._log_names

    def __set__(self, instance, value):
        if isinstance(value, str):
            val_list = value.split(";")
        elif isinstance(value, list):
            val_list = []
            for val in value:
                val_list.append(str(val))
        else:
            val_list = [str(value)]
        self._log_names = val_list


# end MotorLogName


class MotorOffset(PropDescriptor):
    """Initial value used to identify crystal rotation angle according to the formula:
    psi=motor_offset+wccr.getTimeAveragedValue() where wccr is the log describing
    crystal rotation. See motor_log_name property for its description.
    """

    def __init__(self):
        self._offset = None

    def __get__(self, instance, owner=None):
        if instance is None:
            return self
        return self._offset

    def __set__(self, instance, value):
        # we do not need to analyze for None or empty list
        # as all this is implemented within generic setter
        if value is None:
            self._offset = None
        else:
            self._offset = float(value)


# end MotorOffset


class RotationAngle(PropDescriptor):
    """Property used to identify rotation angle
    psi=motor_offset+wccr.getTimeAveragedValue().

    If set to None or not set, the rotation angle
    is calculated from motor_offset and log, containing
    property value. If set explicitly to some value,
    the value specified is used and stored in NXSPE files.
    """

    def __init__(self, MotorLogNamesClass, Motor_Offset):
        self._mot_offset = Motor_Offset
        self._motor_log = MotorLogNamesClass
        # user may override value derived
        # from log, by providing its own value
        # this value would be used instead of
        # calculations
        self._own_psi_value = None
        # user should define workspace, which contain rotation logs
        # Motor log will be read from this workspace
        self._log_ws_name = None

    #
    def __get__(self, instance, owner=None):
        if instance is None:
            return self

        if self._own_psi_value is None or np.isnan(self._own_psi_value):
            return self.read_psi_from_workspace(self._log_ws_name)
        else:
            return self._own_psi_value

    def __set__(self, instance, value):
        if isinstance(value, str):
            if value in mtd:  ## its workspace
                self._log_ws_name = value
                self._own_psi_value = None
            else:  # it is string representation of psi.  Should be
                # convertible to number.
                self._own_psi_value = float(value)
        elif isinstance(value, Workspace):
            self._log_ws_name = value.name()
            self._own_psi_value = None
        elif value is None:  # clear all
            self._own_psi_value = None
        else:  # own psi value
            self._own_psi_value = float(value)

    def _read_ws_logs(self, external_ws=None):
        """read specified workspace logs from workspace
        provided either internally or externally
        """
        working_ws = external_ws
        if working_ws is None:
            working_ws = mtd[self._log_ws_name]
        if working_ws is None:
            raise RuntimeError("No workspace provided. Can not read logs to identify psi")
        else:
            if isinstance(external_ws, str):
                working_ws = mtd[external_ws]

        value = None
        # pylint: disable=protected-access
        log_names = self._motor_log._log_names
        for name in log_names:
            try:
                value = working_ws.getRun().getTimeAveragedValue(name)
                break
            # pylint: disable=bare-except
            except:
                pass
        return value

    def read_psi_from_workspace(self, workspace):
        """Independent method to read rotation angle from workspace and
        previously set log and offset parameters
        """
        # pylint: disable=protected-access
        offset = self._mot_offset._offset
        if offset is None:
            return np.NaN
        log_val = self._read_ws_logs(workspace)
        if log_val is None:
            return np.NaN
        else:
            return offset + log_val

    def dependencies(self):
        return ["motor_log_names", "motor_offset"]


# end RotationAngle


class AbsCorrInfo(PropDescriptor):
    """Class responsible for providing additional values for
    absorption corrections algorithms.

    The values should contain of the algorithm selector currently
    is_fast:True for AbsorptionCorrection algorithms family
    or
    is_mc:True for MonteCarloAbsorption correction algorithms family

    accompanied by the dictionary of all non-sample related properties
    accepted by these algorithms.

    If this key is missing, the class assumes that MonteCarloAbsorption
    algorithm (is_mc:True) is selected.

    The value for the property can be provided as dictionary or as
    string representation of this dictionary.
    The whole contents of the previous dictionary is removed
    on assignment of the new dictionary.
    """

    # The set of properties acceptable by MonteCarloAbsorption algorithm:
    _MC_corrections_accepts = {
        "NumberOfWavelengthPoints": lambda x: int(x),
        "EventsPerPoint": lambda x: int(x),
        "SeedValue": lambda x: int(x),
        "Interpolation": lambda x: list_checker(x, ("Linear", "CSpline"), 'MonteCarloAbsorptions "Interpolation"'),
        "SparseInstrument": lambda x: bool(x),
        "NumberOfDetectorRows": lambda x: int(x),
        "NumberOfDetectorColumns": lambda x: int(x),
        "MaxScatterPtAttempts": lambda x: int(x),
    }
    # The set of properties acceptable by AbsorptionCorrection algorithm:
    _Fast_corrections_accepts = {
        "ScatterFrom": lambda x: list_checker(x, ("Sample", "Container", "Environment"), 'AbsorptionCorrections "ScatterFrom"'),
        "NumberOfWavelengthPoints": lambda x: float(x),
        "ExpMethod": lambda x: list_checker(x, ("Normal", "FastApprox"), 'AbsorptionCorrections "ExpMethod"'),
        "EMode": lambda x: list_checker(x, ("Direct", "Indirect", "Elastic"), 'AbsorptionCorrections "EMode"'),
        "EFixed": lambda x: float(x),
        "ElementSize": lambda x: float(x),
    }

    def __init__(self):
        self._alg_prop = {}
        # Use Monte-Carlo corrections by default
        self._is_fast = False

    def __get__(self, instance, owner=None):
        if instance is None:
            return self
        alg_prop = self._alg_prop
        if self._is_fast:
            alg_prop["is_fast"] = True
        else:
            alg_prop["is_mc"] = True
        return alg_prop

    def __set__(self, instance, value):
        if value is None or len(value) == 0:
            # Clear all previous values and set up the defaults
            self._alg_prop = {"is_mc": True}
            self._is_fast = False
            return
        val_dict = {}
        if isinstance(value, str):
            val = re.sub(" u ", " ", re.sub(r'[{}\[\]"=:;,\']', " ", value))
            val_list = re.split(r"\s+", val)
            ik = 0
            while ik < len(val_list):
                key = val_list[ik]
                if len(key) == 0:
                    ik += 1
                    continue
                val = val_list[ik + 1]
                if key in ("is_fast", "is_mc"):
                    self._algo_selector(key, val)
                    ik += 2
                    continue
                val_dict[key] = val
                ik += 2
        elif isinstance(value, dict):
            val_dict = value
            is_fast = val_dict.pop("is_fast", None)
            if is_fast is not None:
                self._algo_selector("is_fast", is_fast)
            is_mc = val_dict.pop("is_mc", None)
            if is_mc is not None:
                self._algo_selector("is_mc", is_mc)

        else:
            raise (
                KeyError(
                    "AbsCorrInfo accepts only a dictionary "
                    "with AbsorptionCorrections algorithm properties "
                    "or string representation of such dictionary"
                )
            )

        if self._is_fast:
            algo_name = "AdsorptionCorrection"
            acceptable_prop = AbsCorrInfo._Fast_corrections_accepts
        else:
            algo_name = "MonteCarloAbsorption"
            acceptable_prop = AbsCorrInfo._MC_corrections_accepts

        for key, val in val_dict.items():
            check_normalizer = acceptable_prop.get(key, None)
            if check_normalizer is None:
                raise KeyError("The key {0} is not acceptable key for {1} algorithm".format(key, algo_name))
            val_dict[key] = check_normalizer(val)
        # Store new dictionary in the property
        self._alg_prop = val_dict

    def __str__(self):
        alg_prop = self.__get__(self, AbsCorrInfo)
        return str(alg_prop)

    def _algo_selector(self, algo_key, key_val):
        if algo_key == "is_fast":
            self._is_fast = bool(key_val)
        elif algo_key == "is_mc":
            self._is_fast = not bool(key_val)


class AbsorptionShapesContainer(PropDescriptor):
    """Class to keep AbsorptionShape classes,
    responsible for making absorption corrections
    for the absorption on correspondent shapes.

    NOT IMPLEMENTED: should also handle conversion
    from a shape to its text representaton and v.v.
    (using AbsorptionShape appropriate classes)

    """

    def __init__(self):
        self._theShapeHolder = None

    #
    def __get__(self, instance, owner=None):
        if instance is None:
            return self

        return self._theShapeHolder

    def __set__(self, instance, value):
        if value is None:
            self._theShapeHolder = None
            return
        if isinstance(value, str):
            self._theShapeHolder = anAbsorptionShape.from_str(value)
        elif isinstance(value, anAbsorptionShape):
            self._theShapeHolder = value
        else:
            raise ValueError("The property accepts only anAbsorptionShape type classes or their string representation")

    def __str__(self):
        if self._theShapeHolder is None:
            return "None"
        else:
            return self._theShapeHolder.str()


def list_checker(val, list_in, mess_base):
    """Helper function to check the value val (first input) belongs to the
    set, specified as the second input.

    If this is not true, raise ValueError indicating what property is wrong.
    To do indication, mess_base should contain the string, referring to
    the invalid property.
    """
    if val in list_in:
        return val
    else:
        raise ValueError("{0} property can only have values from the set of: {1}".format(mess_base, str(list_in)))


# -----------------------------------------------------------------------------------------
# END Descriptors for PropertyManager itself
# -----------------------------------------------------------------------------------------
