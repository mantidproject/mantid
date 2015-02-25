""" Sample LET reduction scrip """ 
import os
os.environ["PATH"] = r"c:/Mantid/Code/builds/br_master/bin/Release;"+os.environ["PATH"]
 

from Direct.ReductionWrapper import *
try:
    import reduce_vars as rv
except:
    rv = None

#
def find_binning_range(energy,ebin):
    """ function finds the binning range used in multirep mode 
        for merlin ls=11.8,lm2=10. mult=2.8868 dt_DAE=1
        for LET    ls=25,lm2=23.5 mult=4.1     dt_DAE=1.6
        all these values have to be already present in IDF and should be taken from there

        # THIS FUNCTION SHOULD BE MADE GENERIG AND MOVED OUT OF HERE
    """

    InstrName =  config['default.instrument'][0:3]
    if InstrName.find('LET')>-1:
        ls  =25
        lm2 =23.5
        mult=4.1
        dt_DAE = 1.6
    elif InstrName.find('MER')>-1:
        ls =11.8
        lm2=10
        mult=2.8868
        dt_DAE = 1
    else:
       raise RuntimeError("Find_binning_range: unsupported/unknown instrument found")

    energy=float(energy)

    emin=(1.0-ebin[2])*energy   #minimum energy is with 80% energy loss
    lam=(81.81/energy)**0.5
    lam_max=(81.81/emin)**0.5
    tsam=252.82*lam*ls   #time at sample
    tmon2=252.82*lam*lm2 #time to monitor 6 on LET
    tmax=tsam+(252.82*lam_max*mult) #maximum time to measure inelastic signal to
    t_elastic=tsam+(252.82*lam*mult)   #maximum time of elastic signal
    tbin=[int(tmon2),dt_DAE,int(tmax)]				
    energybin=[float("{0: 6.4f}".format(elem*energy)) for elem in ebin]

    return (energybin,tbin,t_elastic)
#--------------------------------------------------------------------------------------------------------
def find_background(ws_name,bg_range):
    """ Function to find background from multirep event workspace
     dt_DAE = 1 for MERLIN and 1.6 for LET
     should be precalculated or taken from IDF

        # THIS FUNCTION SHOULD BE MADE GENERIC AND MOVED OUT OF HERE
    """
    InstrName =  config['default.instrument'][0:3]
    if InstrName.find('LET')>-1:
        dt_DAE = 1.6
    elif InstrName.find('MER')>-1:
        dt_DAE = 1
    else:
       raise RuntimeError("Find_binning_range: unsupported/unknown instrument found")

    bg_ws_name = 'bg'
    delta=bg_range[1]-bg_range[0]
    Rebin(InputWorkspace='w1',OutputWorkspace=bg_ws_name,Params=[bg_range[0],delta,bg_range[1]],PreserveEvents=False)	
    v=(delta)/dt_DAE
    CreateSingleValuedWorkspace(OutputWorkspace='d',DataValue=v)
    Divide(LHSWorkspace=bg_ws_name,RHSWorkspace='d',OutputWorkspace=bg_ws_name)
    return bg_ws_name


