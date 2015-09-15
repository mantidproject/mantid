import os
import platform
import shutil
import re
import copy
from datetime import date
import time
from xml.dom import minidom

# the list of instruments this configuration is applicable to
INELASTIC_INSTRUMENTS = ['MAPS','LET','MERLIN','MARI','HET']
# the list of the parameters, which can be replaced if found in user files
USER_PROPERTIES=['instrument', 'cycleID', 'start_date', 'rb_folder']

class UserProperties(object):
    """Helper class to define & retrieve user properties
       as retrieved from file provided by user office
    """
    def __init__(self,user_id=None):
        """ None is done for com"""
        self._user_id = str(user_id)
        self._instrument={}
        self._rb_dirs = {}
        self._cycle_IDs={}
        self._start_dates = {}
        self._recent_dateID=None

#
    def set_user_properties(self,instrument,start_date,cycle,rb_folder):
        """Define the information, user office provides about user.

           The info has the form:
           instrument -- string with full instrument name
           date       -- string with experiment start date in the form YYYYMMDD
           cycle      -- string with the cycle id in the form CYCLEYYYYN
                         where N is the cycle number within the year
           rb_folder  -- string containing the full path to working folder available
                        for all users and IS participating in the experiment.
        """
        self.check_input(instrument,start_date,cycle,rb_folder)
        #when user starts
        recent_date = date(int(start_date[0:4]),int(start_date[4:6]),int(start_date[6:8]))
        recent_date_id = str(recent_date)
        self._start_dates[recent_date_id]=recent_date

        # a data which define the cycle ID e.g 2014_3 or something
        self._cycle_IDs[recent_date_id] = (str(cycle[5:9]),str(cycle[9:10]))
        self._instrument[recent_date_id]   = str(instrument).upper()
        self._rb_dirs[recent_date_id]       = rb_folder
        if self._recent_dateID:
            max_date = self._start_dates[self._recent_dateID]
            for date_key,a_date in self._start_dates.iteritems():
                if a_date>max_date:
                    self._recent_dateID = date_key
                    max_date = a_date
        else:
            self._recent_dateID = recent_date_id

    def replace_variables(self,data_string):
        """Replace variables defined in USER_PROPERTIES
            and enclosed in $ sign with their values
		   defined for a user
		"""
        str_parts = data_string.split('$')
        for prop in USER_PROPERTIES:
            try:
                ind = str_parts.index(prop)
            except Exception:
                ind = None
            if not ind is None:
                str_parts[ind] = str(getattr(self,prop))
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
    def start_date(self):
        """Last start date"""
        if self._recent_dateID:
            return  self._start_dates[self._recent_dateID]
        else:
            raise RuntimeError("User's experiment date is not defined. User undefined")
 #
    @property
    def instrument(self):
        """return instrument used in last actual experiment"""
        if self._recent_dateID:
            return  self._instrument[self._recent_dateID]
        else:
            raise RuntimeError("User's experiment date is not defined. User undefined")
#
    @property
    def rb_dir(self):
        """return rb folder used in last actual instrument"""
        if self._recent_dateID:
            return  self._rb_dirs[self._recent_dateID]
        else:
            raise RuntimeError("User's experiment date is not defined. User undefined")
#
    @property
    def cycleID(self):
        """return last cycleID the user is participating"""
        if self._recent_dateID:
            year,num =self._cycle_IDs[self._recent_dateID]
            return  "{0}_{1}".format(year,num)
        else:
            raise RuntimeError("User's experiment date is not defined. User undefined")
    @property
    def cycle(self):
        """return last cycle the user is participating"""
        if self._recent_dateID:
            return  self._cycle_IDs[self._recent_dateID]
        else:
            raise RuntimeError("User's experiment date is not defined. User undefined")
#
    @property
    def userID(self):
        return self._user_id
    @userID.setter
    def userID(self,val):
        self._user_id = str(val)

