import os
#os.environ["PATH"] =\
#r"c:/Mantid/Code/builds/br_master/bin/Release;"+os.environ["PATH"]
""" Sample MARI reduction scrip used in testing ReductionWrapper """ 
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
     """ Method executes reduction over single file
         Overload only if custom reduction is needed
     """
     outWS = ReductionWrapper.reduce(self,input_file,output_directory)
     #SaveNexus(outWS,Filename = 'MARNewReduction.nxs')
     return outWS
 
   def validate_result(self,build_validation=False):
      """ Change this method to verify different results     """
      # build_validation -- if true, build and save new workspace rather then validating the old one
      rez,message = ReductionWrapper.build_or_validate_result(self,11001,"MARIReduction.nxs",build_validation,1.e-2)
      return rez,message

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
       """ Define main properties used in reduction """ 
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

     maps_dir = 'd:/Data/MantidSystemTests/Data'
     data_dir = 'd:/Data/Mantid_Testing/14_12_15'
     ref_data_dir = 'd:/Data/MantidSystemTests/SystemTests/AnalysisTests/ReferenceResults' 
     config.setDataSearchDirs('{0};{1};{2}'.format(data_dir,maps_dir,ref_data_dir))
     #config.appendDataSearchDir('d:/Data/Mantid_GIT/Test/AutoTestData')
     config['defaultsave.directory'] = data_dir # folder to save resulting spe/nxspe files.  Defaults are in

     # execute stuff from Mantid
     #rd = ReduceMARIFromFile()
     #rd= ReduceMARIMon2Norm()
     rd = ReduceMARIMonitorsSeparate()
     #rd = ReduceMARIFromWorkspace()
     rd.def_advanced_properties()
     rd.def_main_properties()

     # Save web variables
     run_dir = os.path.dirname(os.path.realpath(__file__))
     file = os.path.join(run_dir,'reduce_vars.py')
     rd.save_web_variables(file)
#### Set up time interval (sec) for reducer to check for input data file.         ####
     #  If this file is not present and this value is 0,reduction fails 
     #  if this value >0 the reduction wait until file appears on the data 
     #  search path checking after time specified below.
     rd.wait_for_file = 0  # waiting time interval

###### Run reduction over all run numbers or files assigned to                   ######
     # sample_run  variable 
     red_ws = rd.run_reduction()


