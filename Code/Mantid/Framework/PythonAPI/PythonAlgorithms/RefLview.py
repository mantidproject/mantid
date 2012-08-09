"""*WIKI* 

Liquids Reflectometer (REFL) NeXus viewer
This routine will display some of the metadata defined by the IS
for a given run or set of runs. 

*WIKI*"""

from MantidFramework import *
from mantidsimple import *
from numpy import zeros, shape, arange
import math
import sfCalculator

class RefLview(PythonAlgorithm):

    def category(self):
        return "Reflectometry\\SNS"

    def name(self):
        return "RefLview"
    
    def version(self):
        return 1

    def PyInit(self):
        self.declareListProperty("RunNumbers", [0], 
                                 Validator=ArrayBoundedValidator(Lower=0),
                                 Description="List of run numbers to process")

    def PyExec(self):   
        
        import os
        import numpy
        import math
        from reduction.instruments.reflectometer import wks_utility
        
        from mantid import mtd
        #remove all previous workspaces
        list_mt = mtd.getObjectNames()
        for _mt in list_mt:
            if _mt.find('_scaled') != -1:
                mtd.remove(_mt)
            if _mt.find('_reflectivity') != -1:
                mtd.remove(_mt)
        from mantidsimple import mtd    

        bDebug = False
        if bDebug:
            print '====== Running in mode DEBUGGING ======='

        run_numbers = self.getProperty("RunNumbers")
        if bDebug:
            print 'run_numbers (before getSequenceRuns): ' 
            print str(run_numbers)
            print
        run_numbers = wks_utility.getSequenceRuns(run_numbers)
        if bDebug:
            print 'run_numbers (after getSequenceRuns): ' 
            print str(run_numbers)
            print
            
        for _run in run_numbers:
        
            #make sure we are working with integer
            _run = int(_run)
        
            print '********* Working with run: ' + str(_run) + ' *********'

            #Pick a good workspace name
            ws_name = "refl%d" % _run
            ws_event_data = ws_name+"_evt"  

            try:
                data_file = FileFinder.findRuns("REF_L%d" %_run)[0]
                if bDebug:
                    print 'DEBUG: full file name is ' + data_file
            except RuntimeError:
                msg = "RefLReduction: could not find run %d\n" % _run
                msg += "Add your data folder to your User Data Directories in the File menu"
                if bDebug:
                    print 'DEBUG: file name could not be found !'
                raise RuntimeError(msg)
                
            if not mtd.workspaceExists(ws_event_data):
                LoadEventNexus(Filename=data_file, 
                               OutputWorkspace=ws_event_data)

            #retrieve list of metadata
            mt_run = mtd[ws_event_data].getRun()

            #run_title
            run_title = mt_run.getProperty('run_title').value
            _line = ' Run title: ' + run_title
            print _line
            
            #run_start
            run_start = mt_run.getProperty('run_start').value
            _line = ' Run start: ' + run_start
            print _line
            
            #duration
            duration_value = mt_run.getProperty('duration').value
            duration_units = mt_run.getProperty('duration').units
            _line = ' Duration: {0:.2f}'.format(duration_value)
            _line += ' ' + duration_units
            print _line
            
            #Lambda Requested
            lambda_request_value = mt_run.getProperty('LambdaRequest').value[0]
            lambda_request_units = mt_run.getProperty('LambdaRequest').units
            _line = ' Lambda requested: {0:.2f}'.format(lambda_request_value)
            _line += ' ' + lambda_request_units
            print _line
            
            #tthd
            tthd_value = mt_run.getProperty('tthd').value[0]
            tthd_units = mt_run.getProperty('tthd').units
            _line = ' tthd: {0:.4f}'.format(tthd_value)
            _line += ' ' + tthd_units
            print _line
           
            #thi
            thi_value = mt_run.getProperty('thi').value[0]
            thi_units = mt_run.getProperty('thi').units
            _line = ' thi: {0:.4f}'.format(thi_value)
            _line += ' ' + thi_units
            print _line
            
            #(tthd-thi)/2
            _cal = (float(tthd_value)-float(thi_value))/2.
            _line = ' (tthd-thi)/2: {0:.2f}'.format(_cal)
            _line += ' ' + thi_units
            print _line
            
            #ths
            ths_value = mt_run.getProperty('ths').value[0]
            ths_units = mt_run.getProperty('ths').units
            _line = ' ths: {0:.4f}'.format(ths_value)
            _line += ' ' + ths_units
            print _line
            
            #s1h
            s1h_value, s1h_units = wks_utility.getS1h(mtd[ws_event_data])
            _line = ' s1h: {0:.4f}'.format(s1h_value)
            _line += ' ' + s1h_units
            print _line
            
            #s2h
            s2h_value, s2h_units = wks_utility.getS2h(mtd[ws_event_data])
            _line = ' s2h: {0:.4f}'.format(s2h_value)
            _line += ' ' + s2h_units
            print _line
            
            #s1w
            s1w_value, s1w_units = wks_utility.getS1w(mtd[ws_event_data])
            _line = ' s1w: {0:.4f}'.format(s1w_value)
            _line += ' ' + s1w_units
            print _line
            
            #s2w
            s2w_value, s2w_units = wks_utility.getS2w(mtd[ws_event_data])
            _line = ' s2w: {0:.4f}'.format(s2w_value)
            _line += ' ' + s2w_units
            print _line

            print '********************************'
            print 
        
mtd.registerPyAlgorithm(RefLview())