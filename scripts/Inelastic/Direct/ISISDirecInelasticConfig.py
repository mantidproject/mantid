#!/usr/bin/python
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import sys
import platform
import shutil
import re
import copy
from datetime import date
import time
from xml.dom import minidom

# the list of instruments this configuration is applicable to
INELASTIC_INSTRUMENTS = ["MAPS", "LET", "MERLIN", "MARI", "HET"]
# the list of the parameters, which can be replaced if found in user files
USER_PROPERTIES = ["instrument", "userID", "cycleID", "start_date", "rb_folder"]


class UserProperties(object):
    """Helper class to define & retrieve user properties
    as retrieved from file provided by user office
    """

    def __init__(self, *args):
        """Build user properties from space separated string in the form:
        "userId instr_name rb_num cycle_mu start_date"
        or list of five elements with the same meaning
        """
        self._instrument = {}
        self._rb_dirs = {}
        self._cycle_IDs = {}
        self._start_dates = {}
        self._rb_exist = {}

        self._user_id = None
        self._recent_dateID = None
        if args[0] is None:
            return
        if len(args) == 1:
            input_str = str(args[0])
            param = input_str.split()
            self._user_id = param[0]
            if len(param) == 5:
                self.set_user_properties(param[1], param[2], param[3], param[4])
            else:  # only userID was provided, nothing else is defined
                return
        elif len(args) == 5:
            self._user_id = str(args[0])
            self.set_user_properties(args[1], args[2], args[3], args[4])
        else:
            raise RuntimeError(
                "User has to be defined by the list of 5 components in the form:\n{0}".format(
                    "[userId,instr_name,rb_num,cycle_mu,start_date]"
                )
            )

    def __str__(self):
        """Convert class to string. Only last cycle settings are returned"""
        if self._user_id:
            return "{0} {1} {2} {3} {4}".format(self._user_id, self.instrument, self.rb_folder, self.cycleID, str(self.start_date))
        else:
            return "None"

            #

    def set_user_properties(self, instrument, rb_folder_or_id, cycle, start_date):
        """Define the information, user office provides about user.

        The info has the form:
        instrument -- string with full instrument name
        date       -- string with experiment start date in the form YYYYMMDD
        cycle      -- string with the cycle id in the form CYCLEYYYYN
                      where N is the cycle number within the year
        rb_folder  -- string containing the full path to working folder available
                     for all users and IS participating in the experiment.
        """
        instrument, start_date, cycle, rb_folder_or_id, rb_exist = self.check_input(instrument, start_date, cycle, rb_folder_or_id)
        # when user starts
        recent_date = date(int(start_date[0:4]), int(start_date[4:6]), int(start_date[6:8]))
        recent_date_id = str(recent_date)
        self._start_dates[recent_date_id] = recent_date
        self._rb_exist[recent_date_id] = rb_exist

        # a data which define the cycle ID e.g 2014_3 or something
        self._cycle_IDs[recent_date_id] = (str(cycle[5:9]), str(cycle[9:]))

        self._instrument[recent_date_id] = str(instrument).upper()
        self._rb_dirs[recent_date_id] = rb_folder_or_id
        if self._recent_dateID:
            max_date = self._start_dates[self._recent_dateID]
            for date_key, a_date in self._start_dates.items():
                if a_date > max_date:
                    self._recent_dateID = date_key
                    max_date = a_date
        else:
            self._recent_dateID = recent_date_id

    def replace_variables(self, data_string):
        """Replace variables defined in USER_PROPERTIES
        and enclosed in $ sign with their values
        defined for a user
        """
        str_parts = data_string.split("$")
        for prop in USER_PROPERTIES:
            try:
                ind = str_parts.index(prop)
            # pylint: disable=W0703
            except Exception:
                ind = None
            if ind is not None:
                str_parts[ind] = str(getattr(self, prop))
        data_string = "".join(str_parts)
        return data_string

    #
    @property
    def GID(self):
        """Returns user's group ID which coincide with
        number part of the rb directory
        """
        if self._user_id:
            RBfolder = os.path.basename(self.rb_dir)
            return RBfolder[2:]
        else:
            return None
            #

    @property
    def rb_folder(self):
        """Returns short name of user's RB folder
        consisting of string RB and string representation of
        RB number e.g. RB1510324
        """
        if self._user_id:
            RBfolder = os.path.basename(self.rb_dir)
            return RBfolder
        else:
            return None

    @property
    def rb_dir(self):
        """return rb folder used in last actual instrument"""
        if self._recent_dateID:
            return self._rb_dirs[self._recent_dateID]
        else:
            raise RuntimeError("User's experiment date is not defined. User undefined")

    @rb_dir.setter
    def rb_dir(self, user_home_path):
        """Set user's rb-folder path"""
        rb_path = self.rb_folder
        full_path = os.path.join(user_home_path, rb_path)
        if os.path.exists(full_path) and os.path.isdir(full_path):
            self._rb_dirs[self._recent_dateID] = full_path
            self._rb_exist[self._recent_dateID] = True
        else:
            pass

    def get_rb_num(self, exp_date):
        """Returns short name of user's RB folder
        consisting of string RB and string representation of
        RB number e.g. RB1510324,
        used on the date specified
        """
        return os.path.basename(self._rb_dirs[exp_date])

    #

    def get_rb_dir(self, exp_date):
        """Returns full name name of user's RB folder corresponding to the
        experiment, with the data provided.
        """
        return self._rb_dirs[exp_date]

    @property
    def rb_id(self):
        """the same as rb_folder:
        returns string with RB and string representation of
        RB number e.g. RB1510324
        """
        return self.rb_folder

    #
    @property
    def start_date(self):
        """Last start date"""
        if self._recent_dateID:
            return self._start_dates[self._recent_dateID]
        else:
            raise RuntimeError("User's experiment date is not defined. User undefined")
            #

    @property
    def instrument(self):
        """return instrument used in last actual experiment"""
        if self._recent_dateID:
            return self._instrument[self._recent_dateID]
        else:
            raise RuntimeError("User's experiment date is not defined. User undefined")
            #

    def get_instrument(self, cycle_date_id):
        """Return the instrument, used in the cycle with the date specified"""
        return self._instrument[cycle_date_id]

    #
    @property
    def rb_dir_exist(self):
        """return true if user's rb dir exist and false otherwise"""
        if self._recent_dateID:
            return self._rb_exist[self._recent_dateID]
        else:
            raise RuntimeError("User's experiment date is not defined. User undefined")

            #

    @property
    def cycleID(self):
        """return last cycleID the user is participating"""
        if self._recent_dateID:
            year, num = self._cycle_IDs[self._recent_dateID]
            return "{0}_{1}".format(year, num)
        else:
            raise RuntimeError("User's experiment date is not defined. User undefined")

    @property
    def cycle(self):
        """return last cycle the user is participating"""
        if self._recent_dateID:
            return self._cycle_IDs[self._recent_dateID]
        else:
            raise RuntimeError("User's experiment date is not defined. User undefined")
            #

    @property
    def userID(self):
        return self._user_id

    @userID.setter
    def userID(self, val):
        self._user_id = str(val)

    # number of branches as necessary
    # pylint: disable=R0912
    def check_input(self, instrument, start_date, cycle, rb_folder_or_id):
        """Verify that input is correct"""
        # Checks if instrument is inelastic and raises RuntimeError if not
        self.validate_instrument(instrument)

        # Checks if the date is valid and raises a RuntimeError if not
        start_date = self.validate_date(start_date)

        def convert_cycle_int(cycle_int):
            if cycle_int > 999:  # Full cycle format 20151
                cycle = "CYCLE{0:05}".format(cycle_int)
            else:
                cycle = "CYCLE20{0:03}".format(cycle_int)
            return cycle

        if isinstance(cycle, int):
            cycle = convert_cycle_int(cycle)
        if isinstance(cycle, str):
            if len(cycle) == 11:
                last_letter = cycle[-1]
                if last_letter.upper() not in {"A", "B", "C", "D", "E"}:
                    raise RuntimeError(
                        "Cycle should be a string in the form CYCLEYYYYN[A,B,C,D "
                        "N-- the cycle's number in a year or integer in the form: YYYYN or YYN "
                        "but it is {0}".format(cycle)
                    )
                else:
                    cycle = cycle.upper()
            elif len(cycle) < 10:
                cycle = cycle.replace("_", "")
                try:
                    cycle = int(cycle)
                except ValueError:
                    raise RuntimeError(
                        "Cycle should be a string in the form CYCLEYYYYN "
                        "N-- the cycle's number in a year or integer in the form: YYYYN or YYN "
                        "but it is {0}".format(cycle)
                    )
                cycle = convert_cycle_int(cycle)
                if not (len(cycle) == 10 and re.match("^CYCLE", cycle)):
                    raise RuntimeError(
                        "Cycle should be a string in form CYCLEYYYYN "
                        "N-- the cycle's number in a year or integer in the form: YYYYN or YYN "
                        "but it is {0}".format(cycle)
                    )
        if isinstance(rb_folder_or_id, int):
            rb_folder_or_id = "RB{0:07}".format(rb_folder_or_id)
        if not isinstance(rb_folder_or_id, str):
            raise RuntimeError("RB Folder {0} should be a string".format(rb_folder_or_id))
        else:
            f_path, rbf = os.path.split(rb_folder_or_id)
            if len(rbf) != 9:
                try:
                    rbf = int(rbf)
                    rbf = "RB{0:07}".format(rbf)
                    rb_folder_or_id = os.path.join(f_path, rbf)
                except ValueError:
                    raise RuntimeError("RB Folder {0} should be a string containing RB number at the end".format(rb_folder_or_id))
        # end
        if os.path.exists(rb_folder_or_id) and os.path.isdir(rb_folder_or_id):
            rb_exist = True
        else:
            rb_exist = False

        return instrument, start_date, cycle, rb_folder_or_id, rb_exist

    # -----------------------------------------------------------------------------------------------

    def validate_instrument(self, instrument):
        if instrument not in INELASTIC_INSTRUMENTS:
            raise RuntimeError("Instrument {0} has to be one of ISIS inelastic instruments".format(instrument))

    def validate_date(self, start_date):
        if isinstance(start_date, str):
            # the date of express -- let's make it long in the past
            if start_date.lower() == "none":
                start_date = "19800101"
                error = False
            else:
                start_date = start_date.replace("-", "")
                if len(start_date) != 8:
                    start_date = "20" + start_date
                if len(start_date) == 8:
                    error = False
                else:
                    error = True
        else:
            error = True
        if error:
            raise RuntimeError(
                "Experiment start date should be defined as a string in the form YYYYMMDD or YYMMDD but it is: {0}".format(start_date)
            )
        return start_date

    def get_all_instruments(self):
        """Return list of all instruments, user is working on during this cycle"""
        return list(self._instrument.values())

    def get_all_cycles(self):
        """Return list of all cycles the user participates in"""
        return list(self._instrument.keys())

    def get_all_rb(self):
        """Return list of all rb folders the user participates in"""
        return list(self._rb_dirs.values())


