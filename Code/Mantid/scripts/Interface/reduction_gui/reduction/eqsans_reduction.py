"""
    This class holds all the necessary information to create a reduction script.
    This is a fake version of the Reducer for testing purposes.
"""
import time
import os
from scripter import BaseReductionScripter

HAS_MANTID = False
try:
    import mantidplot
    HAS_MANTID = True
except:
    pass

class EQSANSReductionScripter(BaseReductionScripter):
    """
        Organizes the set of reduction parameters that will be used to
        create a reduction script. Parameters are organized by groups that
        will each have their own UI representation.
    """
    
    def __init__(self, name="EQSANS", settings=None):
        super(EQSANSReductionScripter, self).__init__(name=name)
        self._settings = settings        
    
    def to_script(self, file_name=None):
        """
            Spits out the text of a reduction script with the current state.
            @param file_name: name of the file to write the script to
        """
        script = "# EQSANS reduction script\n"
        script += "# Script automatically generated on %s\n\n" % time.ctime(time.time())
        
        if self._settings.api2:
            script += "import mantid\n"
            script += "from mantid.simpleapi import *\n"
            script += "from reduction_workflow.instruments.sans.sns_command_interface import *\n"
            script += "config = ConfigService.Instance()\n"
            script += "config['instrumentName']='EQSANS'\n"
        else:
            script += "from MantidFramework import *\n"
            script += "mtd.initialise(False)\n"
            script += "from reduction.instruments.sans.sns_command_interface import *\n"
            script += "mtd.settings['default.instrument'] = 'EQSANS'\n"
        
        script += "\n"
        
        for item in self._observers:
            if item.state() is not None:
                script += str(item.state())
        
        xml_process = "None"
        if file_name is None:
            xml_process = os.path.join(self._output_directory, "EQSANS_process.xml")
            xml_process = os.path.normpath(xml_process)
            self.to_xml(xml_process)
            
        if self._settings.api2:
            script += "SaveIq(process=%r)\n" % xml_process
        else:
            script += "SaveIqAscii(process=%r)\n" % xml_process

        script += "Reduce()\n"

        if file_name is not None:
            f = open(file_name, 'w')
            f.write(script)
            f.close()
        
        return script
    
    def to_batch(self):
        """
        """
        data_files = []
        data_options = None
        for item in self._observers:
            state = item.state()
            print state.__class__.__name__
            if state is not None and state.__class__.__name__=="DataSets":
                data_options = state
        
        if data_options is None or data_options.separate_jobs is False:
            return [self.to_script()]
        
        data_files = data_options.get_data_file_list()
                    
        scripts = []
        for data_file in data_files:
            script = "# EQSANS reduction script\n"
            script += "# Script automatically generated on %s\n\n" % time.ctime(time.time())
            
            if self._settings.api2:
                script += "import mantid\n"
                script += "from mantid.simpleapi import *\n"
                script += "from reduction_workflow.instruments.sans.sns_command_interface import *\n"
                script += "config = ConfigService.Instance()\n"
                script += "config['instrumentName']='EQSANS'\n"
            else:
                script += "from MantidFramework import *\n"
                script += "mtd.initialise(False)\n"
                script += "from reduction.instruments.sans.sns_command_interface import *\n"
                script += "mtd.settings['default.instrument'] = 'EQSANS'\n"
            
            script += "\n"
            
            for item in self._observers:
                if item.state() is not None:
                    if state.__class__.__name__=="SampleData":
                        script += item.state().to_script(data_file=data_file)
                    else:
                        script += str(item.state())
            
            xml_process = "None"
            
            if self._settings.api2:
                script += "SaveIq(process=%r)\n" % xml_process
            else:
                script += "SaveIqAscii(process=%r)\n" % xml_process
    
            script += "Reduce()\n"            
            scripts.append(script)
            
        return scripts
        
    def set_options(self):
        """
            Set up the reduction options, without executing
        """
        if HAS_MANTID:
            self.update()
            table_ws = "__patch_options"
            script = "# Reduction script\n"
            script += "# Script automatically generated on %s\n\n" % time.ctime(time.time())
            
            if self._settings.api2:
                script += "import mantid\n"
                script += "from mantid.simpleapi import *\n"
                script += "\n"
                script += "if AnalysisDataService.doesExist('%s'):\n" % table_ws
                script += "   AnalysisDataService.remove('%s')\n\n" % table_ws
            else:
                script += "from MantidFramework import *\n"
                script += "mtd.initialise(False)\n"
                script += "\n"
                script += "if mtd.workspaceExists('%s'):\n" % table_ws
                script += "   mtd.deleteWorkspace('%s')\n\n" % table_ws
            
            script += "SetupEQSANSReduction(\n"
            for item in self._observers:
                if item.state() is not None:
                    if hasattr(item.state(), "options"):
                        script += item.state().options()

            script += "ReductionProperties='%s')" % table_ws            
            mantidplot.runPythonScript(script, True)
            return table_ws
        else:
            raise RuntimeError, "Reduction could not be executed: Mantid could not be imported"

        

    