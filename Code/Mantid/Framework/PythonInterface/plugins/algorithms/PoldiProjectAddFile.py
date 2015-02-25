#pylint: disable=no-init
from mantid.api import *
from mantid.kernel import Direction
from os.path import split, splitext

import re



class PoldiProjectAddFile(PythonAlgorithm):

    def category(self):
        """ Mantid required
        """
        return "SINQ\\Poldi\\Obsolete"

    def name(self):
        """ Mantid required
        """
        return "PoldiProjectAddDir"

    def summary(self):
        return "Add all the .hdf files from the given directory to the queue for automatic processing."

    def PyInit(self):
        """ Mantid required
        """

        self.declareProperty(FileProperty(name="File",defaultValue="",action=FileAction.Load), "Poldi data file")

        self.declareProperty(ITableWorkspaceProperty(name="OutputWorkspace", defaultValue="PoldiAnalysis", direction=Direction.Output), "Poldi analysis main worksheet")



    def path_leaf(path):
        head, tail = ntpath.split(path)
        return tail


    def interpreteName(self, name):
        patern="(.*[ a-zA-Z]*/*\*)*poldi(?P<year>[0-9]*)n(?P<numero>[0-9]*)"
        regex  = re.match(patern, name, re.M|re.I)
        year   = int(regex.group("year"))
        numero = int(regex.group("numero"))
        return (year, numero)






    def PyExec(self):
        """ Mantid required
        """
        self.log().debug('Poldi Data Analysis ---- start')
        sample_info_ws = None

        try:
            sample_info_ws_name = self.getProperty("OutputWorkspace").valueAsStr
            if sample_info_ws_name == "":
                sample_info_ws_name = "PoldiAnalysis"
            self.log().debug('Poldi Data Analysis ---- %s'%(sample_info_ws_name))
            sample_info_ws = mtd[sample_info_ws_name]
            self.log().debug('                    ---- workspace loaded')
        except:
            self.log().debug('                    ---- workspace created')
            sample_info_ws = WorkspaceFactory.createTable()
            sample_info_ws.addColumn("str","spl Name")
            sample_info_ws.addColumn("int","year")
            sample_info_ws.addColumn("int","number")
            sample_info_ws.addColumn("str","data file")
            sample_info_ws.addColumn("str","spl log")
            sample_info_ws.addColumn("str","spl corr")
            sample_info_ws.addColumn("str","spl dead wires")
            sample_info_ws.addColumn("str","spl peak")

        dataFile = self.getProperty("File").value

        self.log().debug('Poldi - load data - %s'%(dataFile))
        (sample_root, sample_name) = split(dataFile)
        (sample_name, sampleExt) = splitext(sample_name)
        self.log().error('Poldi -  samples : %s' %(sample_root))
        self.log().error('Poldi -          : %s' %(sample_name))
        self.log().error('Poldi -          : %s' %(sampleExt))

        if "hdf" in sampleExt:
            self.log().error('Poldi -  samples : %s' %(sample_name))
            file_path = dataFile
            sample_name_log = "%sLog" %sample_name
            sample_name_corr = "%sCorr" %sample_name
            sample_name_deadw = "%sDeadWires" %sample_name
            (sample_year, sample_numero) = self.interpreteName(sample_name)
            sample_name_peak = "%sPeak" %sample_name

            sample_info_ws.addRow([sample_name, sample_year, sample_numero, file_path,
                                   sample_name_log,
                                   sample_name_corr,
                                   sample_name_deadw,
                                   sample_name_peak])
        nb_of_sample = sample_info_ws.rowCount()
        self.log().error('Poldi -   1 samples added')
        self.log().error('      -  %d samples in total' %(nb_of_sample))

        self.setProperty("OutputWorkspace", sample_info_ws)



AlgorithmFactory.subscribe(PoldiProjectAddFile)
