# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,invalid-name
#from mantid.api import AlgorithmFactory
#from mantid.simpleapi import PythonAlgorithm, WorkspaceProperty
# from mantid.kernel import Direction
from __future__ import (absolute_import, division, print_function)
from mantid.api import *
from mantid.kernel import *
import mantid.simpleapi
import os


class LoadVisionElasticEQ(PythonAlgorithm):

    __equatorial = "bank25,bank26,bank27,bank28,bank29,bank30"

    def category(self):
        return "DataHandling\\Nexus"

    def seeAlso(self):
        return [ "LoadVisionElasticBS","LoadVisionInelastic" ]

    def name(self):
        return "LoadVisionElasticEQ"

    def summary(self):
        return "This algorithm loads only the equatorial elastic detectors on VISION."

    def PyInit(self):
        self.declareProperty(FileProperty("Filename", "", action=FileAction.Load, extensions=[".nxs.h5"]))
        self.declareProperty("Banks", "all")
        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output))

    def PyExec(self):
        filename = self.getProperty("Filename").value
        banks = self.getProperty("Banks").value

        # First lets replace 'All' with 'equatorial'
        banks = banks.lower().replace("all", "equatorial")
        banks = banks.lower().replace("equatorial", self.__equatorial)
        banks = banks.lower()

        # Let's make sure we have a unique and sorted list
        banks_integers = banks.replace("bank", "")
        banks_list_integers = banks_integers.split(",")
        banks_list_integers = list(set(banks_list_integers))
        banks_list_integers.sort(key=int)
        banks_list = ['bank{0}'.format(i) for i in banks_list_integers]
        banks = ",".join(banks_list)

        wksp_name = "__tmp"
        wksp_cache = "__cache"

        idf_path = config.getInstrumentDirectory()
        dictionary_path = os.path.join(idf_path, "nexusdictionaries")

        ws = None
        try:
            # First try to load as events
            ws = mantid.simpleapi.LoadEventNexus(Filename=filename, BankName=banks, OutputWorkspace=wksp_name)
        #pylint: disable=bare-except
        except:
            workspaces = []
            first_bank = True
            # Now lets try histograms.
            for bank in banks_list:
                mantid.simpleapi.LoadFlexiNexus(Filename=filename,
                                                Dictionary=os.path.join(dictionary_path, 'vision-'+bank+'.dic'),
                                                OutputWorkspace=bank)
                mantid.simpleapi.LoadInstrument(Workspace=bank,
                                                Filename=ExperimentInfo.getInstrumentFilename('VISION'),
                                                RewriteSpectraMap=False)
                workspaces.append(bank)
                if first_bank:
                    mantid.simpleapi.RenameWorkspace(InputWorkspace=bank, OutputWorkspace=wksp_cache)
                    first_bank = False
                else:
                    mantid.simpleapi.ConjoinWorkspaces(wksp_cache, bank)

            ws = mantid.simpleapi.RenameWorkspace(InputWorkspace=wksp_cache, OutputWorkspace=wksp_name)

        self.setProperty("OutputWorkspace", ws)
        mantid.simpleapi.DeleteWorkspace(wksp_name)


# Register
AlgorithmFactory.subscribe(LoadVisionElasticEQ)
