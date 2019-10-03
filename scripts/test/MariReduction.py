# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
""" Sample MARI reduction scrip used in testing ReductionWrapper """
import os

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
       prop['sample_run'] = 21461
       prop['wb_run'] = 21334
       prop['incident_energy'] = 50
       prop['energy_bins'] = [-10,0.1,49]

      # Absolute units reduction properties.
       prop['monovan_run'] = None
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
      prop['map_file'] = "mari_res2013.map"
      prop['monovan_mapfile'] = "mari_res2013.map"
      prop['hard_mask_file'] = "mari_mask2016_1.msk"
      prop['det_cal_file'] = 21334
      prop['save_format'] = ''
      return prop
      #
#-------------------------------------------------------------------------------------------------#
    @iliad
    def reduce(self,input_file=None,output_directory=None):
      """ Method executes reduction over single file
          Overload only if custom reduction is needed
      """
      """ Here one can define custom preprocessing procedure, to be applied to the whole
          run, obtained from instrument
      1) Get access to the pointer to the input workspace 
         (the workspace will be loaded  and calibrated at this point if it has not been done yet)
          using sample_run property class.
      run_ws = PropertyManager.sample_run.get_worksapce()
      2) Get access to any propertrty defined in Instrument_Properties.xml file
         or redefined in the reduction script:
      properties = self.prop_man
        e.g:
      RunNumber = properties.sample_run
      ei = properties.incident_energy
        Perform custom preprocessing procedure (with the values you specified)
        preprocessed_ws = custom_preprocessing_procedure(run_ws,RunNumber,ei,...)
      3) Store preprocessed workspace in the sample_run property for further analysis
         for the case where the workspace name have changed in the Analysis Data Service
        (this situation is difficult to predict so better always do this operation)
      PropertyManager.sample_run.synchronize_ws(preprocessed_ws)
      """
      ws = ReductionWrapper.reduce(self,input_file,output_directory)
      """ Here one can deploy custrom post-processing procedure, in the way, similar to
      the preprocessing procedure, using either ws pointer, or the procedure, similar to 
      pre-processing:
      ws = PropertyManager.sample_run.get_worksapce()
      Do post-processing.....
      ws_postprocessed = .....
      Do not forget to store result in the data property.
      PropertyManager.sample_run.synchronize_ws(ws_postprocessed)
      """
      #SaveNexus(ws,Filename = 'MARNewReduction.nxs')
      return ws
#------------------------------------------------------------------------------------------------ #
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
     #map_mask_dir = 'd:/Data/MantidSystemTests/Data'
    # folder where input data can be found
     #data_dir = 'd:/Data/Mantid_Testing/14_11_27'
     # auxiliary folder with results
     #ref_data_dir = 'd:/Data/MantidSystemTests/SystemTests/AnalysisTests/ReferenceResults'
     # Set input path to
     #config.setDataSearchDirs('{0};{1};{2}'.format(data_dir,map_mask_dir,ref_data_dir))
     # use appendDataSearch directory to add to existing data search path
     #config.appendDataSearchDir('d:/Data/Mantid_GIT/Test/AutoTestData')
     # folder to save resulting spe/nxspe files.
     #config['defaultsave.directory'] = data_dir

###### Initialize reduction class above and set up reduction properties. Note no parameters  ######
     rd = ReduceMARI()
     # set up advanced and main properties
     rd.def_advanced_properties()
     rd.def_main_properties()

     # uncomment rows below to save web variables to use in web services.
     #run_dir = os.path.dirname(os.path.realpath(__file__))
     #file = os.path.join(run_dir,'reduce_vars.py')
     #rd.save_web_variables(file)

     # Web-like reduction over sequence of files
     #rd.reduce()
###### Run reduction on all files provided as parameters ######
     red_ws = rd.run_reduction()