#
# --------------------------------------------------------------------#
#


class MantidConfigDirectInelastic(object):
    """Class describes Mantid server specific user's configuration,
     necessary for Direct Inelastic reduction and analysis to work

     The class should not depend on Mantid itself.

     1) Valid for Mantid 3.4 available on 18/05/2015 and expects server
     to have:
     Map/masks folder with layout defined on (e.g. svn checkout)
     https://svn.isis.rl.ac.uk/InstrumentFileFinder/trunk
     2) User scripts folder with layout defined on
     (e.g. git checkout or Mantid script repository set-up):
     git@github.com:mantidproject/scriptrepository.git
     see https://github.com/mantidproject/scriptrepository for details
     3) The data can be found in archive, mounted at /archive/NDXxxxxx/Instrument/data/cycle_XX_Y

     4)There are number of other assumptions about script path, used scripts, Mantid confg,
       and other folders
       All these assumptions are summarized within __init__

    The class have to change/to be amended if the configuration
    changes or has additional features.
    """

    # pylint: disable=too-many-instance-attributes
    # It has as many as parameters describing ISIS configuration.

    def __init__(
        self,
        mantid="/opt/Mantid/",
        home_dir="/home/",
        script_repo="/opt/UserScripts/",
        map_mask_folder="/usr/local/mprogs/InstrumentFileFinder/",
    ):
        """Initialize generic config variables and variables specific to a server"""

        self._mantid_path = str(mantid)
        self._home_path = str(home_dir)
        self._script_repo = str(script_repo)
        self._map_mask_folder = str(map_mask_folder)
        # check if all necessary server folders specified as class parameters are present
        self._check_server_folders_present()
        #
        # Static Parts of dynamic contents of Mantid configuration file
        self._root_data_folder = "/archive"  # root folder for all experimental results -- particular one will depend on
        # instrument and cycle number.
        # the common part of all strings, generated dynamically as function of input class parameters.
        self._dynamic_options_base = ["default.facility=ISIS"]
        # Path to python scripts, defined and used by Mantid wrt to Mantid Root (this path may be version specific)
        self._python_mantid_path = ["scripts/Calibration/", "scripts/Examples/", "scripts/Interface/"]
        # Static paths to user scripts, defined wrt script repository root
        self._python_user_scripts = set(["direct_inelastic/ISIS/qtiGenie/"])
        # Relative to a particular user path to place links, important to user
        self._user_specific_link_path = "Desktop"
        # Relative to a particular user name of folders with link to instrument files
        self._map_mask_link_name = "instrument_files"
        # the name of the file, which describes python files to copy to user. The file has to be placed in
        #  script_repository/instrument_name folder
        # File name, used as source of reduction scripts for particular instrument
        self._user_files_descr = "USER_Files_description.xml"
        # fall back files defined to use if USER_Files_description is for some reason not available or wrong
        # pylint: disable=W0108
        # it will not work without lambda as intended
        self._sample_reduction_file = lambda InstrName: "{0}Reduction_Sample.py".format(InstrName)
        # File name, used as target for copying to user folder for user to deploy as the base for his reduction script
        # it will not work without lambda as intended
        self._target_reduction_file = lambda InstrName, cycleID: "{0}Reduction_{1}.py".format(InstrName, cycleID)

        # Static contents of the Mantid Config file
        self._header = (
            "# This file can be used to override any properties for this installation.\n"
            "# Any properties found in this file will override any that are found in the Mantid.Properties file\n"
            "# As this file will not be replaced with further installations of Mantid it is a safe place to put\n"
            "# properties that suit your particular installation.\n"
            "#\n"
            "# See here for a list of possible options:''"
            "# http://www.mantidproject.org/Properties_File#Mantid.User.Properties''\n"
            "#\n"
            "#uncomment to enable archive search - ICat and Orbiter\n"
            "datasearch.searcharchive = On #  may be important for autoreduction to work,\n"
        )
        #
        self._footer = (
            "##\n"
            "## LOGGING\n"
            "##\n"
            "\n"
            "## Uncomment to change logging level\n"
            "## Default is notice\n"
            "## Valid values are: error, warning, notice, information, debug\n"
            "#logging.loggers.root.level=information\n"
            "\n"
            "## MantidWorkbench\n"
            "##\n"
            "## Show invisible workspaces\n"
            "#MantidOptions.InvisibleWorkspaces=0\n"
            "## Re-use plot instances for different plot types\n"
            "#MantidOptions.ReusePlotInstances=Off\n\n"
            "## Uncomment to disable use of OpenGL to render unwrapped instrument views\n"
            "#MantidOptions.InstrumentView.UseOpenGL=Off\n"
        )

        # Methods, which build & verify various parts of Mantid configuration
        self._dynamic_options = [
            self._set_default_inst,
            self._set_script_repo,
            # necessary to have on an Instrument scientist account, disabled on generic setup
            self._def_python_search_path,
            self._set_datasearch_directory,
            self._set_rb_directory,
        ]
        self._user = None
        self._cycle_data_folder = set()
        # this is the list, containing configuration strings
        # generated by the class. No configuration is present initially.
        # Its contents is generated by _init_config method from server and user specific
        # input parameters together.
        self._dynamic_configuration = None
        # Unconditionally rewrite Mantid Configuration
        self._force_change_config = False
        # Unconditionally rewrite copy of sample reduction script
        self._force_change_script = False

    #
    def config_need_replacing(self, config_file_name):
        """Method specifies conditions when existing configuration file should be replaced"""
        if self._force_change_config:
            return True
        # missing file should always be replaced
        if not os.path.isfile(config_file_name):
            return True

        start_date = self._user.start_date
        unmodified_creation_time = time.mktime(start_date.timetuple())
        targ_config_time = os.path.getmtime(config_file_name)

        # Only rewrite configuration if nobody have touched it
        if unmodified_creation_time == targ_config_time:
            return True
        else:
            return False
            #

    #

    def get_user_file_description(self, instr_name=None):
        """returbs full file name (with path) for an xml file which describes
        files, which should be copied to a user.

        If instrument name is known or provided, function
        calculates this name wrt. the location of the file in the Mantid user
        script repository.
        """
        if self._user:
            if not instr_name:
                instr_name = self._user.instrument
            return os.path.join(self._script_repo, "direct_inelastic", instr_name, self._user_files_descr)
        else:
            return self._user_files_descr
            #

    def script_need_replacing(self, target_script_name):
        """Method specifies conditions when existing reduction file should be replaced
        by a sample file.
        """
        if self._force_change_script:
            return True
        # non-existing file should always be replaced
        if not os.path.isfile(target_script_name):
            return True
        # Always replace sample file if it has not been touched
        start_date = self._user.start_date
        # this time is set up to the file, copied from the repository
        unmodified_file_time = time.mktime(start_date.timetuple())
        targ_file_time = os.path.getmtime(target_script_name)
        if unmodified_file_time == targ_file_time:
            return True
        else:  # somebody have modified the target file. Leave it alone
            return False
            #

    def _fullpath_to_copy(self, short_source_file=None, short_target_file=None, cycle_id=None):
        """Append full path to source and target files"""

        if cycle_id:
            InstrName = self._user.get_instrument(cycle_id)
            rb_folder = self._user.get_rb_dir(cycle_id)
        else:
            InstrName = self._user.instrument
            rb_folder = self._user.rb_dir
        if short_source_file is None:
            short_source_file = self._sample_reduction_file(InstrName)
        if short_target_file is None:
            CycleID = self._user.cycleID
            short_target_file = self._target_reduction_file(InstrName, CycleID)

        source_path = os.path.join(self._script_repo, "direct_inelastic", InstrName.upper())
        full_source = os.path.join(source_path, short_source_file)

        full_target = os.path.join(rb_folder, short_target_file)
        return full_source, full_target

    #
    def copy_reduction_sample(self, user_file_description=None, cycle_id=None, rb_group=None):
        """copy sample reduction scripts from Mantid script repository
        to user folder.
        """
        if user_file_description is None:
            user_file_description = self.get_user_file_description()
        if rb_group is None:
            rb_group = self._user.userID

        info_to_copy = self._parse_user_files_description(user_file_description, cycle_id)
        for source_file, dest_file, subst_list in info_to_copy:
            self._copy_user_file_job(source_file, dest_file, rb_group, subst_list)

    def _copy_and_parse_user_file(self, input_file, output_file, replacemets_list):
        """Method processes file provided for user and replaces list of keywords, describing user
        and experiment (See comments in User_files_description.xml) with their values
        """
        fh_targ = open(output_file, "w")
        if not fh_targ:
            return
        var_to_replace = list(replacemets_list.keys())
        with open(input_file) as fh_source:
            for line in fh_source:
                rez = line
                for var in var_to_replace:
                    if var in rez:
                        rez = rez.replace(var, replacemets_list[var])
                fh_targ.write(rez)
        fh_targ.close()

    #
    def _copy_user_file_job(self, input_file, output_file, rb_group, replacement_list=None):
        """Method copies file provided into the requested destination
        and replaces keys specified in replacement list dictionary with their
        values if replacement_list is provided.
        """
        if not os.path.isfile(input_file):
            return

        # already have target file or modified by user
        if not self.script_need_replacing(output_file):
            return
        if os.path.isfile(output_file):
            os.remove(output_file)
        if replacement_list is None:
            shutil.copyfile(input_file, output_file)
        else:
            self._copy_and_parse_user_file(input_file, output_file, replacement_list)
        os.chmod(output_file, 0o777)

        ownership_str = "chown {0}:{1} {2}".format(self._user.userID, rb_group, output_file)
        if platform.system() != "Windows":
            os.system(ownership_str)
        # Set up the file creation and modification dates to the users start date
        start_date = self._user.start_date
        file_time = time.mktime(start_date.timetuple())
        os.utime(output_file, (file_time, file_time))

    def _get_file_attributes(self, file_node, cycle=None):
        """processes xml file_node to retrieve file attributes to copy"""

        source_file = file_node.getAttribute("file_name")
        if source_file is None:
            return (None, None)
        target_file = file_node.getAttribute("copy_as")

        if target_file is None:
            source_file = target_file
        else:
            if "$" in target_file:
                target_file = self._user.replace_variables(target_file)
        full_source, full_target = self._fullpath_to_copy(source_file, target_file, cycle)

        return (full_source, full_target)

    #
    def _parse_replacement_info(self, repl_info):
        """process dom element 'replacement' and
        returns the  variables with its correspondent value
        to replace variable by their value.

        If value contains one or more of the supported variables as its part, this
        variable is replaced by its value.
        Supported variables are defined by global list USER_PROPERTIES
        and their values are taken from current self._user class
        """
        # what should be replaced in the file
        source = repl_info.getAttribute("var")
        if len(source) == 0:
            raise ValueError(
                '"replace" field of {0} file for instrument {1} has to contain attribute "var" and its value'.format(
                    self._user_files_descr, self._user.instrument
                )
            )
        # what should be placed instead of the replacement
        dest = repl_info.getAttribute("by_var")
        if len(dest) == 0:
            raise ValueError(
                '"replace" field of {0} file for instrument {1} has to contain attribute "by_var" and its value'.format(
                    self._user_files_descr, self._user.instrument
                )
            )

        # replace use-specific variables by their values
        if "$" in dest:
            dest = self._user.replace_variables(dest)
        return (source, dest)

    def _parse_user_files_description(self, job_description_file, cycle_id=None):
        """Method parses xml file used to describe files to provide to user"""

        # mainly for debugging purposes
        filenames_to_copy = []

        # does not work if user is not defined
        if self._user is None:
            return None
        # parse job description file, fail down on default behaviour if
        # user files description is not there
        try:
            domObj = minidom.parse(job_description_file)
        # have no idea what minidom specific exception is:
        # pylint: disable=W0703
        except Exception:
            input_file, output_file = self._fullpath_to_copy(None, None, cycle_id)
            filenames_to_copy.append((input_file, output_file, None))
            return filenames_to_copy

        files_to_copy = domObj.getElementsByTagName("file_to_copy")

        # go through all files in the description and define file copying operations
        for file_node in files_to_copy:
            # retrieve file attributes or its default values if the attributes are missing
            input_file, output_file = self._get_file_attributes(file_node, cycle_id)
            if input_file is None:
                continue

            # identify all replacements, defined for this file
            replacements_info = file_node.getElementsByTagName("replace")
            if len(replacements_info) == 0:
                replacement_list = None
            else:
                replacement_list = {}
                for replacement in replacements_info:
                    source, dest = self._parse_replacement_info(replacement)
                    replacement_list[source] = dest
            filenames_to_copy.append((input_file, output_file, replacement_list))

        return filenames_to_copy

    #
    def get_data_folder_name(self, instr, cycle_ID):
        """Method to generate a data folder from instrument name and the cycle start date
        (cycle ID)
        The agreement on the naming as currently in ISIS:
        e.g: /archive/NDXMERLIN/Instrument/data/cycle_08_1

         Note: will fail if cycle numbers ever become a 2-digit numbers e.g. cycle_22_10
        """
        # cycle folder have short form without leading numbers
        cycle_fold_n = int(cycle_ID[0]) - 2000
        folder = os.path.join(
            self._root_data_folder, "NDX" + instr.upper(), "Instrument/data/cycle_{0:02}_{1}".format(cycle_fold_n, str(cycle_ID[1][0]))
        )
        return folder

    def is_inelastic(self, instr_name):
        """Check if the instrument is inelastic"""
        if instr_name in INELASTIC_INSTRUMENTS:
            return True
        else:
            return False

    #
    def init_user(self, fedIDorUser, theUser=None):
        """Define settings, specific to a user
        Supports two interfaces -- old and the new one
        where
        OldInterface: requested two input parameters
        fedID   -- users federal id
        theUser -- class defining all other user property
        NewInterface: requested single parameter:
        theUser -- class defining all user's properties including fedID
        """
        if not theUser:
            if isinstance(fedIDorUser, UserProperties):
                theUser = fedIDorUser
            else:
                raise RuntimeError("self.init_user(val) has to have val of UserProperty type only and got")
        else:
            theUser.userID = fedIDorUser

        # check if all users instruments are inelastic instruments. (script works for inelastic only)
        users_instruments = theUser.get_all_instruments()
        for instr in users_instruments:
            if not self.is_inelastic(instr):
                raise RuntimeError("Instrument {0} is not among acceptable instruments".format(instr))
        self._user = theUser

        # pylint: disable=W0201
        # its init method so the change is reasonable
        self._fedid = theUser.userID
        user_folder = os.path.join(self._home_path, self._fedid)
        if not os.path.exists(user_folder):
            raise RuntimeError("User with fedID {0} does not exist. Create such user folder first".format(self._fedid))
        # get RB folders for all experiments user participates in.
        all_rbf = theUser.get_all_rb()
        for rb_folder in all_rbf:
            if not os.path.exists(str(rb_folder)):
                raise RuntimeError("Experiment folder with {0} does not exist. Create such folder first".format(rb_folder))
        #
        # how to check cycle folders, they may not be available
        self._cycle_data_folder = set()
        # pylint: disable=W0212
        for date_key, folder_id in list(theUser._cycle_IDs.items()):
            self._cycle_data_folder.add(self.get_data_folder_name(theUser._instrument[date_key], folder_id))
        # Initialize configuration settings
        self._dynamic_configuration = copy.deepcopy(self._dynamic_options_base)
        self._init_config()

    #
    def _check_server_folders_present(self):
        """Routine checks all necessary server folder are present"""
        if not os.path.exists(self._mantid_path):
            raise RuntimeError("SERVER ERROR: no correct Mantid path defined at {0}".format(self._mantid_path))
        if not os.path.exists(self._home_path):
            raise RuntimeError("SERVER ERROR: no correct home path defined at {0}".format(self._home_path))
        if not os.path.exists(self._script_repo):
            raise RuntimeError(
                (
                    "SERVER ERROR: no correct user script repository defined at {0}\n"
                    "Check out Mantid script repository from account, "
                    "which have admin rights"
                ).format(self._script_repo)
            )
        if not os.path.exists(self._map_mask_folder):
            raise RuntimeError(
                (
                    "SERVER ERROR: no correct map/mask folder defined at {0}\n"
                    "Check out Mantid map/mask files from svn at"
                    " https://svn.isis.rl.ac.uk/InstrumentFileFinder/trunk"
                ).format(self._map_mask_folder)
            )

    def _init_config(self):
        """Execute Mantid properties setup methods"""
        for fun in self._dynamic_options:
            fun()

    #
    def _set_default_inst(self):
        """Set up last instrument, deployed by user"""
        if self._user:
            InstrName = self._user.instrument
            self._dynamic_configuration.append("default.instrument={0}".format(InstrName))
        else:
            self._dynamic_configuration.append("default.instrument={0}".format("MARI"))

    #
    def _set_script_repo(self):
        """defines script repository location. By default its option is commented"""
        self._dynamic_configuration.append("#ScriptLocalRepository={0}".format(self._script_repo))

    #
    def _def_python_search_path(self):
        """Define path for Mantid Inelastic python scripts"""
        # Note, instrument name script folder is currently upper case on GIT
        if not self._user:
            raise RuntimeError("Can not define python search path without defined user")

        # define main Mantid scripts search path
        path = os.path.join(self._mantid_path, "scripts/")
        for part in self._python_mantid_path:
            path += ";" + os.path.join(self._mantid_path, part)

        # define and append user scripts search path
        user_path_part = copy.deepcopy(self._python_user_scripts)
        # pylint: disable=W0212
        for instr in self._user._instrument.values():
            user_path_part.add(os.path.join("direct_inelastic", instr.upper()))
        for part in user_path_part:
            path += ";" + os.path.join(self._script_repo, part) + "/"

        self._dynamic_configuration.append("pythonscripts.directories=" + path)

    #
    def _set_rb_directory(self):
        """Set up default save directory, the one where data are saved by default"""
        if self._user:
            rb_folder = self._user.rb_dir

            self._dynamic_configuration.append("defaultsave.directory={0}".format(rb_folder))
        else:
            raise RuntimeError("Can not define RB folder without user being defined")

    #
    def _set_datasearch_directory(self):
        """Note, map/mask instrument folder is lower case as if loaded from SVN.
        Autoreduction may have it upper case"""
        if not self._user:
            raise RuntimeError("Can not define Data search path without user being defined")

        instr_name = self._user.instrument
        map_mask_dir = os.path.abspath(os.path.join("{0}".format(self._map_mask_folder), "{0}".format(str.lower(instr_name))))
        # set up all data folders
        all_data_folders = list(self._cycle_data_folder)
        data_dir = os.path.abspath("{0}".format(all_data_folders[0]))
        for folder in all_data_folders[1:]:
            data_dir += ";" + os.path.abspath("{0}".format(folder))

        all_rb_folders = self._user.get_all_rb()
        for folder in all_rb_folders:
            data_dir += ";" + os.path.abspath("{0}".format(folder))

        self._dynamic_configuration.append("datasearch.directories=" + map_mask_dir + ";" + data_dir)

    #
    def generate_config(self, key_users_list=None):
        """Save generated Mantid configuration file into user's home folder
        and copy other files, necessary for Mantid to work properly
        """
        user_path = os.path.join(self._home_path, self._fedid)
        config_path = os.path.join(user_path, ".mantid")
        if not os.path.exists(config_path):
            os.makedirs(config_path)

        config_file = os.path.join(config_path, "Mantid.user.properties")
        if self.config_need_replacing(config_file):
            self._write_user_config_file(config_file)
        else:
            pass
        if platform.system() != "Windows":
            os.system("chown -R {0}:{0} {1}".format(self._fedid, config_path))
        self.make_map_mask_links(user_path)

        users_cycles = self._user.get_all_cycles()
        users_rb = self._user.get_all_rb()
        # extract rb folder without path, which gives RB group name
        users_rb = list(map(os.path.basename, users_rb))
        #
        for cycle, rb_name in zip(users_cycles, users_rb):
            if key_users_list:
                key_user = str(key_users_list[rb_name])
                if self._fedid.lower() != key_user.lower():
                    continue

            instr = self._user.get_instrument(cycle)
            self.copy_reduction_sample(self.get_user_file_description(instr), cycle, rb_name)
            #

    #
    def make_map_mask_links(self, user_path):
        """The method generates references to map files and places these references
        to the user's desktop.
        """
        # the path where to set up links, important to user
        links_path = os.path.join(user_path, self._user_specific_link_path)
        if not os.path.exists(links_path):
            os.makedirs(links_path)
            # the path have to belong to user
            if platform.system() != "Windows":
                os.system("chown -R {0}:{0} {1}".format(self._fedid, links_path))

        map_mask_folder_link = os.path.join(links_path, self._map_mask_link_name)
        if os.path.exists(map_mask_folder_link):
            return
        # create link to map mask folder
        if platform.system() == "Windows":
            # the script is not intended to run on Windows, so this is just for testing
            mmfl = map_mask_folder_link.replace("/", "\\")
            mmf = self._map_mask_folder.replace("/", "\\")
            os.system("mklink /J {0} {1}".format(mmfl, mmf))
        else:
            os.system("ln -s {0} {1}".format(self._map_mask_folder, map_mask_folder_link))

    def _write_user_config_file(self, config_file_name):
        """Write existing dynamic configuration from memory to
        user defined configuration file
        """
        # pylint: disable=C0103
        # What is wrong with fp variable name here?
        fp = open(config_file_name, "w")
        fp.write(self._header)
        fp.write("## -----   Generated user properties ------------ \n")
        fp.write("##\n")
        for opt in self._dynamic_configuration:
            fp.write(opt)
            fp.write("\n##\n")
        fp.write(self._footer)
        fp.close()
        if platform.system() != "Windows":
            os.system("chown -R {0}:{0} {1}".format(self._fedid, config_file_name))
        # Set up configuration for the specific time, which should change only if user
        # modified this configuration
        start_date = self._user.start_date
        file_time = time.mktime(start_date.timetuple())
        os.utime(config_file_name, (file_time, file_time))


