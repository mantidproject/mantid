""" Sample MARI reduction scrip used in testing ReductionWrapper """

from Direct.ReductionWrapper import *
try:
    import reduce_vars as web_var
except:
    web_var = None




class ReduceMARI(ReductionWrapper):
#-------------------------------------------------------------------------------------------------#
   @MainProperties
   def def_main_properties(self):
        """ Define main properties used in reduction """
       prop = {}
       prop['sample_run'] = 11001
       prop['wb_run'] = 11060
       prop['incident_energy'] = 12
       prop['energy_bins'] = [-11,0.05,11]

      # Absolute units reduction properties.
       prop['monovan_run'] = 11015
       prop['sample_mass'] = 10
       prop['sample_rmm'] = 435.96
       return prop
#-------------------------------------------------------------------------------------------------#
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
      prop['det_cal_file'] = "11060"
      prop['save_format'] = ''
      return prop
      #
#-------------------------------------------------------------------------------------------------#
   @iliad
   def reduce(self,input_file=None,output_directory=None):
      """ Method executes reduction over single file

          Overload only if custom reduction is needed
      """
      ws = ReductionWrapper.reduce(input_file,output_directory)
      #SaveNexus(ws,Filename = 'MARNewReduction.nxs')
      return ws
#------------------------------------------------------------------------------------------------ #
   def validate_result(self,build_validation=False):
      """ Change this method to verify different results     """

      # Two commented code rows below define location of the validation file in this script folder 
      # (which is incorrect for Mantid development, as the files are on the data search path or
      # mantid distribution, as the files are not there)
      #run_dir = os.path.dirname(os.path.realpath(__file__))
      #validation_file = os.path.join(run_dir,"MARIReduction.nxs")

      # build_validation -- if true, build and overwrite new workspace rather then validating the old one
      # if file is missing, the validation tries to build it
      rez,message = ReductionWrapper.build_or_validate_result(self,11001,"MARIReduction.nxs",
                                                              build_validation,1.e-3)
      return rez,message

   def __init__(self,web_var=None):
       """ sets properties defaults for the instrument with Name"""
       ReductionWrapper.__init__(self,'MAR',web_var)
#-------------------------------------------------------------------------------------------------#
#-------------------------------------------------------------------------------------------------#
#-------------------------------------------------------------------------------------------------#
def main(input_file=None,output_directory=None):
    """ This method is used to run code from web service
        and should not be touched unless you change the name of the
        particular ReductionWrapper class (e.g. ReduceMARI here)

        exception to change the output folder to save data to
    """
    # note web variables initialization
    rd = ReduceMARI(web_var)
    rd.reduce(input_file,output_directory)

    # Define folder for web service to copy results to
    output_folder = ''
    return output_folder

if __name__ == "__main__":
#-------------------------------------------------------------------------------------------------#
# SECTION USED TO RUN REDUCTION FROM MANTID SCRIPT WINDOW #
#-------------------------------------------------------------------------------------------------#
##### Here one sets up folders where to find input data and where to save results             #####
    # It can be done here or from Mantid GUI
    # Folder where map files are located:
     map_mask_dir = 'd:/Data/MantidSystemTests/Data'
    # folder where input data can be found
     data_dir = 'd:/Data/Mantid_Testing/14_11_27'
     # auxiliary folder with results
    ref_data_dir = 'd:/Data/MantidSystemTests/SystemTests/AnalysisTests/ReferenceResults'
     # Set input path to
     config.setDataSearchDirs('{0};{1};{2}'.format(data_dir,map_mask_dir,ref_data_dir))
     # use appendDataSearch directory to add to existing data search path
     #config.appendDataSearchDir('d:/Data/Mantid_GIT/Test/AutoTestData')
     # folder to save resulting spe/nxspe files.
    config['defaultsave.directory'] = data_dir

###### Initialize reduction class above and set up reduction properties. Note no parameters  ######
     rd = ReduceMARI()
     # set up advanced and main properties
     rd.def_advanced_properties()
     rd.def_main_properties()

     # uncomment rows below to save web variables to use in web services.
     #run_dir = os.path.dirname(os.path.realpath(__file__))
     #file = os.path.join(run_dir,'reduce_vars.py')
     #rd.save_web_variables(file)

    rd.reduce()

