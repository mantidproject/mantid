"""*WIKI* 

*WIKI*"""
import mantid.simpleapi as api
from mantid.api import *
from mantid.kernel import *
from reduction_workflow.find_data import find_data

class HFIRSANSReduction(PythonAlgorithm):

    def PyInit(self):
        #TODO: allow for multiple files to be summed 
        #TODO: allow for input workspace instead of file
        self.declareProperty(FileProperty("Filename", "",
                                          action=FileAction.Load, extensions=['xml']))
        self.declareProperty("ReductionProperties", "__sans_reduction_properties", 
                             validator=StringMandatoryValidator(),
                             doc="Property manager name for the reduction")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", 
                                                     direction = Direction.Output),
                             "Reduced workspace")
        self.declareProperty("OutputMessage", "", 
                             direction=Direction.Output, doc = "Output message")
        
    def _multiple_load(self, data_file, workspace, 
                       property_manager, property_manager_name):
        # Check whether we have a list of files that need merging
        #   Make sure we process a list of files written as a string
        def _load_data(filename, output_ws):
            if not property_manager.existsProperty("LoadAlgorithm"):
                raise RuntimeError, "SANS reduction not set up properly: missing load algorithm"
            p=property_manager.getProperty("LoadAlgorithm")
            alg=Algorithm.fromString(p.valueAsStr)
            alg.setProperty("Filename", filename)
            alg.setProperty("OutputWorkspace", output_ws)
            if alg.existsProperty("ReductionProperties"):
                alg.setProperty("ReductionProperties", property_manager_name)
            alg.execute()
            msg = "Loaded %s\n" % filename
            if alg.existsProperty("OutputMessage"):
                msg = alg.getProperty("OutputMessage").value
            return msg
            
        # Get instrument to use with FileFinder
        instrument = ''
        if property_manager.existsProperty("InstrumentName"):
            instrument = property_manager.getProperty("InstrumentName")

        output_str = ''
        if type(data_file)==str:
            data_file = find_data(data_file, instrument=instrument, allow_multiple=True)
        if type(data_file)==list:
            monitor = 0.0
            timer = 0.0 
            for i in range(len(data_file)):
                output_str += "Loaded %s\n" % data_file[i]
                if i==0:
                    output_str += _load_data(data_file[i], workspace)
                else:
                    output_str += _load_data(data_file[i], '__tmp_wksp')
                    api.Plus(LHSWorkspace=workspace,
                         RHSWorkspace='__tmp_wksp',
                         OutputWorkspace=workspace)
                    # Get the monitor and timer values
                    ws = AnalysisDataService.retrieve('__tmp_wksp')
                    monitor += ws.getRun().getProperty("monitor").value
                    timer += ws.getRun().getProperty("timer").value
            
            # Get the monitor and timer of the first file, which haven't yet
            # been added to the total
            ws = AnalysisDataService.retrieve(workspace)
            monitor += ws.getRun().getProperty("monitor").value
            timer += ws.getRun().getProperty("timer").value
                    
            # Update the timer and monitor
            ws.getRun().addProperty("monitor", monitor, True)
            ws.getRun().addProperty("timer", timer, True)
            
            if AnalysisDataService.doesExist('__tmp_wksp'):
                AnalysisDataService.remove('__tmp_wksp')              
        else:
            output_str += "Loaded %s\n" % data_file
            output_str += _load_data(data_file, workspace)
        return output_str
        
    def PyExec(self):
        filename = self.getProperty("Filename").value
        output_ws = self.getPropertyValue("OutputWorkspace")
        property_manager_name = self.getProperty("ReductionProperties").value
        property_manager = PropertyManagerDataService.retrieve(property_manager_name)
        
        property_list = [p.name for p in property_manager.getProperties()]
        print property_list
        
        output_msg = ""
        # Find the beam center
        if "SANSBeamFinderAlgorithm" in property_list:
            p=property_manager.getProperty("SANSBeamFinderAlgorithm")
            alg=Algorithm.fromString(p.valueAsStr)
            if alg.existsProperty("ReductionProperties"):
                alg.setProperty("ReductionProperties", property_manager_name)
            alg.execute()
            if alg.existsProperty("OutputMessage"):
                output_msg += alg.getProperty("OutputMessage").value+'\n'
        
        # Load the sample data
        if "LoadAlgorithm" not in property_list:
            raise RuntimeError, "HFIR SANS reduction not set up properly: missing load algorithm"
        p=property_manager.getProperty("LoadAlgorithm")
        alg=Algorithm.fromString(p.valueAsStr)
        alg.setProperty("Filename", filename)
        alg.setProperty("OutputWorkspace", output_ws)
        alg.setProperty("ReductionProperties", property_manager_name)
        alg.execute()
        output_msg += "Loaded %s\n" % filename
        if alg.existsProperty("OutputMessage"):
            output_msg += alg.getProperty("OutputMessage").value

        # Perform the main corrections on the sample data
        output_msg += self.process_data_file(output_ws)
        
        # Sample data transmission correction
        if "TransmissionAlgorithm" in property_list:
            p=property_manager.getProperty("TransmissionAlgorithm")
            alg=Algorithm.fromString(p.valueAsStr)
            alg.setProperty("InputWorkspace", output_ws)
            alg.setProperty("OutputWorkspace", '__'+output_ws+"_reduced")
            if alg.existsProperty("ReductionProperties"):
                alg.setProperty("ReductionProperties", property_manager_name)
            alg.execute()
            if alg.existsProperty("OutputMessage"):
                output_msg += alg.getProperty("OutputMessage").value+'\n'
            output_ws = '__'+output_ws+'_reduced'
        
        # Process background data
        if "BackgroundFiles" in property_list:
            background = property_manager.getProperty("BackgroundFiles").value
            background_ws = "__background_%s" % output_ws
            msg = self._multiple_load(background, background_ws, 
                                property_manager, property_manager_name)
            bck_msg = "Loaded background %s\n" % background
            bck_msg += msg
        
            # Process background like we processed the sample data
            bck_msg += self.process_data_file(background_ws)
            
            trans_beam_center_x = None
            trans_beam_center_y = None
            if "BckTransmissionBeamCenterAlgorithm" in property_list:
                # Execute the beam finding algorithm and set the beam
                # center for the transmission calculation
                p=property_manager.getProperty("BckTransmissionBeamCenterAlgorithm")
                alg=Algorithm.fromString(p.valueAsStr)
                if alg.existsProperty("ReductionProperties"):
                    alg.setProperty("ReductionProperties", property_manager_name)
                alg.execute()
                trans_beam_center_x = alg.getProperty("FoundBeamCenterX").value
                trans_beam_center_y = alg.getProperty("FoundBeamCenterY").value
            
            # Background transmission correction
            if "BckTransmissionAlgorithm" in property_list:
                p=property_manager.getProperty("BckTransmissionAlgorithm")
                alg=Algorithm.fromString(p.valueAsStr)
                alg.setProperty("InputWorkspace", background_ws)
                alg.setProperty("OutputWorkspace", '__'+background_ws+"_reduced")
                
                if alg.existsProperty("BeamCenterX") \
                    and alg.existsProperty("BeamCenterY") \
                    and trans_beam_center_x is not None \
                    and trans_beam_center_y is not None:
                    alg.setProperty("BeamCenterX", trans_beam_center_x)
                    alg.setProperty("BeamCenterY", trans_beam_center_y)
                    
                if alg.existsProperty("ReductionProperties"):
                    alg.setProperty("ReductionProperties", property_manager_name)
                alg.execute()
                if alg.existsProperty("OutputMessage"):
                    output_msg += alg.getProperty("OutputMessage").value+'\n'
                background_ws = '__'+background_ws+'_reduced'
        
            # Subtract background
            api.Minus(LHSWorkspace=output_ws,
                         RHSWorkspace=background_ws,
                         OutputWorkspace=output_ws)
            
            bck_msg = bck_msg.replace('\n','\n   |')
            output_msg += "Background subtracted [%s]%s\n" % (background_ws, bck_msg)
        
        # Absolute scale correction
        
        # Geometry correction
        if "GeometryAlgorithm" in property_list:
            p=property_manager.getProperty("GeometryAlgorithm")
            alg=Algorithm.fromString(p.valueAsStr)
            alg.setProperty("InputWorkspace", background_ws)
            alg.setProperty("OutputWorkspace", background_ws)
            if alg.existsProperty("ReductionProperties"):
                alg.setProperty("ReductionProperties", property_manager_name)
            alg.execute()
            if alg.existsProperty("OutputMessage"):
                output_msg += alg.getProperty("OutputMessage").value+'\n'
        
        # Compute I(q)
        if "IQAlgorithm" in property_list:
            iq_output = self.getPropertyValue("OutputWorkspace")
            iq_output = iq_output+'_Iq'
            p=property_manager.getProperty("IQAlgorithm")
            alg=Algorithm.fromString(p.valueAsStr)
            alg.setProperty("InputWorkspace", output_ws)
            alg.setProperty("OutputWorkspace", iq_output)
            alg.setProperty("ReductionProperties", property_manager_name)
            alg.execute()
            if alg.existsProperty("OutputMessage"):
                output_msg += alg.getProperty("OutputMessage").value            

        # Compute I(qx,qy)
        
        # Save data
        
    
        self.setPropertyValue("OutputWorkspace", output_ws)
        self.setProperty("OutputMessage", output_msg)
        
    def process_data_file(self, workspace):
        output_msg = ""
        property_manager_name = self.getProperty("ReductionProperties").value
        property_manager = PropertyManagerDataService.retrieve(property_manager_name)
        property_list = [p.name for p in property_manager.getProperties()]

        # Dark current subtraction
        if "DarkCurrentAlgorithm" in property_list:
            p=property_manager.getProperty("DarkCurrentAlgorithm")
            alg=Algorithm.fromString(p.valueAsStr)
            alg.setProperty("InputWorkspace", workspace)
            alg.setProperty("OutputWorkspace", workspace)
            if alg.existsProperty("ReductionProperties"):
                alg.setProperty("ReductionProperties", property_manager_name)
            alg.execute()
            if alg.existsProperty("OutputMessage"):
                output_msg += alg.getProperty("OutputMessage").value+'\n'

        # Normalize
        if "NormaliseAlgorithm" in property_list:
            p=property_manager.getProperty("NormaliseAlgorithm")
            alg=Algorithm.fromString(p.valueAsStr)
            alg.setProperty("InputWorkspace", workspace)
            alg.setProperty("OutputWorkspace", workspace)
            if alg.existsProperty("ReductionProperties"):
                alg.setProperty("ReductionProperties", property_manager_name)
            alg.execute()
            if alg.existsProperty("OutputMessage"):
                output_msg += alg.getProperty("OutputMessage").value+'\n'
        
        # Mask
        
        # Solid angle correction
        if "SANSSolidAngleCorrection" in property_list:
            p=property_manager.getProperty("SANSSolidAngleCorrection")
            alg=Algorithm.fromString(p.valueAsStr)
            alg.setProperty("InputWorkspace", workspace)
            alg.setProperty("OutputWorkspace", workspace)
            if alg.existsProperty("ReductionProperties"):
                alg.setProperty("ReductionProperties", property_manager_name)
            alg.execute()
            if alg.existsProperty("OutputMessage"):
                output_msg += alg.getProperty("OutputMessage").value+'\n'
        
        # Sensitivity correction
        if "SensitivityAlgorithm" in property_list:
            p=property_manager.getProperty("SensitivityAlgorithm")
            alg=Algorithm.fromString(p.valueAsStr)
            alg.setProperty("InputWorkspace", workspace)
            alg.setProperty("OutputWorkspace", workspace)
            if alg.existsProperty("ReductionProperties"):
                alg.setProperty("ReductionProperties", property_manager_name)
            alg.execute()
            if alg.existsProperty("OutputMessage"):
                output_msg += alg.getProperty("OutputMessage").value+'\n'

        return output_msg
        

#############################################################################################

registerAlgorithm(HFIRSANSReduction)