class ReduceLET_OneRep(ReductionWrapper):
   @MainProperties
   def def_main_properties(self):
       """ Define main properties used in reduction """ 


       prop = {}
       ei = 7.0
       ebin = [-1,0.002,0.95]
 
       prop['sample_run'] = 'LET00006278.nxs'
       prop['wb_run'] = 'LET00005545.raw'
       prop['incident_energy'] = ei
       prop['energy_bins'] = ebin

       
      # Absolute units reduction properties.
       #prop['monovan_run'] = 17589
       #prop['sample_mass'] = 10/(94.4/13) # -- this number allows to get approximately the same system test intensities for MAPS as the old test
       #prop['sample_rmm'] = 435.96 #
       return prop

   @AdvancedProperties
   def def_advanced_properties(self):
      """  separation between simple and advanced properties depends
           on scientist, experiment and user.
           main properties override advanced properties.      
      """

      prop = {}
      prop['map_file'] = 'rings_103'
      prop['hard_mask_file'] ='LET_hard.msk'
      prop['det_cal_file'] = 'det_corrected7.dat'
      prop['save_format']=''
      prop['bleed'] = False
      prop['norm_method']='current'
      prop['detector_van_range']=[0.5,200]
      prop['load_monitors_with_workspace']=True
      #TODO: this has to be loaded from the workspace and work without this 
      #prop['ei-mon1-spec']=40966
     
      
      return prop
      #
   @iliad
   def reduce(self,input_file=None,output_directory=None):
     # run reduction, write auxiliary script to add something here.

      prop = self.reducer.prop_man
      # Ignore input properties for the time being
      white_ws = 'wb_wksp'
      LoadRaw(Filename='LET00005545.raw',OutputWorkspace=white_ws)
      #prop.wb_run = white_ws

      sample_ws = 'w1'
      monitors_ws = sample_ws + '_monitors'
      LoadEventNexus(Filename='LET00006278.nxs',OutputWorkspace=sample_ws,
                     SingleBankPixelsOnly='0',LoadMonitors='1',
                     MonitorsAsEvents='1')
      ConjoinWorkspaces(InputWorkspace1=sample_ws, InputWorkspace2=monitors_ws)
      #prop.sample_run = sample_ws


      ebin = prop.energy_bins
      ei   = prop.incident_energy

      (energybin,tbin,t_elastic) = find_binning_range(ei,ebin)
      Rebin(InputWorkspace=sample_ws,OutputWorkspace=sample_ws, Params=tbin, PreserveEvents='1')

      prop.bkgd_range=[int(t_elastic),int(tbin[2])]

      ebinstring = str(energybin[0])+','+str(energybin[1])+','+str(energybin[2])
      self.reducer.prop_man.energy_bins = ebinstring

      red = DirectEnergyConversion()

      red.initialise(prop)
      outWS = red.convert_to_energy(white_ws,sample_ws)
      #SaveNexus(ws,Filename = 'MARNewReduction.nxs')

      #when run from web service, return additional path for web server to copy data to"
      return outWS

   def __init__(self,rv=None):
     """ sets properties defaults for the instrument with Name"""
     ReductionWrapper.__init__(self,'LET',rv)
#----------------------------------------------------------------------------------------------------------------------

