# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from mantid.simpleapi import *
from mantid import config, api
from mantid.kernel import funcinspect

from Direct.PropertyManager import PropertyManager

# this import is used by children
from Direct.DirectEnergyConversion import DirectEnergyConversion
from types import MethodType  # noqa
import os
import re
import time

try:
    import h5py

    h5py_installed = True
except ImportError:
    h5py_installed = False

from abc import abstractmethod


# R0921 abstract class not referenced -- wrong, client references it.
# pylint: disable=too-many-instance-attributes, R0921


class ReductionWrapper(object):
    """Abstract class provides interface to direct inelastic reduction
    allowing it to be run  from Mantid, web services, or system tests
    using the same interface and the same run file placed in different
    locations.
    """

    # pylint: disable=too-few-public-methods
    class var_holder(object):
        """A simple wrapper class to keep web variables"""

        def __init__(self, Web_vars=None):
            if Web_vars:
                self.standard_vars = Web_vars.standard_vars
                self.advanced_vars = Web_vars.advanced_vars
            else:
                self.standard_vars = None
                self.advanced_vars = None

        #

        def get_all_vars(self):
            """Return dictionary with all defined variables
            combined together
            """
            web_vars = {}
            if self.advanced_vars:
                web_vars = self.advanced_vars.copy()
            if self.standard_vars:
                if len(web_vars) > 0:
                    web_vars.update(self.standard_vars)
                else:
                    web_vars = self.standard_vars.copy()
            return web_vars

    def __init__(self, instrumentName, web_var=None):
        """sets properties defaults for the instrument with Name
        and define if wrapper runs from web services or not
        """
        # internal variable, indicating if we should try to wait for input files to appear
        self._wait_for_file = False
        # The property defines the run number, to validate. If defined, switches reduction wrapper from
        # reduction to validation mode
        self._run_number_to_validate = None
        # internal variable, used in system tests to validate workflow,
        # with waiting for files.  It is the holder to the function
        # used during debugging "wait for files" workflow
        # instead of Pause algorithm
        self._debug_wait_for_files_operation = None
        # tolerance to change in some tests if default is not working well
        self._tolerr = None

        # The variables which are set up from web interface or to be exported to
        # web interface
        if web_var:
            self._run_from_web = True
        else:
            self._run_from_web = False
        self._wvs = ReductionWrapper.var_holder(web_var)
        # Initialize reduced for given instrument
        self.reducer = DirectEnergyConversion(instrumentName)
        #
        web_vars = self._wvs.get_all_vars()
        if web_vars:
            self.reducer.prop_man.set_input_parameters(**web_vars)
        # if run on ISIS, information about the log files, responsible for
        # storing data in archive
        self._last_commit_log_modification_time = None
        self._last_runnum_added_to_archive = 0

    @property
    def wait_for_file(self):
        """If this variable set to positive value, this value
        is interpreted as time to wait until check for specified run file
        if this file have not been find immediately.

        if this variable is 0 or false and the file have not been found,
        reduction will fail
        """
        return self._wait_for_file

    @wait_for_file.setter
    def wait_for_file(self, value):
        if value > 0:
            self._wait_for_file = value
        else:
            self._wait_for_file = False

    #

    def save_web_variables(self, FileName=None):
        """Method to write simple and advanced properties and help
        information  into dictionary, to use by web reduction
        interface

        If no file is provided, reduce_var.py file will be written
        to the folder, containing current script

        """
        if not FileName:
            FileName = "reduce_vars.py"

        f = open(FileName, "w")
        f.write("standard_vars = {\n")
        str_wrapper = "         "
        for key, val in self._wvs.standard_vars.items():
            if isinstance(val, str):
                row = "{0}'{1}':'{2}'".format(str_wrapper, key, val)
            else:
                row = "{0}'{1}':{2}".format(str_wrapper, key, val)
            f.write(row)
            str_wrapper = ",\n         "
        f.write("\n}\nadvanced_vars={\n")
        # print advances variables
        str_wrapper = "         "
        for key, val in self._wvs.advanced_vars.items():
            if isinstance(val, str):
                row = "{0}'{1}':'{2}'".format(str_wrapper, key, val)
            else:
                row = "{0}'{1}':{2}".format(str_wrapper, key, val)
            f.write(row)
            str_wrapper = ",\n        "

        def write_help_block(fhandle, block_name, block_dict):
            str_wrapper = "         "
            row = "{0}'{1}' : {{\n".format(str_wrapper, block_name)
            fhandle.write(row)
            for key in block_dict:
                try:
                    prop = getattr(PropertyManager, key)
                    docstring = prop.__doc__
                    if not docstring:
                        continue
                # pylint: disable=bare-except
                except:
                    continue
                contents = self._do_format(docstring)
                row = "{0}'{1}':'{2}'".format(str_wrapper, key, contents)
                fhandle.write(row)
                str_wrapper = ",\n        "
            fhandle.write("{0} }},\n".format(str_wrapper))

        f.write("\n}\nvariable_help={\n")
        write_help_block(f, "standard_vars", self._wvs.standard_vars)
        write_help_block(f, "advanced_vars", self._wvs.advanced_vars)
        f.write("}\n")
        f.close()

    def _do_format(self, docstring):
        """Format docstring to write it as string in the reduce_var file"""
        contents = re.sub(" +", " ", docstring)
        contents = contents.split("\n")
        contents = "\\n".join(contents)
        return contents

    @property
    def validate_run_number(self):
        """The property defines the run number to validate. If defined, switches reduction wrapper from
        reduction to validation mode, where reduction tries to load result, previously calculated,
        for this run and then compare this result with the result, defined earlier"""
        return self._run_number_to_validate

    @validate_run_number.setter
    def validate_run_number(self, val):
        if val is None:
            self._run_number_to_validate = None
        else:
            self._run_number_to_validate = int(val)

    def validate_settings(self):
        """method validates initial parameters, provided for reduction"""
        self.def_advanced_properties()
        self.def_main_properties()
        if self._run_from_web:
            web_vars = self._wvs.get_all_vars()
            self.reducer.prop_man.set_input_parameters(**web_vars)
        else:
            pass  # we should already set up these variables using
            # def_main_properties & def_advanced_properties
        # validate properties and report result
        return self.reducer.prop_man.validate_properties(False)

    #

    def validation_file_name(self):
        """the name of the file, used as reference to
        validate the run, specified as the class property

        The method can be overloaded to return a workspace
        or workspace name to validate results against.
        """
        # pylint: disable=protected-access
        if PropertyManager.save_file_name._file_name is not None:
            # pylint: disable=protected-access
            file_name = PropertyManager.save_file_name._file_name
            if isinstance(file_name, api.Workspace):
                return file_name
        else:
            instr = self.reducer.prop_man.instr_name
            run_n = self.validate_run_number
            ei = PropertyManager.incident_energy.get_current()
            file_name = "{0}{1}_{2:<3.2f}meV_VALIDATION_file.nxs".format(instr, run_n, ei)
        run_dir = self.validation_file_place()
        full_name = os.path.join(run_dir, file_name)
        return full_name

    def validation_file_place(self):
        """Redefine this to the place, where validation file, used in conjunction with
        'validate_run' property, located. Here it defines the place to this script folder.
        By default it looks for/places it in a default save directory"""
        return config["defaultsave.directory"]

    #
    def validate_result(self, Error=1.0e-6, ToleranceRelErr=True):
        """Method to validate result against existing validation file
        or workspace

        Change this method to verify different results or validate results differently"""
        rez, message = ReductionWrapper.build_or_validate_result(self, Error, ToleranceRelErr)
        return rez, message

    #

    def set_custom_output_filename(self):
        """define custom name of output files if standard one is not satisfactory
        User expected to overload this method within class instantiation"""
        return None

    def evaluate_abs_corrections(self, test_ws, spectra_to_correct):
        """Evaluate absorption corrections from the input workspace
        Input:
        test_ws -- the workspace to calculate corrections for.
                   The corrections themselves should be defined by
                   the following data reduction properties:
                   propmen.correct_absorption_on = TheShapeOfTheSample -- define sample parameters
                   propmen.abs_corr_info   = {} Dictionary with additional correction parameters
                   (can be empty)
         spectra_to_correct -- list of the spectra to correct absorption for.
         If this list is empty, the corrections are calculated for the whole workspace,
         which can cause problems for plotting.

         Returns:
         corrections -- the workspace containing the absorption corrections
         for the spectra, specified in spectra_to_correct variable.
        """

        n_spectra = test_ws.getNumberHistograms()
        decrement = len(spectra_to_correct)
        if decrement > 0:
            red_ws = ExtractSpectra(test_ws, WorkspaceIndexList=spectra_to_correct)
        else:
            decrement = n_spectra

        prop_man = self.reducer.prop_man
        abs_shape = prop_man.correct_absorption_on
        start_time = time.time()
        ws, corrections = abs_shape.correct_absorption(red_ws, prop_man.abs_corr_info)
        end_time = time.time()
        estimated_time = (end_time - start_time) * n_spectra / decrement
        prop_man.log("**************************************************************************************************", "notice")
        prop_man.log(
            "*** Estimated time to run absorption corrections on the final workspace is: {0:.1f}sec".format(estimated_time), "notice"
        )
        prop_man.log("**************************************************************************************************", "notice")
        return (corrections, estimated_time)

    # pylint: disable=too-many-branches
    def build_or_validate_result(self, Error=1.0e-6, ToleranceRelErr=True):
        """Method validates results of the reduction against reference file or workspace.

        Inputs:
        sample_run     -- the run number to reduce or validate against existing result
        validation_file -- The name of nxs file, containing workspace, produced by reducing SampleRun,
                          or the pointer to the workspace, which is the reference workspace
                          for SampleRun reduction.

        Returns:
        True   if reduction for sample_run produces result within Error from the reference file
               as reported by CompareWorkspaces.
        False  if CompareWorkspaces comparison between sample and reduction is unsuccessful

        True  if was not able to load reference file. In this case, algorithm builds validation
              file and returns True if the reduction and saving of this file is successful

        """
        # this row defines location of the validation file
        validation_file = self.validation_file_name()
        sample_run = self.validate_run_number
        if isinstance(validation_file, str):
            path, name = os.path.split(validation_file)
            if name in mtd:
                reference_ws = mtd[name]
                build_validation = False
                fileName = "workspace:" + reference_ws.name()
            else:
                if len(path) > 0:
                    config.appendDataSearchDir(path)
                # it there bug in getFullPath? It returns the same string if given full path
                # but file has not been found
                # pylint: disable=unused-variable
                name, fext = os.path.splitext(name)
                fileName = FileFinder.getFullPath(name + ".nxs")
                if len(fileName) > 0:
                    build_validation = False
                    try:
                        reference_ws = Load(fileName)
                    # pylint: disable=bare-except
                    except:
                        build_validation = True
                else:
                    build_validation = True
        elif isinstance(validation_file, api.Workspace):
            # its workspace:
            reference_ws = validation_file
            build_validation = False
            fileName = "workspace:" + reference_ws.name()
        else:
            build_validation = True
        # --------------------------------------------------------
        if build_validation:
            self.reducer.prop_man.save_file_name = validation_file
            self.reducer.prop_man.log(
                "*** WARNING:can not find or load validation file {0}\n    Building validation file for run N:{1}".format(
                    validation_file, sample_run
                ),
                "warning",
            )
        else:
            self.reducer.prop_man.log(
                "*** FOUND VALIDATION FILE: {0}\n    Validating run {1} against this file".format(fileName, sample_run), "warning"
            )

        # just in case, to be sure
        current_web_state = self._run_from_web
        current_wait_state = self.wait_for_file
        # disable wait for input and
        self._run_from_web = False
        self.wait_for_file = False
        #
        self.def_advanced_properties()
        self.def_main_properties()
        #
        self.reducer.sample_run = sample_run
        self.reducer.prop_man.save_format = None

        reduced = self.reduce()

        if build_validation:
            self.reducer.prop_man.save_file_name = None
            result_name = os.path.splitext(validation_file)[0]
            self.reducer.prop_man.log("*** Saving validation file with name: {0}.nxs".format(result_name), "notice")
            SaveNexus(reduced, Filename=result_name + ".nxs")
            return True, "Created validation file {0}.nxs".format(result_name)
        else:
            if isinstance(reduced, list):  # check only first result in multirep
                reduced = reduced[0]
            # Cheat! Counterintuitive!
            if self._tolerr:
                TOLL = self._tolerr
            else:
                TOLL = Error
            result = CompareWorkspaces(
                Workspace1=reference_ws,
                Workspace2=reduced,
                Tolerance=TOLL,
                CheckSample=False,
                CheckInstrument=False,
                ToleranceRelErr=ToleranceRelErr,
            )

        self.wait_for_file = current_wait_state
        self._run_from_web = current_web_state
        if result[0]:
            return True, "Reference file and reduced workspace are equal with accuracy {0:<3.2f}".format(TOLL)
        else:
            fname, _ = os.path.splitext(fileName)
            filename = fname + "-mismatch.nxs"
            self.reducer.prop_man.log(
                "***WARNING: can not get results matching the reference file.\n   Saving new results to file {0}".format(filename),
                "warning",
            )
            SaveNexus(reduced, Filename=filename)
            return False, result

    @abstractmethod
    def def_main_properties(self):
        """Define properties which considered to be main properties changeable by user

        Should be overwritten by special reduction and decorated with  @MainProperties decorator.

        Should return dictionary with key are the properties names and values -- the default
        values these properties should have.
        """
        raise NotImplementedError("def_main_properties  has to be implemented")

    @abstractmethod
    def def_advanced_properties(self):
        """Define properties which considered to be advanced but still changeable by instrument scientist or advanced user

        Should be overwritten by special reduction and decorated with  @AdvancedProperties decorator.

        Should return dictionary with key are the properties names and values -- the default
        values these properties should have.
        """

        raise NotImplementedError("def_advanced_properties  has to be implemented")

    #

    def _run_pause(self, timeToWait=0):
        """a wrapper around pause algorithm allowing to run something
        instead of pause in debug mode
        """

        if self._debug_wait_for_files_operation is not None:
            # it is callable and the main point of this method is that it is callable
            # pylint: disable=E1102
            self._debug_wait_for_files_operation()
        else:
            Pause(timeToWait)

    #
    def _check_progress_log_run_completed(self, run_number_requested):
        """Method to verify experiment progress log file and check if the file to reduce
        has been written.
        Input:
         run_number_requested -- the number expected to be in logged in the log file

        Output:
          returns: (True,run_number_written,'') if the run_number stored in the log is
                  higher then the run number requested
                  (False,run_number_written,'') if the stored number is lower then the requested

        If progress log is nod defined or not available, the method returns True, last known run number
        and additional text information indicating the reason for failure
        so further checks are necessary to verify if actual file is indeed available
        """
        propman = self.reducer.prop_man
        if len(propman.archive_upload_log_file) == 0:
            return (True, 0, "log test disabled as no log file available")

        mod_time = os.path.getmtime(propman.archive_upload_log_file)
        if self._last_commit_log_modification_time == mod_time:  # Still old data in archive
            run_num = self._last_runnum_added_to_archive
            return (run_num >= run_number_requested, run_num, "no new data have been added to archive")
        self._last_commit_log_modification_time = mod_time
        # Here the file may be modified during the access. Let's try to catch
        # any errors, which may occur due to this modification
        try:
            with open(propman.archive_upload_log_file) as fh:
                contents = fh.read()
        except:
            return (False, self._last_runnum_added_to_archive, "Error accessing log file {0}".format(propman.archive_upload_log_file))
        # If the file is modified during the read operation, the read can return anything
        # Let's be on a safe side and guard the contents parsing too.
        try:
            contents = contents.split()
            run_written = int(contents[1])
        except:
            return (
                False,
                self._last_runnum_added_to_archive,
                "Error processing the contents of the log file {0}".format(propman.archive_upload_log_file),
            )

        self._last_runnum_added_to_archive = run_written
        return (run_written >= run_number_requested, run_written, "")

    #
    def _check_access_granted(self, input_file):
        """Check if the access to the found nxs file is granted

        Created to fix issue on ISIS archive, when file
        is copied through the network for ~2min and become available
        2 minutes after it has been found.
        """

        _, found_ext = os.path.splitext(input_file)
        if found_ext != ".nxs":  # problem solution for nxs files only. Others are seems ok
            return
        if not h5py_installed:  # well this check is not available. Sad, but it available on
            # all our working systems. Inform user about the problem
            self.reducer.prop_man.log(
                "*** Can not verify if file is accessible. Install h5py to be able to check file access in waiting mode", "notice"
            )
            return
        ic = 0
        # ok = os.access(input_file,os.R_OK) # does not work in this case
        try:
            f = h5py.File(input_file, "r")
            ok = True
        except IOError:
            ok = False
            while not ok:
                self.reducer.prop_man.log("*** File found but access can not be gained. Waiting for 10 sec", "notice")
                time.sleep(10)
                ic = ic + 1
                try:
                    f = h5py.File(input_file, "r")
                    ok = True
                except IOError:
                    ok = False
                    if ic > 24:
                        raise IOError("Can not get read access to input file: " + input_file + " after 4 min of trying")
        if ok:
            f.close()

    def reduce(self, input_file=None, output_directory=None):
        """The method performs all main reduction operations over
        single run file

        Wrap it into @iliad wrapper to switch input for
        reduction properties between script and web variables
        """
        if input_file:
            # attribute-defined-outside-init -- wrong, it is not
            # pylint: disable=W0201
            self.reducer.sample_run = str(input_file)
        if output_directory:
            config["defaultsave.directory"] = str(output_directory)

        timeToWait = self._wait_for_file
        if timeToWait > 0:
            _, fext_requested = PropertyManager.sample_run.file_hint()
            run_number_requsted = PropertyManager.sample_run.run_number()
            available, _, info = self._check_progress_log_run_completed(run_number_requsted)
            if len(info) > 0:  # report if archive upload log is not available
                self.reducer.prop_man.log("*** " + info, "warning")

            if available:
                Found, input_file = PropertyManager.sample_run.find_file(
                    self.reducer.prop_man, be_quet=True, force_extension=fext_requested
                )
            else:
                Found = False
            while not Found:
                file_hint, fext = PropertyManager.sample_run.file_hint()
                self.reducer.prop_man.log(
                    "*** Waiting {0} sec for file {1} to appear on the data search path".format(timeToWait, file_hint), "notice"
                )

                self._run_pause(timeToWait)
                available, _, _ = self._check_progress_log_run_completed(run_number_requsted)
                if available:
                    Found, input_file = PropertyManager.sample_run.find_file(
                        self.reducer.prop_man, file_hint=file_hint, be_quet=True, force_extension=fext_requested
                    )
                else:
                    Found = False
            # endWhile
            # found but let's give it some time to finish possible IO operations
            self._check_access_granted(input_file)
            #
            converted_to_energy_transfer_ws = self.reducer.convert_to_energy(None, input_file)

        else:
            converted_to_energy_transfer_ws = self.reducer.convert_to_energy(None, input_file)

        return converted_to_energy_transfer_ws

    #

    def sum_and_reduce(self):
        """procedure used to sum and reduce runs in case when not all files
        are available and user have to wait for these files to appear
        """
        # pylint: disable=protected-access
        if not PropertyManager.sample_run._run_list:
            raise RuntimeError("sum_and_reduce expects run file list to be defined")

        self.reducer.prop_man.sum_runs = True

        timeToWait = self._wait_for_file
        self._wait_for_file = 0
        if timeToWait > 0:
            run_files = PropertyManager.sample_run.get_run_list()
            num_files_to_sum = len(PropertyManager.sample_run)

            ok, missing, found = self.reducer.prop_man.find_files_to_sum()
            n_found = len(found)
            if not ok:
                # necessary to cache intermediate sums in memory
                self.reducer.prop_man.cashe_sum_ws = True
            while not ok:
                while n_found > 0:
                    last_found = found[-1]
                    self.reducer.prop_man.sample_run = last_found  # request to reduce all up to last found
                    # Note that here we run convert to energy instead of user (may be) reloaded reduction!
                    # This would cause problem for user-defined reduction, which pre-process rather than
                    # post-process resulting workspace
                    ws = self.reducer.convert_to_energy()
                    # reset search to whole file list again
                    self.reducer.prop_man.sample_run = run_files[num_files_to_sum - 1]
                    ok, missing, found = self.reducer.prop_man.find_files_to_sum()
                    n_found = len(found)
                    if ok:  # no need to cache sum any more.  All necessary files found
                        self.reducer.prop_man.cashe_sum_ws = False

                self.reducer.prop_man.log(
                    "*** Waiting {0} sec for runs {1} to appear on the data search path".format(timeToWait, str(missing)), "notice"
                )
                self._run_pause(timeToWait)
                ok, missing, found = self.reducer.prop_man.find_files_to_sum()
                n_found = len(found)
            # end not(ok)
            if n_found > 0:
                # cash sum can be dropped now if it has not been done before
                self.reducer.prop_man.cashe_sum_ws = False
                for run in found:
                    # here we have run numbers. Let's get real file names
                    prop_man = self.reducer.prop_man
                    instr_name = prop_man.short_instr_name
                    run_number_requsted = PropertyManager.sample_run.run_number()
                    available, _, _ = self._check_progress_log_run_completed(run_number_requsted)
                    if available:
                        is_found, fname = PropertyManager.sample_run.find_file(prop_man, instr_name, run)
                    else:
                        is_found = False
                    if not is_found:
                        raise RuntimeError("File has been found earlier but can not been retrieved now. Logical bug")
                    else:
                        # found but let's give it some time to finish possible IO operations
                        self._check_access_granted(fname)
                ws = self.reduce()
        else:
            ws = self.reduce()
        self._wait_for_file = timeToWait
        return ws

    #

    def run_reduction(self):
        """ " Reduces runs one by one or sum all them together and reduce after this

        if wait_for_file time is > 0, it will until  missing files appear on the
        data search path
        """
        try:
            _, r = funcinspect.lhs_info("both")
            out_ws_name = r[0]
        # no-exception-type(s) specified. Who knows what exception this internal procedure rises...
        # pylint: disable=W0702
        except:
            out_ws_name = None

        # if this is not None, we want to run validation not reduction
        if self.validate_run_number:
            self.reducer.prop_man.log("**************************************************************************************", "warning")
            self.reducer.prop_man.log("**************************************************************************************", "warning")
            rez, mess = self.build_or_validate_result()
            if rez:
                self.reducer.prop_man.log("*** SUCCESS! {0}".format(mess))
                self.reducer.prop_man.log(
                    "**************************************************************************************", "warning"
                )

            else:
                self.reducer.prop_man.log("*** VALIDATION FAILED! {0}".format(mess))
                self.reducer.prop_man.log(
                    "**************************************************************************************", "warning"
                )
                raise RuntimeError("Validation against old data file failed")
            self.validate_run_number = None
            return rez, mess

        if self.reducer.sum_runs:
            # --------### sum runs provided ------------------------------------###
            if out_ws_name is None:
                self.sum_and_reduce()
                return None
            else:
                red_ws = self.sum_and_reduce()
                RenameWorkspace(InputWorkspace=red_ws, OutputWorkspace=out_ws_name)
                return mtd[out_ws_name]
        else:
            # --------### reduce list of runs one by one ----------------------------###
            runfiles = PropertyManager.sample_run.get_run_file_list()
            if out_ws_name is None:
                for file_name in runfiles:
                    self.reduce(file_name)
                return None
            else:
                results = []
                nruns = len(runfiles)
                for num, file_name in enumerate(runfiles):
                    red_ws = self.reduce(file_name)
                    if isinstance(red_ws, list):
                        for ws in red_ws:
                            results.append(ws)
                    else:
                        if nruns == 1:
                            if red_ws.name() != out_ws_name:
                                RenameWorkspace(InputWorkspace=red_ws, OutputWorkspace=out_ws_name)
                            results.append(mtd[out_ws_name])
                        else:
                            OutWSName = "{0}#{1}of{2}".format(out_ws_name, num + 1, nruns)
                            if red_ws.name() != out_ws_name:
                                RenameWorkspace(InputWorkspace=red_ws, OutputWorkspace=OutWSName)
                            results.append(mtd[OutWSName])
                # end
                if len(results) == 1:
                    return results[0]
                else:
                    return results
                # end if
            # end if
        # end


