#pylint: disable=invalid-name
""" Sample MARI reduction scrip used in testing ReductionWrapper """
import os
#os.environ["PATH"] =\
#r"c:/Mantid/Code/builds/br_master/bin/Release;"+os.environ["PATH"]
from Direct.ReductionWrapper import *
try:
    import reduce_vars as web_var
except:
    web_var = None


class ReduceMARIFromFile(ReductionWrapper):
    @MainProperties
    def def_main_properties(self):
        """ Define main properties used in reduction """
        prop = {}
        prop['sample_run'] = 11001
        prop['wb_run'] = 11060
        prop['incident_energy'] = 12
        prop['energy_bins'] = [-11,0.05,11]

       #prop['sum_runs'] = False

        # Absolute units reduction properties.
        prop['monovan_run'] = 11015
        prop['sample_mass'] = 10
        prop['sample_rmm'] = 435.96
        return prop

    @AdvancedProperties
    def def_advanced_properties(self):
        """  separation between simple and advanced properties depends
           on scientist, experiment and user.
           main properties override advanced properties.
        """
        prop = {}
        prop['map_file'] = "mari_res.map"
        prop['monovan_mapfile'] = "mari_res.map"
        prop['hard_mask_file'] = "mar11015.msk"
        prop['det_cal_file'] = 11060
        prop['save_format'] = ''
        return prop
      #
    @iliad
    def reduce(self,input_file=None,output_directory=None):
        """Method executes reduction over single file
         Overload only if custom reduction is needed
        """
        converted_to_energy_transfer_ws = ReductionWrapper.reduce(self,input_file,output_directory)
        #SaveNexus(outWS,Filename = 'MARNewReduction.nxs')
        return converted_to_energy_transfer_ws

    def set_custom_output_filename(self):
        """ define custom name of output files if standard one is not satisfactory
          In addition to that, example of accessing reduction properties
          Changing them if necessary
        """
        def custom_name(prop_man):
            """Sample function which builds filename from
              incident energy and run number and adds some auxiliary information
              to it.
            """
            # Note -- properties have the same names as the list of advanced and
            # main properties
            ei = prop_man.incident_energy
            # sample run is more then just list of runs, so we use
            # the formalization below to access its methods
            run_num = PropertyManager.sample_run.run_number()
            name = "RUN{0}atEi{1:<3.2f}meV_One2One".format(run_num ,ei)
            return name

        # Uncomment this to use custom filename function
        # Note: the properties are stored in prop_man class accessed as
        # below.
        #return lambda : custom_name(self.reducer.prop_man)
        # use this method to use standard file name generating function
        return None


    def __init__(self,web_var=None):
        """ sets properties defaults for the instrument with Name"""
        ReductionWrapper.__init__(self,'MAR',web_var)
#-------------------------------------------------------------------------------------------------#
#-------------------------------------------------------------------------------------------------#
#-------------------------------------------------------------------------------------------------#
def main(input_file=None,output_directory=None):
    """ This method is used to run code from web service
        and should not be touched except changing the name of the
        particular ReductionWrapper class (e.g. ReduceMARI here)

        You can also change the output folder to save data to
        where web services will copy data

        This method will go when web service implements proper factory
    """
    # note web variables initialization
    rd = ReduceMARIFromFile(web_var)
    rd.reduce(input_file,output_directory)
    # change to the name of the folder to save data to
    return ''

