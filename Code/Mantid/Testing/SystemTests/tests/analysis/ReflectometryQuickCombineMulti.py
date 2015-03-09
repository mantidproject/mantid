#pylint: disable=no-init
import stresstesting
from mantid.simpleapi import *
from isis_reflectometry import quick
from isis_reflectometry import combineMulti

class ReflectometryQuickCombineMulti(stresstesting.MantidStressTest):
    """
    This is a system test for the top-level CombineMulti routines. Quick is the name given to the
    ISIS reflectometry reduction scripts. CombineMulti is used for stitching together runs converted Into I/I0 vs |Q| taken at
    different incident angles (and hence covering different Q-ranges)
    """

    __stitchedWorkspaceName = "stitched_13460_13462"

    def doQuickOnRun(self, runNumber, transmissionNumbers, instrument, incidentAngle):
        defaultInstKey = 'default.instrument'
        defaultInstrument = config[defaultInstKey]
        try:
            config[defaultInstKey] = instrument
            LoadISISNexus(Filename=str(runNumber), OutputWorkspace=str(runNumber))
            for transmissionNumber in transmissionNumbers:
                LoadISISNexus(Filename=str(transmissionNumber), OutputWorkspace=str(transmissionNumber))

            transmissionRuns = ",".join(map(str, transmissionNumbers))
            # Run quick
            quick.quick(str(runNumber), trans=transmissionRuns, theta=incidentAngle)
        finally:
            config[defaultInstKey] = defaultInstrument
        return mtd[str(runNumber) + '_IvsQ']

    def createBinningParam(self, low, step, high):
        return "%f,%f,%f" %(low, step, high)

    def runTest(self):
        step = 0.040
        run1QLow = 0.010
        run1QHigh = 0.06
        run2QLow = 0.035
        run2QHigh = 0.300

        # Create IvsQ workspaces
        IvsQ1 = self.doQuickOnRun(runNumber=13460, transmissionNumbers=[13463,13464], instrument='INTER', incidentAngle=0.7)
        IvsQ1Binned = Rebin(InputWorkspace=IvsQ1, Params=self.createBinningParam(run1QLow, -step, run1QHigh))

        # Create IvsQ workspaces
        IvsQ2 = self.doQuickOnRun(runNumber=13462, transmissionNumbers=[13463,13464], instrument='INTER', incidentAngle=2.3)
        IvsQ2Binned = Rebin(InputWorkspace=IvsQ2, Params=self.createBinningParam(run2QLow, -step, run2QHigh))

        # Peform the stitching
        combineMulti.combineDataMulti([IvsQ1Binned.name(), IvsQ2Binned.name()], self.__stitchedWorkspaceName, [run1QLow, run2QLow], [run1QHigh, run2QHigh], run1QLow, run2QHigh, -step, 1)


    def validate(self):
        self.disableChecking.append('Instrument')
        return self.__stitchedWorkspaceName,'QuickStitchedReferenceResult.nxs'