#
    def check_input(self,instrument,start_date,cycle,rb_folder):
        """Verify that input is correct"""
        if not instrument in INELASTIC_INSTRUMENTS:
            raise RuntimeError("Instrument {0} has to be one of "\
                  "ISIS inelastic instruments".format(instrument))
        if not (isinstance(start_date,str) and len(start_date) == 8):
            raise RuntimeError("Experiment start date {0} should be defined as"\
                  " a sting in the form YYYYMMDD but it is not".format(start_date))
        if not (isinstance(cycle,str) and len(cycle) == 10 and re.match('^CYCLE',cycle)) :
            raise RuntimeError("Cycle {0} should have form CYCLEYYYYN where "\
                  "N-- the cycle's number in a year but it is not".format(cycle))
        if not (os.path.exists(rb_folder) and os.path.isdir(rb_folder)):
            raise RuntimeError("Folder {0} have to exist".format(rb_folder))
#
#--------------------------------------------------------------------#
#
class MantidConfigDirectInelastic(object):
    """Class describes Mantid server specific user's configuration,
        necessary for Direct Inelastic reduction and analysis to work

        The class should not depend on Mantid itself.

        1) Valid for Mantid 3.4 available on 18/05/2015 and expects server
        to have:
        Map/masks folder with layout defined on (e.g. svn checkout)
        https://svn.isis.rl.ac.uk/InstrumentFiles/trunk
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
    def __init__(self,mantid='/opt/Mantid/',home='/home/',\
                 script_repo='/opt/UserScripts/',\
                 map_mask_folder='/usr/local/mprogs/InstrumentFiles/'):
        """Initialize generic config variables and variables specific to a server"""

        self._mantid_path = str(mantid)
        self._home_path  = str(home)
        self._script_repo = str(script_repo)
        self._map_mask_folder = str(map_mask_folder)
        # check if all necessary server folders specified as class parameters are present
        self._check_server_folders_present()
        #
        # Static Parts of dynamic contents of Mantid configuration file
        self._root_data_folder='/archive' # root folder for all experimental results -- particular one will depend on
                                          # instrument and cycle number.
        # the common part of all strings, generated dynamically as function of input class parameters.
        self._dynamic_options_base = ['default.facility=ISIS']
        # Path to python scripts, defined and used by mantid wrt to Mantid Root (this path may be version specific)
        self._python_mantid_path = ['scripts/Calibration/','scripts/Examples/','scripts/Interface/','scripts/Vates/']
        # Static paths to user scripts, defined wrt script repository root
        self._python_user_scripts = set(['direct_inelastic/ISIS/qtiGenie/'])
        # Relative to a particular user path to place links, important to user
        self._user_specific_link_path='Desktop'
        # Relative to a particular user name of folders with link to instrument files
        self._map_mask_link_name = 'instrument_files'
        # the name of the file, which describes python files to copy to user. The file has to be placed in
        #  script_repository/instrument_name folder
        # File name, used as source of reduction scripts for particular instrument
        self._user_files_descr = 'USER_Files_description.xml'
        # fall back files defined to use if USER_Files_description is for some reason not available or wrong
        # pylint: disable=W0108
        # it will not work without lambda as intended
        self._sample_reduction_file = lambda InstrName : '{0}Reduction_Sample.py'.format(InstrName)
        # File name, used as target for copying to user folder for user to deploy as the base for his reduction script
        # it will not work without lambda as intended
        self._target_reduction_file = lambda InstrName,cycleID : '{0}Reduction_{1}.py'.format(InstrName,cycleID)


        # Static contents of the Mantid Config file
        self._header = ("# This file can be used to override any properties for this installation.\n"
                        "# Any properties found in this file will override any that are found in the Mantid.Properties file\n"
                        "# As this file will not be replaced with further installations of Mantid it is a safe place to put\n"
                        "# properties that suit your particular installation.\n"
                        "#\n"
                        "# See here for a list of possible options:''"
                        "# http://www.mantidproject.org/Properties_File#Mantid.User.Properties''\n"
                        "#\n"
                        "#uncomment to enable archive search - ICat and Orbiter\n"
                        "datasearch.searcharchive = On #  may be important for autoreduction to work,\n")
        #
        self._footer = ("##\n"
                        "## LOGGING\n"
                        "##\n"
                        "\n"
                        "## Uncomment to change logging level\n"
                        "## Default is information\n"
                        "## Valid values are: error, warning, notice, information, debug\n"
                        "#logging.loggers.root.level=information\n"
                        "\n"
                        "## Sets the lowest level messages to be logged to file\n"
                        "## Default is warning\n"
                        "## Valid values are: error, warning, notice, information, debug\n"
                        "#logging.channels.fileFilterChannel.level=debug\n"
                        "## Sets the file to write logs to\n"
                        "#logging.channels.fileChannel.path=../mantid.log\n"
                        "##\n"
                        "## MantidPlot\n"
                        "##\n"
                        "## Show invisible workspaces\n"
                        "#MantidOptions.InvisibleWorkspaces=0\n"
                        "## Re-use plot instances for different plot types\n"
                        "#MantidOptions.ReusePlotInstances=Off\n\n"
                        "## Uncomment to disable use of OpenGL to render unwrapped instrument views\n"
                        "#MantidOptions.InstrumentView.UseOpenGL=Off\n")

        # Methods, which build & verify various parts of Mantid configuration
        self._dynamic_options = [self._set_default_inst,
                                 self._set_script_repo, # necessary to have on an Instrument scientist account, disabled on generic setup
                                 self._def_python_search_path,
                                 self._set_datasearch_directory,self._set_rb_directory]
        self._user = None
        self._cycle_data_folder=set()
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
    def config_need_replacing(self,config_file_name):
        """Method specifies conditions when existing configuration file should be replaced"""
        if self._force_change_config:
            return True
        # missing file should always be replaced
        if not os.path.isfile(config_file_name):
            return True
        modification_date = date.fromtimestamp(os.path.getmtime(config_file_name))
        start_date = self._user.start_date
        if modification_date<start_date:
            return True
        else:
            return False
#
    @property
    def user_file_description(self):
        """defines full file name (with path) for an xml file which describes
		   files, which should be copied to a user
		"""
        if self._user:
            return os.path.join(self._script_repo,'direct_inelastic',self._user.instrument,
                                self._user_files_descr)
        else:
            return  self._user_files_descr
#
    def script_need_replacing(self,target_script_name):
        """Method specifies conditions when existing reduction file should be replaced
           by a sample file.
        """
        if self._force_change_script:
            return True
        # non-existing file should always be replaced
        if not os.path.isfile(target_script_name):
            return True
        #Always replace sample file if it has not been touched
        start_date = self._user.start_date
        # this time is set up to the file, copied from the repository
        sample_file_time = time.mktime(start_date.timetuple())
        targ_file_time  = os.path.getmtime(target_script_name)
        if sample_file_time  ==  targ_file_time:
            return True
        else: # somebody have modified the target file. Leave it alone
            return False
#
    def _fullpath_to_copy(self,short_source_file=None,short_target_file=None):
        """Append full path to source and target files """

        InstrName = self._user.instrument
        rb_folder = self._user.rb_dir
        if short_source_file is None:
            short_source_file = self._sample_reduction_file(InstrName)
        if short_target_file is None:
            CycleID = self._user.cycleID
            short_target_file = self._target_reduction_file(InstrName,CycleID)


        source_path = os.path.join(self._script_repo,'direct_inelastic',InstrName.upper())
        full_source = os.path.join(source_path,short_source_file)

        full_target = os.path.join(rb_folder,short_target_file)
        return full_source,full_target
#
    def copy_reduction_sample(self, users_file_description=None):
        """copy sample reduction scripts from Mantid script repository
           to user folder.
        """
        if users_file_description is None:
            users_file_description = self._user_files_descr
        files_to_copy = self._parse_user_files_description(users_file_description)
        for files in files_to_copy:
            self._copy_user_file_job(files[0],files[1],files[2])


    def _copy_and_parse_user_file(self,input_file,output_file,replacemets_list):
        """Method processes file provided for user and replaces list of keywords, describing user
           and experiment (See comments in User_files_description.xml) with their values
        """
        fh_targ    = open(output_file,'w')
        if not fh_targ :
            return
        var_to_replace = replacemets_list.keys()
        with open(input_file) as fh_source:
            for line in fh_source:
                rez = line
                for var in var_to_replace:
                    if var in rez:
                        rez = rez.replace(var, replacemets_list[var])
                fh_targ.write(rez)
        fh_targ.close()
#
    def _copy_user_file_job(self,input_file,output_file,replacement_list=None):
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
            shutil.copyfile(input_file,output_file)
        else:
            self._copy_and_parse_user_file(input_file,output_file,replacement_list)
        os.chmod(output_file,0777)

        if platform.system() != 'Windows':
            os.system("chown {0}:{0} {1}".format(self._fedid,output_file))
        # Set up the file creation and modification dates to the users start date
        start_date = self._user.start_date
        file_time = time.mktime(start_date.timetuple())
        os.utime(output_file,(file_time,file_time))

    def _get_file_attributes(self,file_node):
        """processes xml file_node to retrieve file attributes to copy """

        source_file = file_node.getAttribute("file_name")
        if source_file is None:
            return (None,None)
        target_file = file_node.getAttribute("copy_as")

        if target_file is None:
            source_file = target_file
        else:
            if "$" in target_file:
                target_file = self._user.replace_variables(target_file)
        full_source,full_target  = self._fullpath_to_copy(source_file,target_file)

        return (full_source,full_target)
#
    def _parse_replacement_info(self,repl_info):
        """process dom element 'replacement' and
           returns the  variables with its correspondent value
           to replace variable by their value.

           If value contains one or more of the supported variables as its part, this
           variable is replaced by its value.
           Supported variables are defined by global list USER_PROPERTIES
           and their values are taken from current self._user class
        """
        # what should be replaced in the file
        source  = repl_info.getAttribute("var")
        if len(source) == 0:
            raise InvalidArgument('"replace" field of {0} file for instrument {1} has to contain attribute "var" and its value'\
                                   .format(self._user_files_descr,self._user.instrument))
        # what should be placed instead of the replacement
        dest    = repl_info.getAttribute("by_var")
        if len(dest) == 0:
            raise InvalidArgument('"replace" field of {0} file for instrument {1} has to contain attribute "by_var" and its value'\
                                   .format(self._user_files_descr,self._user.instrument))

        # replace use-specific variables by their values
        if '$' in dest:
            dest = self._user.replace_variables(dest)
        return (source,dest)

    def _parse_user_files_description(self,job_description_file):
        """ Method parses xml file used to describe files to provide to user"""

        # mainly for debugging purposes
        filenames_to_copy=[]

        # does not work if user is not defined
        if self._user is None:
            return None
        # parse job description file, fail down on default behavior if
        # user files description is not there
        try:
            domObj=minidom.parse(job_description_file)
        except Exception:
            input_file,output_file = self._fullpath_to_copy()
            filenames_to_copy.append((input_file,output_file,None))
            return filenames_to_copy

        files_to_copy = domObj.getElementsByTagName("file_to_copy")

        # go through all files in the description and define file copying operations
        for file_node in files_to_copy :
            # retrieve file attributes or its default values if the attributes are missing
            input_file,output_file =self._get_file_attributes(file_node)
            if input_file is None:
                continue

            # identify all replacements, defined for this file
            replacements_info = file_node.getElementsByTagName('replace')
            if len(replacements_info) == 0:
                replacement_list = None
            else:
                replacement_list = {}
                for replacement in replacements_info:
                    source,dest = self._parse_replacement_info(replacement)
                    replacement_list[source]=dest
            filenames_to_copy.append((input_file,output_file,replacement_list))

        return filenames_to_copy
#
    def get_data_folder_name(self,instr,cycle_ID):
        """Method to generate a data folder from instrument name and the cycle start date
           (cycle ID)
           The agreement on the naming as currently in ISIS:
           e.g: /archive/NDXMERLIN/Instrument/data/cycle_08_1
        """
        # cycle folder have short form without leading numbers
        cycle_fold_n =int(cycle_ID[0])-2000
        folder = os.path.join(self._root_data_folder,'NDX'+instr.upper(),\
                              "Instrument/data/cycle_{0:02}_{1}".format(cycle_fold_n,str(cycle_ID[1])))
        return folder

    def is_inelastic(self,instr_name):
        """Check if the instrument is inelastic"""
        if instr_name in INELASTIC_INSTRUMENTS:
            return True
        else:
            return False
    #
    def init_user(self,fedIDorUser,theUser=None):
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
            if isinstance(fedIDorUser,UserProperties):
                theUser = fedIDorUser
            else:
                raise RuntimeError("self.init_user(val) has to have val of UserProperty type only")
        else:
            theUser.userID = fedIDorUser
        #
        # pylint: disable=W0212
        # bad practice but I need all instruments, not the current one
        for instr in theUser._instrument.values():
            if not self.is_inelastic(instr):
                raise RuntimeError('Instrument {0} is not among acceptable instruments'.format(instrument))
        self._user=theUser

        # pylint: disable=W0201
        # its init method so the change is reasonable
        self._fedid =  theUser.userID
        user_folder = os.path.join(self._home_path,self._fedid)
        if not os.path.exists(user_folder):
            raise RuntimeError("User with fedID {0} does not exist. Create such user folder first".format(self._fedid))
        # pylint: disable=W0212
        # bad practice but I need all rb_dires, not the current one
        for rb_folder in theUser._rb_dirs.values():
            if not os.path.exists(str(rb_folder)):
                raise RuntimeError("Experiment folder with {0} does not exist. Create such folder first".format(rb_folder))
        #
        # how to check cycle folders, they may not be available
        self._cycle_data_folder=set()
        # pylint: disable=W0212
        for date_key,folder_id in theUser._cycle_IDs.items():
            self._cycle_data_folder.add(self.get_data_folder_name(theUser._instrument[date_key],folder_id))
        # Initialize configuration settings
        self._dynamic_configuration = copy.deepcopy(self._dynamic_options_base)
        self._init_config()
    #
    def  _check_server_folders_present(self):
        """Routine checks all necessary server folder are present"""
        if not os.path.exists(self._mantid_path):
            raise RuntimeError("SERVER ERROR: no correct Mantid path defined at {0}".format(self._mantid_path))
        if not os.path.exists(self._home_path):
            raise RuntimeError("SERVER ERROR: no correct home path defined at {0}".format(self._home_path))
        if not os.path.exists(self._script_repo):
            raise RuntimeError(("SERVER ERROR: no correct user script repository defined at {0}\n"
                                "Check out Mantid script repository from account, which have admin rights").format(self._script_repo))
        if not os.path.exists(self._map_mask_folder):
            raise RuntimeError(("SERVER ERROR: no correct map/mask folder defined at {0}\n"
                                "Check out Mantid map/mask files from svn at https://svn.isis.rl.ac.uk/InstrumentFiles/trunk")\
                                .format(self._map_mask_folder))

    def _init_config(self):
        """Execute Mantid properties setup methods"""
        for fun in self._dynamic_options:
            fun()
    #
    def _set_default_inst(self):
        """Set up last instrument, deployed by user"""
        if self._user:
            InstrName = self._user.instrument
            self._dynamic_configuration.append('default.instrument={0}'.format(InstrName))
        else:
            self._dynamic_configuration.append('default.instrument={0}'.format('MARI'))
    #
    def _set_script_repo(self):
        """ defines script repository location. By default its option is commented"""
        self._dynamic_configuration.append('#ScriptLocalRepository={0}'.format(self._script_repo))
    #
    def _def_python_search_path(self):
        """Define path for Mantid Inelastic python scripts"""
        # Note, instrument name script folder is currently upper case on GIT
        if not self._user:
            raise RuntimeError("Can not define python search path without defined user")

        # define main Mantid scripts search path
        path = os.path.join(self._mantid_path,'scripts/')
        for part in self._python_mantid_path:
            path +=';'+os.path.join(self._mantid_path,part)

        # define and append user scripts search path
        user_path_part = copy.deepcopy(self._python_user_scripts)
        # pylint: disable=W0212
        for instr in self._user._instrument.values():
            user_path_part.add(os.path.join('direct_inelastic',instr.upper()))
        for part in user_path_part:
            path +=';'+os.path.join(self._script_repo,part)+'/'

        self._dynamic_configuration.append('pythonscripts.directories=' + path)
    #
    def _set_rb_directory(self):
        """Set up default save directory, the one where data are saved by default"""
        if self._user:
            rb_folder = self._user.rb_dir

            self._dynamic_configuration.append('defaultsave.directory={0}'.format(rb_folder))
        else:
            raise RuntimeError("Can not define RB folder without user being defined")
    #
    def _set_datasearch_directory(self):
        """Note, map/mask instrument folder is lower case as if loaded from SVN.
           Autoreduction may have it upper case"""
        if not self._user:
            raise RuntimeError("Can not define Data search path without user being defined")

        instr_name = self._user.instrument
        map_mask_dir  = os.path.abspath(os.path.join('{0}'.format(self._map_mask_folder),\
                                                     '{0}'.format(str.lower(instr_name))))
        # set up all data folders
        all_data_folders=list(self._cycle_data_folder)
        data_dir = os.path.abspath('{0}'.format(all_data_folders[0]))
        for folder in all_data_folders[1:]:
            data_dir +=';'+os.path.abspath('{0}'.format(folder))

        # pylint: disable=W0212
        all_rb_folders = self._user._rb_dirs
        for folder in all_rb_folders.values():
            data_dir+=';'+os.path.abspath('{0}'.format(folder))

        self._dynamic_configuration.append('datasearch.directories='+map_mask_dir+';'+data_dir)
    #
    def generate_config(self):
        """Save generated Mantid configuration file into user's home folder
           and copy other files, necessary for Mantid to work properly
        """
        user_path = os.path.join(self._home_path,self._fedid)
        config_path = os.path.join(user_path,'.mantid')
        if not os.path.exists(config_path):
            err = os.makedirs(config_path)
            if err:
                raise RuntimeError('can not find or create Mantid configuration path {0}'.format(config_path))

        config_file = os.path.join(config_path,'Mantid.user.properties')
        if self.config_need_replacing(config_file):
            self._write_user_config_file(config_file)
        else:
            pass
        if platform.system() != 'Windows':
            os.system('chown -R {0}:{0} {1}'.format(self._fedid,config_path))

        self.copy_reduction_sample(self.user_file_description)
        #
        self.make_map_mask_links(user_path)
    #
    def make_map_mask_links(self,user_path):
        """The method generates references to map files and places these references
           to the user's desktop.
        """
        # the path where to set up links, important to user
        links_path = os.path.join(user_path,self._user_specific_link_path)
        if not os.path.exists(links_path):
            os.makedirs(links_path)
            # the path have to belong to user
            if platform.system() != 'Windows':
                os.system('chown -R {0}:{0} {1}'.format(self._fedid,links_path))

        map_mask_folder_link = os.path.join(links_path,self._map_mask_link_name)
        if os.path.exists(map_mask_folder_link):
            return
        # create link to map mask folder
        if platform.system() == 'Windows':
            # the script is not intended to run on Windows, so this is just for testing
            mmfl = map_mask_folder_link.replace('/','\\')
            mmf  = self._map_mask_folder.replace('/','\\')
            os.system("mklink /J {0} {1}".format(mmfl,mmf))
        else:
            os.system('ln -s {0} {1}'.format(self._map_mask_folder,map_mask_folder_link))

    def _write_user_config_file(self,config_file_name):
        """Write existing dynamic configuration from memory to
           user defined configuration file
        """
        # pylint: disable=C0103
        # What is wrong with fp variable name here?
        fp = open(config_file_name,'w')
        fp.write(self._header)
        fp.write('## -----   Generated user properties ------------ \n')
        fp.write('##\n')
        for opt in self._dynamic_configuration:
            fp.write(opt)
            fp.write('\n##\n')
        fp.write(self._footer)
        fp.close()
        if platform.system() != 'Windows':
            os.system('chown -R {0}:{0} {1}'.format(self._fedid,config_file_name))


