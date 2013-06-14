"""*WIKI* 

This program is a simple Reflectometer NeXus viewer. The various metadata will be displayed:
 - Run title
 - Run start
 - Lambda requested
 - tthd [degree]
 - thi [degree]
 - s1h [mm]
 - s2h [mm]
 - s1w [mm]
 - s2w [mm]

Input examples:
 '''74099''' will display the metadata of run 74099
 '''74099,74052''' will display the metadata of runs 74099 and 74052
 '''74099-74102''' will display the metadata of runs 74099, 74100, 74101 and 74102
 '''74099-74101, 74054''' will display the metadata of runs 74099, 74100, 74101 and 74054


''Example:''

'''74099-74102,74090'''


<nowiki>*********</nowiki> Working with run: 74099 <nowiki>*********</nowiki>
 Run title: direct beam Al2O3_No3Rep0
 Run start: 2012-04-14T22:13:41
 Lambda requested: 8.39999961853 Angstrom
 tthd: -4.0001 degree
 thi: -4.0003 degree
 ths: -3.9998 degree
 s1h: 1.2650 millimetre
 s2h: 0.4240 millimetre
 s1w: 7.9990 millimetre
 s2w: 8.0010 millimetre

<nowiki>*********</nowiki> Working with run: 74100<nowiki> *********</nowiki>
 Run title: direct beam Al2O3_No4Rep0
 Run start: 2012-04-14T22:20:49
 Lambda requested: 6.17000007629 Angstrom
 tthd: -4.0001 degree
 thi: -4.0003 degree
 ths: -3.9998 degree
 s1h: 0.4260 millimetre
 s2h: 0.4240 millimetre
 s1w: 7.9990 millimetre
 s2w: 8.0010 millimetre

<nowiki>*********</nowiki> Working with run: 74101<nowiki> *********</nowiki>
 Run title: direct beam Al2O3_No5Rep0
 Run start: 2012-04-14T22:25:46
 Lambda requested: 3.8900001049 Angstrom
 tthd: -4.0001 degree
 thi: -4.0003 degree
 ths: -3.9998 degree
 s1h: 0.3020 millimetre
 s2h: 0.3000 millimetre
 s1w: 7.9990 millimetre
 s2w: 8.0010 millimetre

<nowiki>*********</nowiki> Working with run: 74102 <nowiki>*********</nowiki>
 Run title: NONE
 Run start: 2012-04-14T22:29:07
 Lambda requested: 3.75 Angstrom
 tthd: -4.0001 degree
 thi: -4.0003 degree
 ths: -3.9998 degree
 s1h: 0.2610 millimetre
 s2h: 0.2590 millimetre
 s1w: 7.9990 millimetre
 s2w: 8.0010 millimetre

<nowiki>*********</nowiki> Working with run: 74090 <nowiki>*********</nowiki>
 Run title: _No3Rep0
 Run start: 2012-04-14T21:03:00
 Lambda requested: 8.39999961853 Angstrom
 tthd: -2.8002 degree
 thi: -4.0003 degree
 ths: -3.4000 degree
 s1h: 1.2610 millimetre
 s2h: 0.4130 millimetre
 s1w: 19.9980 millimetre
 s2w: 19.9960 millimetre

*WIKI*"""

#from MantidFramework import *
#from mantid.api import PythonAlgorithm, AlgorithmFactory
from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *
#import mantid
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
        
        overlap_validator = CompositeValidator()
        overlap_validator.add(FloatBoundedValidator(lower=0.0))
        overlap_validator.add(FloatMandatoryValidator())
        
        self.declareProperty(name="RunNumbers", defaultValue="",
#                                 validator=overlap_validator,
                                 doc="List of run numbers to process")
        self.setWikiSummary("""Liquids Reflectometer (REFL) NeXus viewer
This routine will display some of the metadata defined by the IS
for a given run or set of runs.""") 
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
#        from mantidsimple import mtd    

        bDebug = True
        if bDebug:
            print '====== Running in mode DEBUGGING ======='

        run_numbers = self.getPropertyValue("RunNumbers")
        if bDebug:
            print 'run_numbers (before getSequenceRuns): ' 
            print str(run_numbers)
            print
#        run_numbers = wks_utility.getSequenceRuns(run_numbers)
        if bDebug:
            print 'run_numbers (after getSequenceRuns): ' 
            print str(run_numbers)
            print

#        for _run in run_numbers:
        
        #make sure we are working with integer
#        _run = int(_run)
        _run = int(run_numbers)
        
        print '********* Working with run: ' + str(_run) + ' *********'

        #Pick a good workspace name
        ws_name = "refl%d" % _run
        ws_event_data = ws_name + "_evt"  

        try:
            data_file = FileFinder.findRuns("REF_L%d" % _run)[0]
            if bDebug:
                print 'DEBUG: full file name is ' + data_file
        except RuntimeError:
            msg = "RefLReduction: could not find run %d\n" % _run
            msg += "Add your data folder to your User Data Directories in the File menu"
            if bDebug:
                print 'DEBUG: file name could not be found !'
            raise RuntimeError(msg)
                
#       if not mtd.doesExist(ws_event_data):
        ws_event_data = LoadEventNexus(Filename=data_file)

        #retrieve list of metadata
        mt_run = ws_event_data.getRun()

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
        _cal = (float(tthd_value) - float(thi_value)) / 2.
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
        s1h_value = wks_utility.getS1h(ws_event_data)
        _line = ' s1h: {0:.4f} mm'.format(s1h_value)
#        _line += ' ' + s1h_units
        print _line
            
        #s2h
        s2h_value = wks_utility.getS2h(ws_event_data)
        _line = ' s2h: {0:.4f} mm'.format(s2h_value)
#        _line += ' ' + s2h_units
        print _line
            
        #s1w
        s1w_value = wks_utility.getS1w(ws_event_data)
        _line = ' s1w: {0:.4f} mm'.format(s1w_value)
#        _line += ' ' + s1w_units
        print _line
            
        #s2w
        s2w_value = wks_utility.getS2w(ws_event_data)
        _line = ' s2w: {0:.4f} mm'.format(s2w_value)
#        _line += ' ' + s2w_units
        print _line

        print '********************************'
        print 
        
#Register Algorithm with Mantid
registerAlgorithm(RefLview())
