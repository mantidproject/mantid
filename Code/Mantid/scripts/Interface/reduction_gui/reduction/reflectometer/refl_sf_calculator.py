"""
    This class holds all the necessary information to create a reduction script.
    This is a fake version of the Reducer for testing purposes.
"""
import time
from reduction_gui.reduction.scripter import BaseReductionScripter
# Check whether Mantid is available
try:
    from MantidFramework import *
    mtd.initialise(False)
    HAS_MANTID = True
except:
    HAS_MANTID = False  

class REFLSFCalculatorScripter(BaseReductionScripter):
    """
        Organizes the set of reduction parameters that will be used to
        create a reduction script. Parameters are organized by groups that
        will each have their own UI representation.
    """
    
    def __init__(self, name="REFL"):
        super(REFLSFCalculatorScripter, self).__init__(name=name)        
    
    def create_script(self, script_part2):
        """
        This creates the special script for the sfCalculator algorithm
        """
        algo = 'sfCalculator.calculate'
        
        script_split = script_part2.split('\n')
        new_script = ''
        
        run_number = []
        attenuator = []
        
        peak_from = []
        peak_to = []
        back_from = []
        back_to = []
        
        tof_range = ['','']
        incident_medium = ''
        
        for _line in script_split:
            if _line != '':
                _line_split = _line.split(':')
                _arg = _line_split[0]
                _val = _line_split[1]
                
                if _arg == 'Run number':
                    run_number.append(_val)
                    continue
                
                if _arg == 'TOF from':
                    if tof_range[0] != '':
                        tof_range[0] = _val
                    continue
                
                if _arg == 'TOF to':
                    if tof_range[1] != '':
                        tof_range[1] = _val
                    continue
                        
                if _arg == 'Incident medium':
                    if incident_medium != '':
                        incident_medium = _val
                    continue
                    
                if _arg == 'Number of attenuator':
                    attenuator.append(_val)
                    continue
                
                if _arg == 'Peak from pixel':
                    peak_from.append(_val)
                    continue
                
                if _arg == 'Peak to pixel':
                    peak_from.append(_val)
                    continue
                
                if _arg == 'Back from pixel':
                    peak_from.append(_val)
                    continue
                
                if _arg == 'Back to pixel':
                    peak_from.append(_val)
                    continue
            
        run_attenuator = []    
        for (run,att) in zip(run_number, attenuator):
            run_attenuator.append(run + ':' + attenuator)
            
        list_peak_back = []
        for (_peak_from, _peak_to, _back_from, _back_to) in zip(peak_from, peak_to, back_from, back_to):
            list_peak_back.append([_peak_from,_peak_to,_back_from,_back_to])
            
        
        
            
            
                



        
    
    
    
    
        return new_script
    
    def to_script(self, file_name=None):
        """
            Spits out the text of a reduction script with the current state.
            @param file_name: name of the file to write the script to
        """
        script = "# %s scaling factor calculation script\n" % self.instrument_name
        script += "# Script automatically generated on %s\n\n" % time.ctime(time.time())
        
        script += "import os\n"
        script += "from MantidFramework import *\n"
        script += "mtd.initialise(False)\n"
        script += "from mantidsimple import *\n\n"
                
        script += "REF_RED_OUTPUT_MESSAGE = ''\n\n"

        script_part2 = ''
        for item in self._observers:
            if item.state() is not None:
                script_part2 += str(item.state())

        script += self.create_script(script_part2)

        if file_name is not None:
            f = open(file_name, 'w')
            f.write(script)
            f.close()
        
        return script
   
    def apply(self):
        """
            Apply the reduction process
        """        
        if HAS_MANTID:
            script = self.to_script(None)

            try:
                t0 = time.time()
                exec script
                delta_t = time.time()-t0
                print REF_RED_OUTPUT_MESSAGE
                print "SF calculation time: %5.2g sec" % delta_t
                # Update scripter
                for item in self._observers:
                    if item.state() is not None:
                        item.state().update()
            except:
                # Update scripter [Duplicated code because we can't use 'finally' on python 2.4]
                for item in self._observers:
                    if item.state() is not None:
                        # Things might be broken, so update what we can
                        try:
                            item.state().update()
                        except:
                            pass
                raise RuntimeError, sys.exc_value
        else:
            raise RuntimeError, "SF calculation could not be executed: Mantid could not be imported"
        

    