#----------------------------------------------------------------------------------------------------------------------
class ReduceMARIFromWorkspace(ReductionWrapper):

    @MainProperties
    def def_main_properties(self):
        """Define main properties used in reduction """
        prop = {}
        prop['sample_run'] = Load(Filename='MAR11001.RAW',OutputWorkspace='MAR11001.RAW')
       # WB workspace
        prop['wb_run'] = Load(Filename='MAR11060.RAW',OutputWorkspace='MAR11060.RAW')
        prop['incident_energy'] = 12
        prop['energy_bins'] = [-11,0.05,11]

      # Absolute units reduction properties.
        prop['monovan_run'] = Load(Filename='MAR11015.RAW',OutputWorkspace='MAR11015.RAW')
        prop['sample_mass'] = 10
        prop['sample_rmm'] = 435.96


        return prop

    @AdvancedProperties
    def def_advanced_properties(self):
        """  separation between simple and advanced properties depends
           on scientist, experiment and user.
           main properties override advanced properties.
      """
        prop = {}
        prop['map_file'] = "mari_res.map"
        prop['monovan_mapfile'] = "mari_res.map"
        prop['hard_mask_file'] = "mar11015.msk"
      # MARI calibration uses one of data files defined on instrument.  Here
      # vanadium run is used for calibration
      # TODO: Why not workspace?
        prop['det_cal_file'] = "11060"
        prop['save_format'] = ''
        return prop
      #
    @iliad
    def reduce(self,input_file=None,output_directory=None):
        """ Method executes reduction over single file
         Overload only if custom reduction is needed
     """
        ws = ReductionWrapper.reduce(self,input_file,output_directory)
     #SaveNexus(ws,Filename = 'MARNewReduction.nxs')
        return ws

    def __init__(self,web_var=None):
        """ sets properties defaults for the instrument with Name"""
        ReductionWrapper.__init__(self,'MAR',web_var)
#----------------------------------------------------------------------------------------------------------------------
class ReduceMARIMon2Norm(ReductionWrapper):

    @MainProperties
    def def_main_properties(self):
        """ Define main properties used in reduction """
        prop = {}
        prop['sample_run'] = Load(Filename='MAR11001.RAW',OutputWorkspace='MAR11001.RAW')
       # WB workspace
        prop['wb_run'] = Load(Filename='MAR11060.RAW',OutputWorkspace='MAR11060.RAW')
        prop['incident_energy'] = 12
        prop['energy_bins'] = [-11,0.05,11]

      # Absolute units reduction properties.
        prop['monovan_run'] = 11015 #Load(Filename='MAR11015.RAW',OutputWorkspace='MAR11015.RAW')
        prop['sample_mass'] = 10
        prop['sample_rmm'] = 435.96

        return prop

    @AdvancedProperties
    def def_advanced_properties(self):
        """  separation between simple and advanced properties depends
           on scientist, experiment and user.
           main properties override advanced properties.
      """
        prop = {}
        prop['map_file'] = "mari_res.map"
        prop['monovan_mapfile'] = "mari_res.map"
        prop['hard_mask_file'] = "mar11015.msk"
      #prop['hardmaskOnly'] ="mar11015.msk"
        prop['normalise_method'] = 'monitor-2'
      # reduction from workspace currently needs detector_calibration file
      # MARI calibration uses one of data files defined on instrument.  Here
      # vanadium run is used for calibration
      # TODO: Why not workspace?, check it
        prop['det_cal_file'] = "11060"
        prop['save_format'] = []
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
        ReductionWrapper.__init__(self,'MAR',web_var)
#----------------------------------------------------------------------------------------------------------------------
class MARIReductionSum(ReductionWrapper):
    @MainProperties
    def def_main_properties(self):
        """ Define main properties used in reduction """
        prop = {}
        prop['sample_run'] = [11001,11015]
        prop['wb_run'] = 11060
        prop['incident_energy'] = 11
        prop['energy_bins'] = [-11,0.05,11]
        prop['sum_runs'] = True

      # Absolute units reduction properties.
       #prop['monovan_run'] = 11015
       #prop['sample_mass'] = 32.58
       #prop['sample_rmm'] = 50.9415# 435.96
        return prop

    @AdvancedProperties
    def def_advanced_properties(self):
        """  separation between simple and advanced properties depends
           on scientist, experiment and user.
           main properties override advanced properties.
      """
        prop = {}
        prop['map_file'] = "mari_res.map"
        prop['monovan_mapfile'] = "mari_res.map"
        prop['hard_mask_file'] = "mar11015.msk"
      #prop['det_cal_file'] =11060
        prop['save_format'] = ''
        return prop
      #
    @iliad
    def reduce(self,input_file=None,output_directory=None):
        """ Method executes reduction over single file
         Overload only if custom reduction is needed
     """
        ws = ReductionWrapper.reduce(self,input_file,output_directory)
     #SaveNexus(ws,Filename = 'MARNewReduction.nxs')
        return ws

    def __init__(self,web_var=None):
        """ sets properties defaults for the instrument with Name"""
        ReductionWrapper.__init__(self,'MAR',web_var)
