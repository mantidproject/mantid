"""*WIKI* 

    HFIR SANS reduction workflow
    
*WIKI*"""
import mantid.simpleapi as api
from mantid.api import *
from mantid.kernel import *
from reduction_workflow.find_data import find_data
import os
import BaseSANSReduction

class HFIRSANSReduction(BaseSANSReduction.SANSReduction):

    def category(self):
        return "Workflow\\SANS;PythonAlgorithms"

    def name(self):
        return "HFIRSANSReduction"
    
    def PyInit(self):
        self._py_init()
        
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
            instrument = property_manager.getProperty("InstrumentName").value

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
        self._py_exec()
#############################################################################################

registerAlgorithm(HFIRSANSReduction)
