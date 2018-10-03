# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,invalid-name
from __future__ import (absolute_import, division, print_function)

from mantid.api import PythonAlgorithm, AlgorithmFactory,MatrixWorkspaceProperty,PropertyMode
from mantid.kernel import Direction
import mantid.simpleapi
import mantid
import numpy


class DakotaChiSquared(PythonAlgorithm):
    """ Get chi squared by comparing two mantid nexus files
    """

    def category(self):
        """ Return category
        """
        return "Utility\\Workspaces"

    def name(self):
        """ Return name
        """
        return "DakotaChiSquared"

    def summmary(self):
        return "Compare two nexus files containing matrix workspaces and output chi squared into a file"

    def PyInit(self):
        """ Declare properties
        """
        f1=mantid.api.FileProperty("DataFile","",mantid.api.FileAction.Load,".nxs")
        self.declareProperty(f1,"Input Nexus file containing data.")
        f2=mantid.api.FileProperty("CalculatedFile","",mantid.api.FileAction.Load,".nxs")
        self.declareProperty(f2,"Input Nexus file containing calculated data.")
        fout=mantid.api.FileProperty("OutputFile","",mantid.api.FileAction.Save,".xml")
        self.declareProperty(fout,"Output filename containing chi^2.")
        self.declareProperty("ChiSquared",0.0,Direction.Output)
        self.declareProperty(MatrixWorkspaceProperty("ResidualsWorkspace", "",Direction.Output,PropertyMode.Optional),
                             "The name of the workspace that will contain residuals.")
        return

    def PyExec(self):
        """ Main execution body
        """
            #get parameters
        f1 = self.getProperty("DataFile").value
        f2 = self.getProperty("CalculatedFile").value
        fout = self.getProperty("OutputFile").value

            #load files
        __w1=mantid.simpleapi.Load(f1)
        __w2=mantid.simpleapi.Load(f2)

        #validate inputs as histograms
        if isinstance(__w1, mantid.api.IEventWorkspace):
            raise ValueError( 'Wrong workspace type for data file. Expected histogram file but found event')
        if isinstance(__w2, mantid.api.IEventWorkspace):
            raise ValueError( 'Wrong workspace type for calculated file. Expected histogram file but found event')
        if __w1.blocksize()!=__w2.blocksize() or __w1.getNumberHistograms()!=__w2.getNumberHistograms():
            mantid.kernel.logger.error('The file sizes are different')
            raise ValueError( 'The file sizes are different')

            #calculate chi^2
        soeName = self.getPropertyValue("ResidualsWorkspace")
        if len(soeName)>0:
            mantid.simpleapi.SignalOverError(__w1-__w2,OutputWorkspace=soeName)
            self.setProperty("ResidualsWorkspace",soeName)
            __soe=mantid.mtd[soeName]
        else:
            __soe=mantid.simpleapi.SignalOverError(__w1-__w2)
        __soe2=__soe*__soe
        __soe2=mantid.simpleapi.ReplaceSpecialValues(__soe2,0,0,0,0)

        data=__soe2.extractY()
        chisquared=numpy.sum(data)

            #write out the Dakota chi squared file
        f = open(fout,'w')
        f.write(str(chisquared)+' obj_fn\n')
        f.close()

        self.setProperty("ChiSquared",chisquared)

            #cleanup
        mantid.simpleapi.DeleteWorkspace(__w1.name())
        mantid.simpleapi.DeleteWorkspace(__w2.name())
        mantid.simpleapi.DeleteWorkspace(__soe2.name())
        if len(soeName)==0:
            mantid.simpleapi.DeleteWorkspace(__soe.name())


AlgorithmFactory.subscribe(DakotaChiSquared)
