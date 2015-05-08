import os
import platform
import shutil
import re
from datetime import date

# the list of instruments this configuration is applicable to
INELASTIC_INSTRUMENTS = ['MAPS','LET','MERLIN','MARI','HET']

class UserProperties(object):
    """Helper class to define & retrieve user properties
       as retrieved from file provided by user office
    """
    def __init__(self):
        self.instrument={}
        self.rb_dir = {}
        self.cycle_IDlist={}
        self.start_dates = {}
        self._recent_dateID=None

    def set_user_properties(self,instrument,start_date,cycle,rb_folder):
        """Define the information, user office provides about user. The info has the form:
           instrument -- string with full instrument name
           date       -- experiment start date in the form YYYYMMDD
           cycle      -- the cycle id in the form CYCLEYYYYN where N is the cycle number within the year
           rb_folder  -- the working folder available for all users and IS participating in the experiment.
        """
        self.check_input(instrument,start_date,cycle,rb_folder)
        #when user starts
        recent_date = date(int(start_date[0:4]),int(start_date[4:6]),int(start_date[6:8]))
        recent_date_id = str(recent_date)
        self.start_dates[recent_date_id]=recent_date

        # a data which define the cycle ID e.g 2014_3 or something
        self.cycle_IDlist[recent_date_id] = (str(cycle[5:9]),str(cycle[9:10]))
        self.instrument[recent_date_id]   = str(instrument).upper()
        self.rb_dir[recent_date_id]       = rb_folder
        if self._recent_dateID:
            for a_date in self.start_dates:
                if recent_date > a_date:
                    self._recent_dateID = recent_date_id
        else:
            self._recent_dateID = recent_date_id

    def check_input(self,instrument,start_date,cycle,rb_folder):
        """Verify that input is correct"""
        if not instrument in INELASTIC_INSTRUMENTS:
            raise RuntimeError("Instrument {0} has to be one of ISIS inelastic instruments".format(instrument))
        if not (isinstance(start_date,str) and len(start_date) == 8):
            raise RuntimeError("Experiment start date {0} should be defined as a sting in the form YYYYMMDD but it is not".format(start_date))
        if not (isinstance(cycle,str) and len(cycle) == 10 and re.match('^CYCLE',cycle)) :
            raise RuntimeError("Cycle {0} should have form CYCLEYYYYN where N-- the cycle's number in a year but it is not".format(cycle))
        if not (os.path.exists(rb_folder) and os.path.isdir(rb_folder)):
            raise RuntimeError("Folder {0} have to exist".format(rb_folder))

