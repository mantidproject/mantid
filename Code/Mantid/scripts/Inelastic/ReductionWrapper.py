from mantid import config
from DirectEnergyConversion import  DirectEnergyConversion
from DirectPropertyManager import DirectPropertyManager;
import os
try:
    import reduce_vars as rv
    using_web_data = True
except:
    using_web_data = False

class ReductionWrapper(object):
    def __init__(self,instrumentName):
      """ sets properties defaults for the instrument with Name"""
      self.red_prop = DirectPropertyManager(instrumentName)
    def set_main_properties(self):
       # main properties.
        self.red_prop.sample_run = 11001;
        self.red_prop.wb_run     = 11060;

        self.red_prop.incident_energy =  12;
        self.red_prop.energy_bins     = [-11,0.05,11]

      # Absolute units reduction properties.
        self.red_prop.monovan_run= 11015;
        self.red_prop.sample_mass= 10;
        self.red_prop.sample_rmm = 435.96

    def set_advanced_properties(self):
      #  separation between simple and advanced properties depends
      #  on scientist, experiment and user.
      #  main properties override advanced properties.
        self.red_prop.record_advanced_properties = True;
        #
        self.red_prop.map_file = "mari_res.map"
        self.red_prop.monovan_mapfile = "mari_res.map"
        self.red_prop.hard_mask_file ="mar11015.msk"
        self.red_prop.det_cal_file =11060
        self.red_prop.save_format='nxs'

        self.red_prop.record_advanced_properties = False;
    def run(self):
       # run reduction, write auxiliary script to add something here.
       red = DirectEnergyConversion();

       if using_web_data:
           web_vars = dict(rv.standard_vars.items()+rv.advanced_vars.items());
           self.red_prop.set_input_parameters(**web_vars);
       #end

       red.initialise(self.red_prop);
       ws = red.convert_to_energy_transfer();
       return ws;

if __name__=="__main__":
     maps_dir = 'd:/Data/MantidSystemTests/Data'
     data_dir ='d:/Data/Mantid_Testing/14_11_27'
     ref_data_dir = 'd:/Data/MantidSystemTests/SystemTests/AnalysisTests/ReferenceResults' 
     config.setDataSearchDirs('{0};{1};{2}'.format(data_dir,maps_dir,ref_data_dir))
     #config.appendDataSearchDir('d:/Data/Mantid_GIT/Test/AutoTestData')
     config['defaultsave.directory'] = data_dir # folder to save resulting spe/nxspe files. Defaults are in



     rd= ReductionWrapper('MARI')
     if not using_web_data:
        run_dir=os.path.dirname(os.path.realpath(__file__))
        rd.set_advanced_properties();
        rd.set_main_properties();
        file = os.path.join(run_dir,'reduce_vars.py');
        rd.red_prop.export_changed_values(file);

     rd.run(); 
