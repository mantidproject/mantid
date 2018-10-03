# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=invalid-name,no-init
from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi
import mantid.api
import mantid.kernel


class CorrectLogTimes(mantid.api.PythonAlgorithm):
    """ Class to shift log times to match proton charge
    """

    ws = None

    def category(self):
        """ Mantid required
        """
        return "DataHandling\\Logs"

    def seeAlso(self):
        return [ "ChangeLogTime","CreateLogTimeCorrection","ChangePulsetime","ShiftLogTime" ]

    def name(self):
        """ Mantid required
        """
        return "CorrectLogTimes"

    def summary(self):
        return "This algorithm attempts to make the time series property logs start at the same time "+\
               "as the first time in the proton charge log."

    def PyInit(self):
        self.declareProperty(mantid.api.WorkspaceProperty("Workspace", "",direction=mantid.kernel.Direction.InOut), "Input workspace")
        self.declareProperty("LogNames","",doc="Experimental log values to be shifted. If empty, will attempt to shift all logs")

    def PyExec(self):
        self.ws = self.getProperty("Workspace").value
        logNames = self.getProperty("LogNames").value

        logList=[]

        #check for parameters and build the result string
        for value in logNames.split(','):
            value=value.strip()
            if len(value)>0:
                if not self.ws.run().hasProperty(value):
                    err = 'Property '+value+' not found'
                    raise ValueError(err)
                else:
                    logList.append(value)

        if len(logList)==0:
            logList=list(self.ws.getRun().keys())

        for x in logList:
            if x not in ['duration','proton_charge','start_time','run_title','run_start','run_number','gd_prtn_chrg','end_time']:
                try:
                    self.ShiftTime(x)
                #pylint: disable= bare-except
                except:
                    pass

    def ShiftTime(self, logName):
        """
        shift the time in a given log to match the time in the proton charge log"
        """
        PC = self.ws.getRun()['proton_charge'].firstTime()
        P = self.ws.getRun()[logName].firstTime()
        Tdiff = PC-P
        Tdiff_num = Tdiff.total_milliseconds()*1E-3
        mantid.simpleapi.ChangeLogTime(InputWorkspace=self.ws, OutputWorkspace = self.ws, LogName = logName, TimeOffset = Tdiff_num)


mantid.api.AlgorithmFactory.subscribe(CorrectLogTimes)
