"""*WIKI* 

*WIKI*"""
from mantid.api import *
from mantid.kernel import *

class HFIRSANSReduction(PythonAlgorithm):

    def PyInit(self):
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
        
    def PyExec(self):
        filename = self.getProperty("Filename").value
        output_ws = self.getPropertyValue("OutputWorkspace")
        property_manager_name = self.getProperty("ReductionProperties").value
        property_manager = PropertyManagerDataService.retrieve(property_manager_name)
        
        property_list = [p.name for p in property_manager.getProperties()]

        # Find the beam center
        self.find_beam_center(property_manager)
        
        # Load the sample data
        if "LoadAlgorithm" not in property_list:
            raise RuntimeError, "HFIR SANS reduction not set up properly: missing load algorithm"
        p=property_manager.getProperty("LoadAlgorithm")
        alg=Algorithm.fromString(p.valueAsStr)
        alg.setProperty("Filename", filename)
        alg.setProperty("OutputWorkspace", output_ws)
        alg.setProperty("ReductionProperties", property_manager_name)
        alg.execute()
        output_msg = "Loaded %s\n" % filename
        if alg.existsProperty("OutputMessage"):
            output_msg += alg.getProperty("OutputMessage").value

        # Perform the main corrections on the sample data
        output_msg += self.process_data_file(output_ws)
        
        # Sample data transmission correction
        
        # Process background data
        
        # Background transmission correction
        
        # Subtract background
        
        # Absolute scale correction
        
        # Compute I(q)
        
        # Compute I(qx,qy)
        
        # Save data
        
    
        self.setPropertyValue("OutputWorkspace", output_ws)
        self.setProperty("OutputMessage", output_msg)
        
    def find_beam_center(self, property_manager):
        pass
    
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
        return output_msg
        

#############################################################################################

registerAlgorithm(HFIRSANSReduction)
