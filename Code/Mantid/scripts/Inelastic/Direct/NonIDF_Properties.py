#pylint: disable=invalid-name
from Direct.PropertiesDescriptors import *
from Direct.RunDescriptor import RunDescriptor,RunDescriptorDependent


class NonIDF_Properties(object):
    """ Class defines the interface for main properties, used in reduction, and not described in
        IDF ( Instrument_Properties.xml file)

        These properties are main set of properties, user have to set up
        for reduction to work with defaults.

        The example of such properties are run numbers, energy bins and incident energies.
    """

    # logging levels available for user
    log_options = \
        { "error" :       (1,lambda (msg):   logger.error(msg)),
          "warning" :     (2,lambda (msg):   logger.warning(msg)),
          "notice" :      (3,lambda (msg):   logger.notice(msg)),
          "information" : (4,lambda (msg):   logger.information(msg)),
          "debug" :       (5,lambda (msg):   logger.debug(msg))}


    def __init__(self,Instrument,run_workspace=None):
        """ initialize main properties, defined by the class
            @parameter Instrument  -- name or pointer to the instrument,
                       deployed in reduction
        """
        #
        if not run_workspace is None:
            object.__setattr__(self,'sample_run',run_workspace)

        # Helper properties, defining logging options
        object.__setattr__(self,'_log_level','notice')
        object.__setattr__(self,'_log_to_mantid',False)
        object.__setattr__(self,'_current_log_level',3)


        object.__setattr__(self,'_psi',float('NaN'))
        # SNS motor stuff which is difficult to test as I've never seen it
        object.__setattr__(self,'_motor_name',None)
        object.__setattr__(self,'_motor_offset',0)

        object.__setattr__(self,'_save_file_name',None)

        self._set_instrument_and_facility(Instrument,run_workspace)

        # set up descriptors holder class reference
        RunDescriptor._holder = self
        RunDescriptor._logger = self.log
        # Initiate class-level properties to defaults (Each constructor clears class-level properties?)
        super(NonIDF_Properties,self).__setattr__('sample_run',None)
        super(NonIDF_Properties,self).__setattr__('wb_run',None)
        super(NonIDF_Properties,self).__setattr__('monovan_run',None)

        super(NonIDF_Properties,self).__setattr__('mask_run',None)
        super(NonIDF_Properties,self).__setattr__('wb_for_monovan_run',None)
        super(NonIDF_Properties,self).__setattr__('second_white',None)
        super(NonIDF_Properties,self).__setattr__('_tmp_run',None)


    #end
    def log(self, msg,level="notice"):
        """Send a log message to the location defined
        """
        lev,logger = NonIDF_Properties.log_options[level]
        if self._log_to_mantid:
            logger(msg)
        else:
        # TODO: reconcile this with Mantid.
            if lev <= self._current_log_level:
                print msg
    #-----------------------------------------------------------------------------
    # Complex properties with personal descriptors
    #-----------------------------------------------------------------------------
    incident_energy = IncidentEnergy()
    #
    energy_bins = EnergyBins(incident_energy)
    #
    save_file_name = SaveFileName()
    #
    instr_name = InstrumentDependentProp('_instr_name')
    short_inst_name = InstrumentDependentProp('_short_instr_name')
    facility = InstrumentDependentProp('_facility')
    #
    van_rmm = VanadiumRMM()
    # Run descriptors
    sample_run  = RunDescriptor("SR_","Run ID (number) to convert to energy or list of the such run numbers")
    wb_run      = RunDescriptor("WB_","Run ID (number) for vanadium run used in detectors calibration")
    monovan_run = RunDescriptor("MV_","Run ID (number) for monochromatic vanadium used in absolute units normalization ")

    mask_run    = RunDescriptorDependent(sample_run,"MSK_"," Run used to find masks.\n If not explicitly set, sample_run is used""")
    wb_for_monovan_run = RunDescriptorDependent(wb_run,"MV_WB_"," white beam run used to calculate monovanadium integrals.\n If not explicitly set, white beam for processing run is used")
    # TODO: do something about it.  Second white is explicitly used in
    # diagnostics but not accessed at all
    second_white  = RunDescriptor("Second white beam currently unused in the  workflow despite being referred to in Diagnostics. Should it be used for Monovan Diagnostics?")
    #
    _tmp_run     = RunDescriptor("_TMP","Property used for storing intermediate run data during reduction")
    # property responsible for summing runs
    sum_runs = SumRuns(sample_run,log)
    #-----------------------------------------------------------------------------------
    def getDefaultParameterValue(self,par_name):
        """ method to get default parameter value, specified in IDF """
        return prop_helpers.get_default_parameter(self.instrument,par_name)
    @property
    def instrument(self):
        if self._pInstrument is None:
            raise KeyError("Attempt to use uninitialized property manager")
        else:
            return self._pInstrument
    #
    #-----------------------------------------------------------------------------------
    #TODO: do something about it
    @property
    def print_diag_results(self):
        """ property-sink used in diagnostics """
        return True
    @print_diag_results.setter
    def print_diag_results(self,value):
        pass
    #-----------------------------------------------------------------------------------
    # -----------------------------------------------------------------------------
    @property
    def log_to_mantid(self):
        """ Property specify if high level log should be printed to stdout or added to common Mantid log"""
        return self._log_to_mantid

    @log_to_mantid.setter
    def log_to_mantid(self,val):
        object.__setattr__(self,'_log_to_mantid',bool(val))
    # -----------------------------------------------------------------------------
    #-----------------------------------------------------------------------------------
    @property
    def psi(self):
        """ rotation angle (not available from IDF)"""
        return self._psi
    @psi.setter
    def psi(self,value):
        """set rotation angle (not available from IDF). This value will be saved into NXSpe file"""
        object.__setattr__(self,'_psi',value)
    # -----------------------------------------------------------------------------
    @property
    def motor_name(self):
        return self._motor_name
    @motor_name.setter
    def motor_name(self,val):
        object.__setattr__(self,'_motor_name',val)
    #
    @property
    def motor_offset(self):
        return self._motor_offset
    @motor_offset.setter
    def motor_offset(self,val):
        object.__setattr__(self,'_motor_offset',val)

    # -----------------------------------------------------------------------------
    # Service properties (used by class itself)
    #

    def _set_instrument_and_facility(self,Instrument,run_workspace=None):
        """ simple method used to obtain default instrument for testing """
        # TODO: implement advanced instrument setter, used in DirectEnergy
        # conversion

        if run_workspace:
            instrument = run_workspace.getInstrument()
            instr_name = instrument.getFullName()
            new_name,full_name,facility_ = prop_helpers.check_instrument_name(None,instr_name)
        else:
            if isinstance(Instrument,geometry._geometry.Instrument):
                instrument = Instrument
                instr_name = instrument.getFullName()
                try:
                    new_name,full_name,facility_ = prop_helpers.check_instrument_name(None,instr_name)
                except KeyError: # the instrument pointer is not found in any facility but we have it after all
                    new_name = instr_name
                    full_name = instr_name
                    facility_ = 'TEST'
                #end


            elif isinstance(Instrument,str): # instrument name defined
                new_name,full_name,facility_ = prop_helpers.check_instrument_name(None,Instrument)
                idf_dir = config.getString('instrumentDefinition.directory')
                idf_file = api.ExperimentInfo.getInstrumentFilename(full_name)
                tmp_ws_name = '__empty_' + full_name
                if not mtd.doesExist(tmp_ws_name):
                    LoadEmptyInstrument(Filename=idf_file,OutputWorkspace=tmp_ws_name)
                instrument = mtd[tmp_ws_name].getInstrument()
            else:
                raise TypeError(' neither correct instrument name nor instrument pointer provided as instrument parameter')
        #end if
        object.__setattr__(self,'_pInstrument',instrument)
        object.__setattr__(self,'_instr_name',full_name)
        object.__setattr__(self,'_facility',facility_)
        object.__setattr__(self,'_short_instr_name',new_name)






if __name__ == "__main__":
    pass