class MantidConfigDirectInelastic(object):
    """Class describes Mantid server specific user's configuration,
        necessary for Direct Inelastic reduction and analysis to work

        The class should not depend on Mantid itself.

        Valid for Mantid 3.4 available on 12/05/2015 and expects server
        to have: 
        Map/masks folder with layout defined on (e.g. svn checkout)
        https://svn.isis.rl.ac.uk/InstrumentFiles/trunk
        User scripts folder with layout defined on 
        (e.g. git checkout or Mantid script repository set-up):
        git@github.com:mantidproject/scriptrepository.git
        see https://github.com/mantidproject/scriptrepository for details

       The class have to change/to be amended if the configuration 
       changes or has additional features.
    """
    def __init__(self,mantid,home,script_repo,map_mask_folder):
        """Initialize generic config variables and variables specific to a server"""

        self._mantid_path = str(mantid)
        self._home_path  = str(home)
        self._script_repo = str(script_repo)
        self._map_mask_folder = str(map_mask_folder)

        self._check_server_folders_present()

        #
        self._header = ("# This file can be used to override any properties for this installation.\n"
                        "# Any properties found in this file will override any that are found in the Mantid.Properties file\n"
                        "# As this file will not be replaced with futher installations of Mantid it is a safe place to put\n"
                        "# properties that suit your particular installation.\n"
                        "#\n"
                        "# See here for a list of possible options:''# http://www.mantidproject.org/Properties_File#Mantid.User.Properties''\n"
                        "#\n"
                        "#uncomment to enable archive search - ICat and Orbiter\n"
                        "#datasearch.searcharchive = On #  may be important for autoreduction to work,\n")
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
        #
        self._dynamic_options_base = ['default.facility=ISIS']
        # Path to python scripts, defined and used by mantid wrt to Mantid Root (this path may be version specific)
        self._python_mantid_path = ['scripts/Calibration/','scripts/Examples/','scripts/Interface/','scripts/Vates/']
        # Static paths to user scripts, defined wrt script repository root
        self._python_user_scripts = set(['direct_inelastic/ISIS/qtiGenie/'])
        # Methods, which build & verify various parts of Mantid configuration
        self._dynamic_options = [self._set_default_inst,
                        self._set_script_repo, # this would be necessary to have on an Instrument scientist account, disabled on generic setup
                        self._def_python_search_path,
                        self._set_datasearch_directory,self._set_rb_directory]
        self._instr_name=None
        self._cycle_folder=[]

    def is_inelastic(self,instr_name):
        """Check if the instrument is inelastic"""
        if instr_name in INELASTIC_INSTRUMENTS:
            return True
        else:
            return False
    #
    def init_user(self,fedid,user_prop):
        """Define settings, specific to a user"""
        #
        if not self.is_inelastic(user_prop.instr):
           raise RuntimeError('Instrument {0} is not among acceptable instruments'.format(instrument))
        self._instr_name=str(user_prop.instr)

        self._fedid = str(fedid)
        user_folder = os.path.join(self._home_path,self._fedid)
        if not os.path.exists(user_folder):
            raise RuntimeError("User with fedID {0} does not exist. Create such user folder first".format(fedid))
        if not os.path.exists(str(rb_folder)):
            raise RuntimeError("Experiment folder with {0} does not exist. Create such folder first".format(rb_folder))
        #
        self._rb_folder_dir = str(user_prop.rb_folder)
        # how to check cycle folders, they may not be available
        self._cycle_folder=[]
        for folder in user_prop.cycle_folders:
            self._cycle_folder.append(str(folder))
        # Initialize configuration settings 
        self._dynamic_options_val = copy.deepcopy(self._dynamic_options_base)
        self._init_config()
    #

    def  _check_server_folders_present(self):
        """Routine checks all necessary server folder are present"""
        if not os.path.exists(self._mantid_path):
            raise RuntimeError("SERVER ERROR: no correct mantid path defined at {0}".format(self._mantid_path))
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
        if self._instr_name:
            self._dynamic_options_val.append('default.instrument={0}'.format(self._instr_name))
        else:
            self._dynamic_options_val.append('default.instrument={0}'.format('MARI'))
    #
    def _set_script_repo(self):
        self._dynamic_options_val.append('#ScriptLocalRepository={0}'.format(self._script_repo))
    #
    def _def_python_search_path(self):
        """Define path for Mantid Inelastic python scripts"""
        # Note, instrument name script folder is currently upper case on GIT
        self._python_user_scripts.add(os.path.join('direct_inelastic/',str.upper(self._instr_name))+'/')

        path = os.path.join(self._mantid_path,'scripts/')
        for part in self._python_mantid_path:
            path +=';'+os.path.join(self._mantid_path,part)
        for part in self._python_user_scripts:
            path +=';'+os.path.join(self._script_repo,part)

        self._dynamic_options_val.append('pythonscripts.directories=' + path)
    #
    def _set_rb_directory(self):
       self._dynamic_options_val.append('defaultsave.directory={0}'.format(self._rb_folder_dir))
    #
    def _set_datasearch_directory(self):
        """Note, map/mask instrument folder is lower case as if loaded from SVN. 
           Autoreduction may have it upper case"""

        user_data_dir = os.path.abspath('{0}'.format(self._rb_folder_dir))
        map_mask_dir  = os.path.abspath(os.path.join('{0}'.format(self._map_mask_folder),'{0}'.format(str.lower(self._instr_name))))
        
        all_folders=self._cycle_folder
        data_dir = os.path.abspath('{0}'.format(all_folders[0]))
        for folders in all_folders[1:]:
             data_dir +=';'+os.path.abspath('{0}'.format(all_folders[0]))

        self._dynamic_options_val.append('datasearch.directories='+user_data_dir+';'+map_mask_dir+';'+data_dir)
    #
    def generate_config(self):
        """Save generated Mantid configuration file into user's home folder"""

        config_path = os.path.join(self._home_path,self._fedid,'.mantid')
        if not os.path.exists(config_path):
            err = os.mkdir(config_path)
            if err: 
                raise RuntimeError('can not find or create Mantid configuration path {0}'.format(config_path))
        config_file = os.path.join(config_path,'Mantid.user.properties')
        if os.path.exists(config_file):
            if platform.system() != 'Windows':
              os.system('chown -R '+self._fedid+':'+self._fedid+' '+config_path)
            return
        #
        fp = open(config_file,'w')
        fp.write(self._header)
        fp.write('## -----   Generated user properties ------------ \n')
        fp.write('##\n')
        for opt in self._dynamic_options_val:
            fp.write(opt)
            fp.write('\n##\n')
        fp.write(self._footer)
        fp.close()
        if platform.system() != 'Windows':
            os.system('chown -R '+self._fedid+':'+self._fedid+' '+config_path)

def copy_and_overwrite(from_path, to_path):
    if os.path.exists(to_path):
        shutil.rmtree(to_path)
    shutil.copytree(from_path, to_path)
