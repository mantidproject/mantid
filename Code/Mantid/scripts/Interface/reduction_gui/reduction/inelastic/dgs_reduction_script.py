#pylint: disable=invalid-name
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



        for item in self._observers:
            state = item.state()
            if state is not None:
                try:
                    data_list = state.sample_file
                except:
                    pass


        out_dir_line=""
        script = "# DGS reduction script\n"
        script += "# Script automatically generated on %s\n\n" % time.ctime(time.time())

        script += "import mantid\n"
        script += "import os\n"
        script += "from mantid.simpleapi import *\n"
        script += "config['default.facility']=\"%s\"\n" % self.facility_name
        script += "\n"
        script += 'DGS_input_data=Load("'+data_list+'")\n'
        script +=  "DGS_output_data=%s(\n" % DgsReductionScripter.TOPLEVEL_WORKFLOWALG
        script += DgsReductionScripter.WIDTH + 'SampleInputWorkspace=DGS_input_data,\n'
        for item in self._observers:
            if item.state() is not None:
                for subitem in str(item.state()).split('\n'):
                    if len(subitem) and subitem.find("SampleInputFile")==-1 and subitem.find("OutputDirectory")==-1:
                        script += DgsReductionScripter.WIDTH + subitem + "\n"
                    elif len(subitem) and subitem.find("OutputDirectory")!=-1:
                        out_dir_line=subitem.strip(',')+"\n"
        script += DgsReductionScripter.WIDTH_END + ")\n"
        script += "\n"

        if len(out_dir_line)==0:
            output_dir=mantid.config['defaultsave.directory']
            out_dir_line='OutputDirectory="%s"\n'%output_dir

        script += out_dir_line

        filenames=self.filenameParser(data_list)
        if len(filenames)==1:
            script += "OutputFilename=os.path.join(OutputDirectory,DGS_output_data[0].getInstrument().getName()+str(DGS_output_data[0].getRunNumber())+'.nxs')\n"
            script += 'SaveNexus(DGS_output_data[0],OutputFilename)\n'
        else:
            script += "for i in range("+str(len(filenames))+"):\n"
            script += DgsReductionScripter.WIDTH+"OutputFilename=os.path.join(OutputDirectory,DGS_output_data[0][i].getInstrument().getName()+str(DGS_output_data[0][i].getRunNumber())+'.nxs')\n"
            script += DgsReductionScripter.WIDTH+'SaveNexus(DGS_output_data[0][i],OutputFilename)\n'


        if file_name is not None:
            f = open(file_name, 'w')
            f.write(script)
            f.close()

        return script

    def to_live_script(self):
        """
            Generate the script for live data reduction
        """
        # Need to construct Dgs call slightly differently: no line breaks & extract output workspace
        options = ""
        for item in self._observers:
            if item.state() is not None:
                for subitem in str(item.state()).split('\n'):
                    if len(subitem):
                        if 'OutputWorkspace' in subitem:
                            output_workspace = subitem
                            options += "OutputWorkspace=output,"
                        else:
                            options += subitem

        if '_spe' in output_workspace:
            output_workspace_name = 'live_spe'
            output_workspace = 'OutputWorkspace="live_spe"'
        else:
            output_workspace_name = output_workspace.split('"')[1]

        script = """StartLiveData(UpdateEvery='10',Instrument='"""
        script += self.instrument_name
        script += """',ProcessingScript='"""
        script +=  DgsReductionScripter.TOPLEVEL_WORKFLOWALG + '('
        script += options
        script += ")"

        script += """',PreserveEvents=True,RunTransitionBehavior='Stop',"""
        script += output_workspace
        script += ")\n"

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

        data_files = self.filenameParser(data_list)
        scripts = []

        out_dir_line=""
        for data_file in data_files:
            script = "# DGS reduction script\n"
            script += "# Script automatically generated on %s\n\n" % time.ctime(time.time())

            script += "import mantid\n"
            script += "import os\n"
            script += "from mantid.simpleapi import *\n"
            script += "config['default.facility']=\"%s\"\n" % self.facility_name


            script += "\n"

            if isinstance(data_file,list):
                data_file='+'.join(data_file)
            script += 'DGS_input_data=Load("'+data_file+'")\n'
            script +=  "DGS_output_data=%s(\n" % DgsReductionScripter.TOPLEVEL_WORKFLOWALG
            script += DgsReductionScripter.WIDTH + 'SampleInputWorkspace=DGS_input_data,\n'
            for item in self._observers:
                if item.state() is not None:
                    for subitem in str(item.state()).split('\n'):
                        if len(subitem) and subitem.find("SampleInputFile")==-1 and subitem.find("OutputDirectory")==-1:
                            script += DgsReductionScripter.WIDTH + subitem + "\n"
                        elif len(subitem) and subitem.find("OutputDirectory")!=-1:
                            out_dir_line=subitem.strip(',')+"\n"
            script += DgsReductionScripter.WIDTH_END + ")\n"

            script +="\n"
            if len(out_dir_line)==0:
                output_dir=mantid.config['defaultsave.directory']
                out_dir_line='OutputDirectory="%s"\n'%output_dir
            script += out_dir_line

            script +="OutputFilename=os.path.join(OutputDirectory,DGS_output_data[0].getInstrument().getName()+str(DGS_output_data[0].getRunNumber())+'.nxs')\n"
            script +="SaveNexus(DGS_output_data[0],OutputFilename)\n"

            scripts.append(script)

        return scripts

    def filenameParser(self, filename):
        alg = mantid.api.AlgorithmManager.createUnmanaged("Load")
        alg.initialize()
        alg.setPropertyValue('Filename',str(filename))
        fnames=alg.getProperty('Filename').value
        if not isinstance(fnames,list):
            fnames=[fnames] #make a list with one element
        return fnames