def MainProperties(main_prop_definition):
    """Decorator stores properties dedicated as main and sets these properties
    as input to reduction parameters."""

    def main_prop_wrapper(*args):
        # execute decorated function
        prop_dict = main_prop_definition(*args)
        # print "in decorator: ",properties
        host = args[0]
        # pylint: disable=protected-access
        if not host._run_from_web:  # property run locally
            # pylint: disable=protected-access
            host._wvs.standard_vars = prop_dict
            host.reducer.prop_man.set_input_parameters(**prop_dict)
        return prop_dict

    return main_prop_wrapper


#


def AdvancedProperties(adv_prop_definition):
    """Decorator stores properties decided to be advanced and sets these properties
    as input for reduction parameters
    """

    def advanced_prop_wrapper(*args):
        prop_dict = adv_prop_definition(*args)
        # print "in decorator: ",properties
        host = args[0]
        # pylint: disable=protected-access
        if not host._run_from_web:  # property run locally
            # pylint: disable=protected-access
            host._wvs.advanced_vars = prop_dict
            host.reducer.prop_man.set_input_parameters(**prop_dict)
        return prop_dict

    return advanced_prop_wrapper


# pylint: disable=too-many-branches


def iliad(reduce):
    """This decorator wraps around main procedure and switch input from
    web variables to properties or vice versa depending on web variables
    presence
    """

    def iliad_wrapper(*args):
        # seq = inspect.stack()
        # output workspace name.
        try:
            name = funcinspect.lhs_info("names")
            out_ws_name = name[0]
        # no-exception-type(s) specified. Who knows what exception this internal procedure rises...
        # pylint: disable=W0702
        except:
            out_ws_name = None

        host = args[0]
        if len(args) > 1:
            input_file = args[1]
            if len(args) > 2:
                output_directory = args[2]
            else:
                output_directory = None
        else:
            input_file = None
            output_directory = None
        # add input file folder to data search directory if file has it
        if input_file and isinstance(input_file, str):
            data_path = os.path.dirname(input_file)
            if len(data_path) > 0:
                try:
                    config.appendDataSearchDir(str(data_path))
                    args[1] = os.path.basename(input_file)
                # pylint: disable=bare-except
                except:  # if mantid is not available, this should ignore config
                    pass
        if output_directory:
            config["defaultsave.directory"] = str(output_directory)

        # pylint: disable=protected-access
        if host._run_from_web:
            # pylint: disable=protected-access
            web_vars = host._wvs.get_all_vars()
            host.reducer.prop_man.set_input_parameters(**web_vars)
        else:
            pass  # we should set already set up variables using

        custom_print_function = host.set_custom_output_filename()
        if custom_print_function is not None:
            PropertyManager.save_file_name.set_custom_print(custom_print_function)
        #
        rez = reduce(*args)

        # prohibit returning workspace to web services.
        # pylint: disable=protected-access
        if host._run_from_web and not isinstance(rez, str):
            rez = ""
        else:
            if isinstance(rez, list):
                # multirep run, just return as it is
                return rez
            if rez is not None and out_ws_name and rez.name() != out_ws_name:
                # the function does not return None, pylint is wrong
                # pylint: disable=W1111
                rez = PropertyManager.sample_run.synchronize_ws(rez, out_ws_name)
        return rez

    return iliad_wrapper


def custom_operation(custom_fun):
    DirectEnergyConversion.__setattr__()

    def custom_fun_wrapper(*args):
        # execute decorated function
        ws = custom_fun(*args)
        # print "in decorator: ",properties
        # host = args[0]
        return ws

    return custom_fun_wrapper


if __name__ == "__main__":
    pass
