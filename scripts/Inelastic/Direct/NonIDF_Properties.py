# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from Direct.PropertiesDescriptors import (
    prop_helpers,
    AvrgAccuracy,
    EnergyBins,
    IncidentEnergy,
    InstrumentDependentProp,
    SaveFileName,
    VanadiumRMM,
)
from Direct.RunDescriptor import RunDescriptor, RunDescriptorDependent
from mantid.api import mtd, ExperimentInfo
from mantid.kernel import config, logger
from mantid.simpleapi import LoadEmptyInstrument
from mantid import geometry

import os
from sys import platform


class NonIDF_Properties(object):
    """Class defines the interface for main properties, used in reduction, and not described in
    IDF ( Instrument_Properties.xml file)

    These properties are main set of properties, user have to set up
    for reduction to work with defaults.

    The example of such properties are run numbers, energy bins and incident energies.
    """

    # logging levels available for user
    # pylint: disable=unnecessary-lambda
    log_options = {
        "error": (1, lambda msg: logger.error(msg)),
        "warning": (2, lambda msg: logger.warning(msg)),
        "notice": (3, lambda msg: logger.notice(msg)),
        "information": (4, lambda msg: logger.information(msg)),
        "debug": (5, lambda msg: logger.debug(msg)),
    }

    # The default location of the archive load log file
    if platform.startswith("linux"):
        # linux default log file location
        archive_upload_log_template = "/archive/NDX{0}/Instrument/logs/lastrun.txt"
    elif platform == "win32":
        # windows default log file location
        archive_upload_log_template = r"\\isis\inst$\NDX{0}\Instrument\logs\lastrun.txt"
    else:
        archive_upload_log_template = ""
    # ----------------------------------------------------------------------------------

    #
    def __init__(self, Instrument, run_workspace=None):
        """initialize main properties, defined by the class
        @parameter Instrument  -- name or pointer to the instrument,
                   deployed in reduction
        """
        #
        if run_workspace is not None:
            object.__setattr__(self, "sample_run", run_workspace)

        # Helper properties, defining logging options
        object.__setattr__(self, "_log_level", "notice")
        object.__setattr__(self, "_log_to_mantid", False)

        object.__setattr__(self, "_current_log_level", 3)
        object.__setattr__(self, "_archive_upload_log_file", None)

        self._set_instrument_and_facility(Instrument, run_workspace)

        # set up descriptors holder class reference
        # pylint: disable=protected-access
        RunDescriptor._holder = self
        # pylint: disable=protected-access
        RunDescriptor._logger = self.log
        # Initiate class-level properties to defaults (Each constructor clears class-level properties?)
        super(NonIDF_Properties, self).__setattr__("sample_run", None)
        super(NonIDF_Properties, self).__setattr__("wb_run", None)
        super(NonIDF_Properties, self).__setattr__("monovan_run", None)
        super(NonIDF_Properties, self).__setattr__("empty_bg_run", None)
        super(NonIDF_Properties, self).__setattr__("empty_bg_run_for_wb", None)
        super(NonIDF_Properties, self).__setattr__("empty_bg_run_for_monovan", None)

        super(NonIDF_Properties, self).__setattr__("mask_run", None)
        super(NonIDF_Properties, self).__setattr__("wb_for_monovan_run", None)
        super(NonIDF_Properties, self).__setattr__("empty_bg_run_for_monoWb", None)
        super(NonIDF_Properties, self).__setattr__("second_white", None)
        super(NonIDF_Properties, self).__setattr__("_tmp_run", None)
        super(NonIDF_Properties, self).__setattr__("_cashe_sum_ws", False)
        super(NonIDF_Properties, self).__setattr__("_mapmask_ref_ws", None)

    # end
    def log(self, msg, level="notice"):
        """Send a log message to the location defined"""
        lev, logger = NonIDF_Properties.log_options[level]
        if self._log_to_mantid:
            logger(msg)
        else:
            # TODO: reconcile this with Mantid.
            if lev <= self._current_log_level:
                print(msg)

    def get_prop_class(self, prop_name):
        """Return the instance of a property if such property exist"""

        prop = NonIDF_Properties.__getattribute__(NonIDF_Properties, prop_name)
        return prop

    # -----------------------------------------------------------------------------
    # Complex properties with personal descriptors
    # -----------------------------------------------------------------------------
    incident_energy = IncidentEnergy()
    #
    auto_accuracy = AvrgAccuracy()
    #
    energy_bins = EnergyBins(incident_energy, auto_accuracy)
    #
    save_file_name = SaveFileName()
    #
    instr_name = InstrumentDependentProp("_instr_name")
    short_inst_name = InstrumentDependentProp("_short_instr_name")
    facility = InstrumentDependentProp("_facility")
    #
    van_rmm = VanadiumRMM()
    # Run descriptors
    sample_run = RunDescriptor(
        "SR_",
        """Run number, workspace or symbolic presentation of such run
                  containing data of scattering from a sample to convert to energy transfer.
                  Also accepts a list of the such run numbers""",
    )
    wb_run = RunDescriptor(
        "WB_",
        """Run number, workspace or symbolic presentation of such run
                  containing results of white beam neutron scattering from vanadium used in detectors calibration.""",
    )
    monovan_run = RunDescriptor(
        "MV_",
        """Run number, workspace or symbolic presentation of such run
                  containing results of monochromatic neutron beam scattering from vanadium sample
                  used in absolute units normalization.\n None disables absolute units calculations.""",
    )
    empty_bg_run = RunDescriptor(
        "EBG_",
        """Run number, workspace or symbolic presentation of such run
                  containing results of the empty instrument background measurements""",
    )
    empty_bg_run_for_wb = RunDescriptor(
        "EBG4WB_",
        """Run number, workspace or symbolic presentation of such run
                  containing results of the empty instrument background measurements for Vanadium""",
    )

    mask_run = RunDescriptorDependent(
        sample_run,
        "MSK_",
        """Run number, workspace or symbolic presentation of such run
                  containing results of experiment, used to find masks.\n If not explicitly set, sample_run is used.""",
    )
    empty_bg_run_for_monovan = RunDescriptorDependent(
        empty_bg_run,
        "EBG4Mono_",
        """Run number, workspace or symbolic presentation of such run
                  containing results of the empty instrument background measurements for monochromatic vanadium (WB normalisatiion)""",
    )
    wb_for_monovan_run = RunDescriptorDependent(
        wb_run,
        "MV_WB_",
        """Run number, workspace or symbolic presentation of such run
                         containing results of white beam neutrons scattering from vanadium, used to calculate monovanadium
                         integrals for monochromatic vanadium.\n
                         If not explicitly set, white beam for sample run is used.""",
    )
    empty_bg_run_for_monoWb = RunDescriptorDependent(
        empty_bg_run_for_wb,
        "EBG4MonoWB_",
        """Run number, workspace or symbolic presentation of such run
                  containing results of the empty instrument background measurements for detector Vanadium for Mono""",
    )

    # TODO: do something about it.  Second white is explicitly used in
    # diagnostics but not accessed at all
    second_white = RunDescriptor(
        """Second white beam run results currently unused in the workflow
                    despite being referred to in Diagnostics.
                    In a future it should be enabled."""
    )
    #
    _tmp_run = RunDescriptor("_TMP", "Property used for storing intermediate run data during reduction.")
    # -----------------------------------------------------------------------------------

    def getDefaultParameterValue(self, par_name):
        """method to get default parameter value, specified in IDF"""
        return prop_helpers.get_default_parameter(self.instrument, par_name)

    @property
    def instrument(self):
        if self._pInstrument is None:
            raise KeyError("Attempt to use uninitialized property manager")
        else:
            return self._pInstrument

    #
    # -----------------------------------------------------------------------------------
    # TODO: do something about it

    @property
    def print_diag_results(self):
        """property-sink used in diagnostics"""
        return True

    @print_diag_results.setter
    def print_diag_results(self, value):
        pass

    # -----------------------------------------------------------------------------------
    # -----------------------------------------------------------------------------

    @property
    def cashe_sum_ws(self):
        """Used together with sum_runs property. If True, a workspace
        with partial sum is stored in ADS
        and used later to add more runs to it
        """
        return self._cashe_sum_ws

    @cashe_sum_ws.setter
    def cashe_sum_ws(self, val):
        # pylint: disable=attribute-defined-outside-init
        self._cashe_sum_ws = bool(val)

    # -----------------------------------------------------------------------------

    @property
    def log_to_mantid(self):
        """Property specify if high level log should be printed to stdout or added to common Mantid log"""
        return self._log_to_mantid

    @log_to_mantid.setter
    def log_to_mantid(self, val):
        object.__setattr__(self, "_log_to_mantid", bool(val))

    # -----------------------------------------------------------------------------

    @property
    def mapmask_ref_ws(self):
        """Property provides reference workspace for LoadMask and GroupWorkspace algorithms

        on 26/07/2016 reference workspace (a workspace which provides appropriate
        spectra-detector mapping was implemented for LoadMask algorithm only.
        """
        return self._mapmask_ref_ws

    @mapmask_ref_ws.setter
    def mapmask_ref_ws(self, val):
        object.__setattr__(self, "_mapmask_ref_ws", val)

    # -----------------------------------------------------------------------------
    @property
    def archive_upload_log_file(self):
        """The full path to the file, containing log describing the last run number,
        uploaded to archive on ISIS. Only after a file has been added to the archive,
        the data are available for reduction. The information is used by reduction
        script, which runs during experiment and waits until data files redy for reduction.
        """
        if self._archive_upload_log_file is None:
            if len(NonIDF_Properties.archive_upload_log_template) > 0:
                trial_file = NonIDF_Properties.archive_upload_log_template.format(self.instr_name)
                self._set_archive_update_log(trial_file, False)
            else:
                object.__setattr__(self, "_archive_upload_log_file", "")

        return self._archive_upload_log_file

    @archive_upload_log_file.setter
    def archive_upload_log_file(self, filename):
        self._set_archive_update_log(filename)

    def _set_archive_update_log(self, filename, report_failure=True):
        if os.path.isfile(filename):
            object.__setattr__(self, "_archive_upload_log_file", filename)
        else:
            object.__setattr__(self, "_archive_upload_log_file", "")
            if report_failure:
                self.log("archive upload file log {0} does not exist. Ignoring it.".format(filename), "warning")

    # -----------------------------------------------------------------------------
    # Service properties (used by class itself)
    # -----------------------------------------------------------------------------

    def _set_instrument_and_facility(self, Instrument, run_workspace=None):
        """Obtain default instrument and facility and store it in properties"""

        if run_workspace:
            instrument = run_workspace.getInstrument()
            instr_name = instrument.getFullName()
            new_name, full_name, facility_ = prop_helpers.check_instrument_name(None, instr_name)
        else:
            # pylint: disable=protected-access
            if isinstance(Instrument, geometry.Instrument):
                instrument = Instrument
                instr_name = instrument.getFullName()
                try:
                    new_name, full_name, facility_ = prop_helpers.check_instrument_name(None, instr_name)
                except KeyError:  # the instrument pointer is not found in any facility but we have it after all
                    new_name = instr_name
                    full_name = instr_name
                    facility_ = config.getFacility("TEST_LIVE")
                # end

            elif isinstance(Instrument, str):  # instrument name defined
                new_name, full_name, facility_ = prop_helpers.check_instrument_name(None, Instrument)
                # idf_dir = config.getString('instrumentDefinitgeton.directory')
                idf_file = ExperimentInfo.getInstrumentFilename(full_name)
                tmp_ws_name = "__empty_" + full_name
                if not mtd.doesExist(tmp_ws_name):
                    LoadEmptyInstrument(Filename=idf_file, OutputWorkspace=tmp_ws_name)
                instrument = mtd[tmp_ws_name].getInstrument()
            else:
                raise TypeError(" neither correct instrument name nor instrument pointer provided as instrument parameter")
        # end if
        object.__setattr__(self, "_pInstrument", instrument)
        object.__setattr__(self, "_instr_name", full_name)
        object.__setattr__(self, "_facility", facility_)
        object.__setattr__(self, "_short_instr_name", new_name)


if __name__ == "__main__":
    pass