# pylint: disable = invalid-name

if __name__ == "__main__":
    if len(sys.argv) != 6:
        print("usage: Config.py userID instrument RBNumber cycleID start_date")
        exit()

    argi = sys.argv[1:]
    user = UserProperties(*argi)

    if platform.system() == "Windows":
        sys.path.insert(0, "c:/Mantid/scripts/Inelastic/Direct")

        base = "d:/Data/Mantid_Testing/config_script_test_folder"
        analysisDir = base

        MantidDir = r"c:\Mantid\_builds\br_master\bin\Release"
        UserScriptRepoDir = os.path.join(analysisDir, "UserScripts")
        MapMaskDir = os.path.join(analysisDir, "InstrumentFileFinder")

        rootDir = os.path.join(base, "users")
    else:
        sys.path.insert(0, "/opt/Mantid/scripts/Inelastic/Direct/")
        # sys.path.insert(0,'/opt/mantidnightly/scripts/Inelastic/Direct/')

        MantidDir = "/opt/Mantid"
        MapMaskDir = "/usr/local/mprogs/InstrumentFileFinder/"
        UserScriptRepoDir = "/opt/UserScripts"
        home = "/home"
        #
        rootDir = "/home/"
        analysisDir = "/instrument/"

    # initialize Mantid configuration
    # its testing route under main so it rightly imports itself
    # pylint: disable=W0406
    mcf = MantidConfigDirectInelastic(MantidDir, rootDir, UserScriptRepoDir, MapMaskDir)
    print("Successfully initialized ISIS Inelastic Configuration script generator")

    rb_user_folder = os.path.join(mcf._home_path, user.userID)
    user.rb_dir = rb_user_folder
    if not user.rb_dir_exist:
        print("RB folder {0} for user {1} should exist and be accessible to configure this user".format(user.rb_dir, user.userID))
        exit()
    # Configure user
    mcf.init_user(user.userID, user)
    mcf.generate_config()
    print("Successfully Configured user: {0} for instrument {1} and RBNum: {2}".format(user.userID, user.instrument, user.rb_folder))