class ReduceLET_MultiRep2014(ReductionWrapper):
   @MainProperties
   def def_main_properties(self):
       """ Define main properties used in reduction """ 


       prop = {}
       ei=[3.4,8.] # multiple energies provided in the data file
       ebin=[-4,0.002,0.8]    #binning of the energy for the spe file. The numbers are as a fraction of ei [from ,step, to ]
 
       prop['sample_run'] = [14305]
       prop['wb_run'] = 5545
       prop['incident_energy'] = ei
       prop['energy_bins'] = ebin

       
      # Absolute units reduction properties.
       # Vanadium labelled Dec 2011 - flat plate of dimensions: 40.5x41x2.0# volume = 3404.025 mm**3 mass= 20.79
       prop['monovan_run'] = 14319 # vanadium run in the same configuration as your sample
       prop['sample_mass'] = 20.79 # 17.25  # mass of your sample (PrAl3)
       prop['sample_rmm'] = 50.9415 # 221.854  # molecular weight of your sample

       return prop

   @AdvancedProperties
   def def_advanced_properties(self):
      """  separation between simple and advanced properties depends
           on scientist, experiment and user.
           main properties override advanced properties.      
      """

      prop = {}
      prop['map_file'] = 'rings_103.map'
      prop['det_cal_file'] = 'det_corrected7.nxs'
      prop['save_format']=''
      prop['bleed'] = False
      prop['norm_method']='current'
      prop['detector_van_range']=[2,7]
      prop['background_range'] = [92000,98000] # TOF range for the calculating flat background
      prop['hardmaskOnly']='LET_hard.msk' # diag does not work well on LET. At present only use a hard mask RIB has created

      # Disable internal background check TODO: fix internal background check
      prop['check_background']=False

      prop['monovan_mapfile'] = 'rings_103.map'

      #TODO: Correct monitor, depending on workspace. This has to be loaded from the workspace and work without this settings 
      #prop['ei-mon1-spec']=40966



      return prop
      #
   @iliad
   def reduce(self,input_file=None,output_directory=None):
     # run reduction, write auxiliary script to add something here.

      red_properties = self.reducer.prop_man
      #######
      wb= red_properties.wb_run
      run_no = red_properties.sample_run
      bg_range =  red_properties.background_range
      ei = red_properties.incident_energy
      ebin = red_properties.energy_bins

      remove_background = True  #if true then will subtract a flat background in time from the time range given below otherwise put False

      red = DirectEnergyConversion()

      red.initialise(red_properties)

      energybin,tbin,t_elastic = find_binning_range(ei[0],ebin)
      energybin,tbin,t_elastic = find_binning_range(ei[1],ebin)

    # loads the white-beam (or rather the long monovan ). Does it as a raw file to save time as the event mode is very large
      if 'wb_wksp' in mtd:
            wb_wksp=mtd['wb_wksp']
      else:  #only load white-beam if not already there
          wb_wksp = LoadRaw(Filename='LET0000'+str(wb)+'.raw',OutputWorkspace='wb_wksp')
        #dgreduce.getReducer().det_cal_file = 'det_corrected7.nxs'
        #wb_wksp = dgreduce.getReducer().load_data('LET0000'+str(wb)+'.raw','wb_wksp')
        #dgreduce.getReducer().det_cal_file = wb_wksp

      for run in [run_no]:     #loop around runs
          fname='LET0000'+str(run)+'.nxs'
          print ' processing file ', fname
          #w1 = dgreduce.getReducer().load_data(run,'w1')
          Load(Filename=fname,OutputWorkspace='w1',LoadMonitors='1')

    
          if remove_background:
                bg_ws_name=find_background('w1',bg_range)

        #############################################################################################
        # this section finds all the transmitted incident energies if you have not provided them
        #if len(ei) == 0:  -- not tested here -- should be unit test for that. 
           #ei = find_chopper_peaks('w1_monitors')       
          print 'Energies transmitted are:'
          print (ei)

          RenameWorkspace(InputWorkspace = 'w1',OutputWorkspace='w1_storage')
          RenameWorkspace(InputWorkspace = 'w1_monitors',OutputWorkspace='w1_mon_storage')
                    
         #now loop around all energies for the run
          result =[]
          for ind,energy in enumerate(ei):
                print float(energy)
                (energybin,tbin,t_elastic) = find_binning_range(energy,ebin)
                print " Rebinning will be performed in the range: ",energybin
                # if we calculate more then one energy, initial workspace will be used more then once
                if ind <len(ei)-1:
                    CloneWorkspace(InputWorkspace = 'w1_storage',OutputWorkspace='w1')
                    CloneWorkspace(InputWorkspace = 'w1_mon_storage',OutputWorkspace='w1_monitors')
                else:
                    RenameWorkspace(InputWorkspace = 'w1_storage',OutputWorkspace='w1')
                    RenameWorkspace(InputWorkspace = 'w1_mon_storage',OutputWorkspace='w1_monitors')

                if remove_background:
                    w1=Rebin(InputWorkspace='w1',OutputWorkspace='w1',Params=tbin,PreserveEvents=False)            
                    Minus(LHSWorkspace='w1',RHSWorkspace='bg',OutputWorkspace='w1')
               

                ######################################################################
                # ensure correct round-off procedure
                argi={}
                argi['monovan_integr_range']=[round(ebin[0]*energy,4),round(ebin[2]*energy,4)] # integration range of the vanadium 
                #MonoVanWSName = None
       
                # absolute unit reduction -- if you provided MonoVan run or relative units if monoVan is not present
                out=red.convert_to_energy(wb_wksp,"w1",energy,energybin,**argi)

                ws_name = 'LETreducedEi{0:2.1f}'.format(energy)
                RenameWorkspace(InputWorkspace=out,OutputWorkspace=ws_name)
                result.append(mtd[ws_name])

                #TODO: this will go when multirep mode is implemented properly
                # Store processed workspaces back to properties
                wb_wksp  = PropertyManager.wb_run.get_workspace()

    
                #SaveNXSPE(InputWorkspace=ws_name,Filename=ws_name+'.nxspe')

      #######
      #when run from web service, return additional path for web server to copy data to"
      return result

   def __init__(self,rv=None):
       """ sets properties defaults for the instrument with Name"""
       ReductionWrapper.__init__(self,'LET',rv)

