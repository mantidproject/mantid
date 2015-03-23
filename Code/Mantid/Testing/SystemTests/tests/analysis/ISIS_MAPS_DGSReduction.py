#pylint: disable=invalid-name
""" Sample MAPS reduction scrip """
import os
os.environ["PATH"] = r"c:\Mantid\Code\builds\br_master\bin\Release;" + os.environ["PATH"]
from Direct.ReductionWrapper import *
try:
    import reduce_vars as web_var
except:
    web_var = None


class ReduceMAPS(ReductionWrapper):
    @MainProperties
    def def_main_properties(self):
        """ Define main properties used in reduction """
        prop = {}
        prop['sample_run'] = 17269
        prop['wb_run'] = 17186
        prop['incident_energy'] = 150
        prop['energy_bins'] = [-15,3,135]

        # Absolute units reduction properties.
        prop['monovan_run'] = 17589
        prop['sample_mass'] = 10 / (94.4 / 13) # -- this number allows to get approximately the same system test intensities for MAPS as the old test
        prop['sample_rmm'] = 435.96 #
        return prop

    @AdvancedProperties
    def def_advanced_properties(self):
        """  separation between simple and advanced properties depends
           on scientist, experiment and user.
           main properties override advanced properties.
        """
        prop = {}
        prop['map_file'] = 'default'
        #prop['monovan_mapfile'] = 'default' #'4to1_mid_lowang.map' # default
        prop['hard_mask_file'] = None
        #prop['det_cal_file'] = ?  default?
        prop['save_format'] = ''

        prop['diag_remove_zero'] = False

        # this are the parameters which were used in old MAPS_Parameters.xml test.
        prop['wb-integr-max'] = 300
        #prop['wb_integr_range']=[20,300]
        prop['bkgd-range-min'] = 12000
        prop['bkgd-range-max'] = 18000
        #prop['bkgd_range']=[12000,18000]

        prop['diag_samp_hi'] = 1.5
        prop['diag_samp_sig'] = 3.3
        prop['diag_van_hi'] = 2.0

        prop['abs_units_van_range'] = [-40,40]

        return prop
      #
    @iliad
    def reduce(self,input_file=None,output_directory=None):
        """ Method executes reduction over single file
         Overload only if custom reduction is needed
        """
        outWS = ReductionWrapper.reduce(self,input_file,output_directory)
        #SaveNexus(ws,Filename = 'MARNewReduction.nxs')
        return outWS

    def __init__(self,web_var=None):
        """ sets properties defaults for the instrument with Name"""
        ReductionWrapper.__init__(self,'MAP',web_var)
    #
    def set_custom_output_filename(self):
        """ define custom name of output files if standard one is not satisfactory
          In addition to that, example of accessing reduction properties
          Changing them if necessary
        """
        def custom_name(prop_man):
            """ sample function which builds filename from
              incident energy and run number and adds some auxiliary information
              to it.
            """
            # Note -- properties have the same names as the list of advanced and
            # main properties
            ei = prop_man.incident_energy
            # sample run is more then just list of runs, so we use
            # the formalization below to access its methods
            run_num = PropertyManager.sample_run.run_number()
            name = "RUN{0}atEi{1:<4.1f}meV_One2One".format(run_num ,ei)
            return name

        # Uncomment this to use custom filename function
        # Note: the properties are stored in prop_man class accessed as
        # below.
        #return lambda : custom_name(self.reducer.prop_man)
        # use this method to use standard file name generating function
        return None

#----------------------------------------------------------------------------------------------------------------------
if __name__ == "__main__":

    data_root = r'd:\Data\MantidDevArea\Datastore\DataCopies'
    data_dir  = os.path.join(data_root,r'Testing\Data\SystemTest')
    ref_data_dir = os.path.join(data_root,r'Testing\SystemTests\tests\analysis\reference')
    result_dir = r'd:/Data/Mantid_Testing/14_12_15'

    config.setDataSearchDirs('{0};{1};{2}'.format(data_dir,ref_data_dir,result_dir))
    #config.appendDataSearchDir('d:/Data/Mantid_GIT/Test/AutoTestData')
    config['defaultsave.directory'] = result_dir # folder to save resulting spe/nxspe files.  Defaults are in

    # execute stuff from Mantid
    rd = ReduceMAPS()
    rd.def_advanced_properties()
    rd.def_main_properties()


#### uncomment rows below to generate web variables and save then to transfer to ###
    ## web services.
    #run_dir = os.path.dirname(os.path.realpath(__file__))
    #file = os.path.join(run_dir,'reduce_vars.py')
    #rd.save_web_variables(file)

#### Set up time interval (sec) for reducer to check for input data file.  ####
    #  If this file is not present and this value is 0,reduction fails
    #  if this value >0 the reduction wait until file appears on the data
    #  search path checking after time specified below.
    rd.wait_for_file = 0  # waiting time interval

####get reduction parameters from properties above, override what you want locally ###
   # and run reduction.  Overriding would have form:
   # rd.reducer.property_name (from the dictionary above) = new value e.g.
   # rd.reducer.energy_bins = [-40,2,40]
   # or
   ## rd.reducer.sum_runs = False

###### Run reduction over all run numbers or files assigned to ######
     # sample_run variable

    # return output workspace only if you are going to do
    # something with it here.  Running range of runs will return the array
    # of workspace pointers.
    #red_ws = rd.run_reduction()
    # usual way to go is to reduce workspace and save it internally
    rd.run_reduction()


#### Validate reduction result against known result, obtained earlier  ###
    #rez,mess=rd.validate_result()
    #if not rez:
    #   raise RuntimeError("validation failed with error: {0}".format(mess))
    #else:
    #   print "ALL Fine"
