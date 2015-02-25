#pylint: disable=no-init
from mantid.api import (PythonAlgorithm,
                        AlgorithmFactory)
from mantid.api import (FileProperty,
                        FileAction)
from mantid.api import (ITableWorkspaceProperty,
                        WorkspaceFactory)
from mantid.kernel import Direction

from os import listdir
from os.path import isfile, join, splitext

import re


class PoldiProjectAddDir(PythonAlgorithm):

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

        self.declareProperty(FileProperty(name="Directory",defaultValue="",action=FileAction.Directory))

        self.declareProperty(ITableWorkspaceProperty("PoldiAnalysis", "PoldiAnalysis", direction=Direction.Output), "Poldi analysis main worksheet")

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
        self.log().warning('Poldi Data Analysis ---- add dir')
        load_data_at_the_end = False

        sample_info_ws_name = ""

        try:
            sample_info_ws_name = self.getProperty("PoldiAnalysis").value
            if sample_info_ws_name == "":
                sample_info_ws_name = "PoldiAnalysis"
            self.log().debug('Poldi Data Analysis ---- %s'%(sample_info_ws_name))
            sample_info_ws = mtd["PoldiAnalysis"]
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
            load_data_at_the_end = True
#             self.setProperty("PoldiAnalysis", sample_info_ws)



#         self.log().debug('Poldi Data Analysis ---- %s'%(sample_info_ws_name))
#         sample_info_ws = mtd[sample_info_ws_name]


        directory = self.getProperty("Directory").value

        onlyfiles = [ f for f in listdir(directory) if isfile(join(directory,f)) ]

        self.log().debug('Poldi - load data')
        for dataFile in onlyfiles:
            (sample_name, sampleExt) = splitext(dataFile)
            file_path = join(directory,dataFile)

#             PoldiProjectAddFile(File=file_path)
#                                 , PoldiAnalysis=sample_info_ws)



            if "hdf" in sampleExt:
                self.log().error('Poldi -  samples : %s' %(sample_name))
                file_path = join(directory,dataFile)
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
        self.log().error('Poldi -  %d samples added' %(nb_of_sample))




        if load_data_at_the_end:
            self.setProperty("PoldiAnalysis", sample_info_ws)


#         if(self.getProperty("RunTheAnalysis").value):
#             PoldiProjectRun(InputWorkspace=sample_info_ws)



AlgorithmFactory.subscribe(PoldiProjectAddDir)