class ReduceLET_MultiRep2015(ReductionWrapper):
   @MainProperties
   def def_main_properties(self):
       """ Define main properties used in reduction """ 


       prop = {}
       ei=[3.4,8.] # multiple energies provided in the data file
       ebin=[-4,0.002,0.8]    #binning of the energy for the spe file. The numbers are as a fraction of ei [from ,step, to ]
 
       prop['sample_run'] = [14305]
       prop['wb_run'] = 5545
       prop['incident_energy'] = ei
       prop['energy_bins'] = ebin

       
      # Absolute units reduction properties.
       # Vanadium labelled Dec 2011 - flat plate of dimensions: 40.5x41x2.0# volume = 3404.025 mm**3 mass= 20.79
       prop['monovan_run'] = 14319 # vanadium run in the same configuration as your sample
       prop['sample_mass'] = 20.79 # 17.25  # mass of your sample (PrAl3)
       prop['sample_rmm'] = 50.9415 # 221.854  # molecular weight of your sample

       return prop

   @AdvancedProperties
   def def_advanced_properties(self):
      """  separation between simple and advanced properties depends
           on scientist, experiment and user.
           main properties override advanced properties.      
      """

      prop = {}
      prop['map_file'] = 'rings_103.map'
      prop['det_cal_file'] = 'det_corrected7.nxs'
      prop['bleed'] = False
      prop['norm_method']='current'
      prop['detector_van_range']=[2,7]
      prop['background_range'] = [92000,98000] # TOF range for the calculating flat background
      prop['hardmaskOnly']='LET_hard.msk' # diag does not work well on LET. At present only use a hard mask RIB has created

      prop['check_background']=True

      prop['monovan_mapfile'] = 'rings_103.map'
      prop['save_format'] = ''
       # if two input files with the same name and  different extension found, what to prefer. 
      prop['data_file_ext']='.nxs' # for LET it may be choice between event and histo mode if 
      # raw file is written in histo, and nxs -- in event mode
                                    
      prop['monovan_mapfile'] = 'rings_103.map'


      #TODO: Correct monitor, depending on workspace. This has to be loaded from the workspace and work without this settings 
      #prop['ei-mon1-spec']=40966



      return prop
      #
   @iliad
   def reduce(self,input_file=None,output_directory=None):
      """ Method executes reduction over single file

          Overload only if custom reduction is needed or 
          special features are requested
      """
      res = ReductionWrapper.reduce(self,input_file,output_directory)
      #
      en = self.reducer.prop_man.incident_energy
      for ind,energy in enumerate(en):
          ws_name = 'LETreducedEi{0:2.1f}'.format(energy)
          RenameWorkspace(InputWorkspace=res[ind],OutputWorkspace=ws_name)
          res[ind]= mtd[ws_name]

      #SaveNexus(ws,Filename = 'LETNewReduction.nxs')
      return res

   def __init__(self,rv=None):
       """ sets properties defaults for the instrument with Name"""
       ReductionWrapper.__init__(self,'LET',rv)

#----------------------------------------------------------------------------------------------------------------------



if __name__=="__main__":
     maps_dir = 'd:/Data/MantidSystemTests/Data'
     data_dir ='d:/Data/Mantid_Testing/14_11_27'
     ref_data_dir = 'd:/Data/MantidSystemTests/SystemTests/AnalysisTests/ReferenceResults' 
     config.setDataSearchDirs('{0};{1};{2}'.format(data_dir,maps_dir,ref_data_dir))
     #config.appendDataSearchDir('d:/Data/Mantid_GIT/Test/AutoTestData')
     config['defaultsave.directory'] = data_dir # folder to save resulting spe/nxspe files. Defaults are in

     # execute stuff from Mantid
     rd =ReduceLET_MultiRep2015()
     #rd =ReduceLET_MultiRep2014()
     #rd = ReduceLET_OneRep()
     rd.def_advanced_properties()
     rd.def_main_properties()


     #using_web_data = False
     #if not using_web_data:
     #   run_dir=os.path.dirname(os.path.realpath(__file__))
     #   file = os.path.join(run_dir,'reduce_vars.py')
     #   rd.export_changed_values(file)

     rd.reduce()