#----------------------------------------------------------------------------------------------------------------------
class ReduceMARIMonitorsSeparate(ReductionWrapper):

    @MainProperties
    def def_main_properties(self):
        """ Define main properties used in reduction """
        prop = {}
        prop['sample_run'] = 11001 #
       # WB workspace Simulate workspace without monitors
        prop['wb_run'] = Load(Filename='MAR11060.RAW',OutputWorkspace='MAR11060.RAW',LoadMonitors='Exclude')
        prop['incident_energy'] = 12
        prop['energy_bins'] = [-11,0.05,11]

      # Absolute units reduction properties.
        prop['monovan_run'] = 11015 #
        prop['sample_mass'] = 10
        prop['sample_rmm'] = 435.96

        return prop

    @AdvancedProperties
    def def_advanced_properties(self):
        """  separation between simple and advanced properties depends
           on scientist, experiment and user.
           main properties override advanced properties.
      """
        prop = {}
        prop['map_file'] = "mari_res.map"
        prop['monovan_mapfile'] = "mari_res.map"
        prop['hard_mask_file'] = "mar11015.msk"
      # MARI calibration uses one of data files defined on instrument.  Here
      # vanadium run is used for calibration
      # TODO: Why not workspace?
        prop['det_cal_file'] = "11060"
        prop['save_format'] = ''
        prop['load_monitors_with_workspace'] = False
        return prop
      #
    @iliad
    def reduce(self,input_file=None,output_directory=None):
        """ Method executes reduction over single file
         Overload only if custom reduction is needed
     """
        outWS = ReductionWrapper.reduce(self,input_file,output_directory)
     #SaveNexus(outWS,Filename = 'MARNewReduction.nxs')
        return outWS

    def __init__(self,web_var=None):
        """ sets properties defaults for the instrument with Name"""
        ReductionWrapper.__init__(self,'MAR',web_var)


if __name__ == "__main__":

    data_root = r'd:\Data\MantidDevArea\Datastore\DataCopies'
    data_dir = os.path.join(data_root,r'Testing\Data\SystemTest')
    ref_data_dir = os.path.join(data_root,r'Testing\SystemTests\tests\analysis\reference')
    result_dir = r'd:/Data/Mantid_Testing/14_12_15'
    config.setDataSearchDirs('{0};{1};{2}'.format(data_dir,ref_data_dir,result_dir))
    #config.appendDataSearchDir('d:/Data/Mantid_GIT/Test/AutoTestData')
    config['defaultsave.directory'] = result_dir # folder to save resulting spe/nxspe files.  Defaults are in

    # execute stuff from Mantid
    #rd = ReduceMARIFromFile()
    rd= ReduceMARIMon2Norm()
    #rd = ReduceMARIMonitorsSeparate()
    #rd = ReduceMARIFromWorkspace()
    rd.def_advanced_properties()
    rd.def_main_properties()

#### uncomment rows below to generate web variables and save then to transfer to ###
    ## web services.
    run_dir = os.path.dirname(os.path.realpath(__file__))
    file = os.path.join(run_dir,'reduce_vars.py')
    rd.save_web_variables(file)

#### Set up time interval (sec) for reducer to check for input data file.  ####
    #  If this file is not present and this value is 0,reduction fails
    #  if this value >0 the reduction wait until file appears on the data
    #  search path checking after time specified below.
    rd.wait_for_file = 0  # waiting time interval

### Define a run number to validate reduction against future changes    #############
    # Take a run number with good reduced results and build validation run
    # for this result. Then place the validation run together with the reduction script.
    # Next time, the script will run reduction and compare the reduction results against
    # the results obtained earlier.
    #rd.validate_run_number = 21968  # Enabling this property disables normal reduction
    # and forces reduction to reduce run specified here and compares results against
    # validation file, processed earlier or calculate this file if run for the first time.
    #This would ensure that reduction script have not changed,
    #allow to identify reason of changes if it was and would allow to recover the script,
    #used to produce initial reduction if changes are unacceptable.


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

