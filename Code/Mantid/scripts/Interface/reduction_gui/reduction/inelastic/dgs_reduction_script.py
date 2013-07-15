"""
    Classes for each reduction step. Those are kept separately 
    from the the interface class so that the DgsReduction class could 
    be used independently of the interface implementation
"""
import xml.dom.minidom
import os
import time
import mantid
from reduction_gui.reduction.scripter import BaseReductionScripter

class DgsReductionScripter(BaseReductionScripter):
    """
        Organizes the set of reduction parameters that will be used to
        create a reduction script. Parameters are organized by groups that
        will each have their own UI representation.
    """
    TOPLEVEL_WORKFLOWALG = "DgsReduction"
    WIDTH_END = "".join([" " for i in range(len(TOPLEVEL_WORKFLOWALG))])
    WIDTH = WIDTH_END + " "
    
    def __init__(self, name="SEQ", facility="SNS"):
        super(DgsReductionScripter, self).__init__(name=name, facility=facility)
        
    def to_script(self, file_name=None):
        """
            Generate reduction script
            @param file_name: name of the file to write the script to
        """     
        script = "from mantid.simpleapi import *\n"
        script += "config['default.facility']=\"%s\"\n" % self.facility_name
        script += "\n"
        script +=  "%s(\n" % DgsReductionScripter.TOPLEVEL_WORKFLOWALG
        for item in self._observers:
            if item.state() is not None:
                for subitem in str(item.state()).split('\n'):
                    if len(subitem):
                        script += DgsReductionScripter.WIDTH + subitem + "\n"
        script += DgsReductionScripter.WIDTH_END + ")\n"
        
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
            if state is not None:
                try:
                    data_list = state.sample_file
                except:
                    pass
        
        data_files = data_list.split(',')
                            
        scripts = []
        output_dir=mantid.config['defaultsave.directory']
        
        if  not os.path.isdir(output_dir):
            raise RuntimeError("Default save path is not set")
        
        for data_file in data_files:
            script = "# DGS reduction script\n"
            script += "# Script automatically generated on %s\n\n" % time.ctime(time.time())
            
            script += "import mantid\n"
            script += "import os\n"
            script += "from mantid.simpleapi import *\n"        
            script += "config['default.facility']=\"%s\"\n" % self.facility_name

            
            script += "\n"
            script +=  "processed=%s(\n" % DgsReductionScripter.TOPLEVEL_WORKFLOWALG
            script+=DgsReductionScripter.WIDTH + 'SampleInputFile="'+data_file+'",\n'
            for item in self._observers:
                if item.state() is not None:
                    for subitem in str(item.state()).split('\n'):
                        if len(subitem) and subitem.find("SampleInputFile")==-1:
                            script += DgsReductionScripter.WIDTH + subitem + "\n"
            script += DgsReductionScripter.WIDTH_END + ")\n"
            
            script +="\n"
            script +="OutputFilename=os.path.join('"+output_dir+ "',processed[0].getInstrument().getName()+str(processed[0].getRunNumber())+'.nxs')\n"
            script +="SaveNexus(processed[0],OutputFilename)\n"
            

       
            scripts.append(script)
            
        return scripts
