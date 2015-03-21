#pylint: disable=invalid-name
""" Sample LET reduction script """
import os
#os.environ["PATH"] = r"c:/Mantid/Code/builds/br_master/bin/Release;"+os.environ["PATH"]


from Direct.ReductionWrapper import *
try:
    import reduce_vars as web_var
except:
    web_var = None

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
class ReduceLET_OneRep(ReductionWrapper):
    @MainProperties
    def def_main_properties(self):
        """Define main properties used in reduction """


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
        """Separation between simple and advanced properties depends
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
        return prop
        #
    @iliad
    def reduce(self,input_file=None,output_directory=None):
        """run reduction, write auxiliary script to add something here."""

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

    def __init__(self,web_var=None):
        """Sets properties defaults for the instrument with Name"""
        ReductionWrapper.__init__(self,'LET',web_var)
#----------------------------------------------------------------------------------------------------------------------

class ReduceLET_MultiRep2015(ReductionWrapper):
    @MainProperties
    def def_main_properties(self):
        """Define main properties used in reduction """


        prop = {}
        ei=[3.4,8.] # multiple energies provided in the data file
        ebin=[-4,0.002,0.8]    #binning of the energy for the spe file. The numbers are as a fraction of ei [from ,step, to ]

        prop['sample_run'] = [14305]
        prop['wb_run'] = 5545
        prop['incident_energy'] = ei
        prop['energy_bins'] = ebin


        # Absolute units reduction properties.
        # Vanadium labeled Dec 2011 - flat plate of dimensions: 40.5x41x2.0# volume = 3404.025 mm**3 mass= 20.79
        prop['monovan_run'] = 14319 # vanadium run in the same configuration as your sample
        prop['sample_mass'] = 20.79 # 17.25  # mass of your sample (PrAl3)
        prop['sample_rmm'] = 50.9415 # 221.854  # molecular weight of your sample

        return prop

    @AdvancedProperties
    def def_advanced_properties(self):
        """separation between simple and advanced properties depends
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
        # if two input files with the same name and different extension found, what to prefer.
        prop['data_file_ext']='.nxs' # for LET it may be choice between event and histo mode if
        # raw file is written in histo, and nxs -- in event mode

        # Absolute units: map file to calculate monovan integrals
        prop['monovan_mapfile'] = 'rings_103.map'
        #Change this to correct value and verify that motor_log_names refers correct and existing
        # log name for crystal rotation to write correct psi value into nxspe files
        prop['motor_offset']=None

        #BUG TODO: old IDF-s do not have this property. In this case, new IDF overrides the old one
        # Should be possibility to define spectra_to_monitors_list to just monitors list, if
        # spectra_to_monitors_list remains undefined
        prop['spectra_to_monitors_list']=5506
        return prop
      #
    @iliad
    def reduce(self,input_file=None,output_directory=None):
        """Method executes reduction over single file

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

    def __init__(self,web_var=None):
        """Sets properties defaults for the instrument with Name"""
        ReductionWrapper.__init__(self,'LET',web_var)

    def set_custom_output_filename(self):
        """Define custom name of output files if standard one is not satisfactory
          In addition to that, example of accessing reduction properties
          Changing them if necessary
        """
        def custom_name(prop_man):
            """sample function which builds filename from
              incident energy and run number and adds some auxiliary information
              to it.
            """
            # Note -- properties have the same names  as the list of advanced and
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

if __name__=="__main__":
    maps_dir = r'd:\Data\MantidDevArea\Datastore\DataCopies\Testing\Data\SystemTest'
    data_dir = r'd:\Data\Mantid_Testing\15_03_01'
    ref_data_dir = r'd:\Data\MantidDevArea\Datastore\DataCopies\Testing\SystemTests\tests\analysis\reference'
    config.setDataSearchDirs('{0};{1};{2}'.format(data_dir,maps_dir,ref_data_dir))
    #config.appendDataSearchDir('d:/Data/Mantid_GIT/Test/AutoTestData')
    config['defaultsave.directory'] = data_dir # folder to save resulting spe/nxspe files. Defaults are in

    # execute stuff from Mantid
    rd =ReduceLET_MultiRep2015()
    #rd = ReduceLET_OneRep()
    rd.def_advanced_properties()
    rd.def_main_properties()


#### uncomment rows below to generate web variables and save then to transfer to   ###
    ## web services.
    #run_dir = os.path.dirname(os.path.realpath(__file__))
    #file = os.path.join(run_dir,'reduce_vars.py')
    #rd.save_web_variables(file)

#### Set up time interval (sec) for reducer to check for input data file.         ####
    #  If this file is not present and this value is 0,reduction fails
    #  if this value >0 the reduction wait until file appears on the data
    #  search path checking after time specified below.
    rd.wait_for_file = 0  # waiting time interval

####get reduction parameters from properties above, override what you want locally ###
   # and run reduction. Overriding would have form:
   # rd.reducer.property_name (from the dictionary above) = new value e.g.
   # rd.reducer.energy_bins = [-40,2,40]
   # or
   ## rd.reducer.sum_runs = False

###### Run reduction over all run numbers or files assigned to                   ######
    # sample_run  variable

    # return output workspace only if you are going to do
    # something with it here. Running range of runs will return the array
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