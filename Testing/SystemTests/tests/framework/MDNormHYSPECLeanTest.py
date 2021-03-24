# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init

"""
System test for MDNorm
"""

from mantid.simpleapi import *
import systemtesting


class MDNormHYSPECLeanTest(systemtesting.MantidSystemTest):

    tolerance=1e-8

    def requiredMemoryMB(self):
        return 5000

    def requiredFiles(self):
        return ['HYS_13656_event.nxs','HYS_13657_event.nxs','HYS_13658_event.nxs']

    def runTest(self):
        config.setFacility('SNS')
        Load(Filename='HYS_13657',OutputWorkspace='sum')
        SetGoniometer(Workspace='sum', Axis0='s1,0,1,0,1')
        # original: min =   , max = 90  461 outputs  12 min
        # original: min = 10, max = 15  30 outputs    4 min
        # original: min = 10, max = 11  15 outputs    4 min
        GenerateEventsFilter(InputWorkspace='sum',
                             OutputWorkspace='splboth',
                             InformationWorkspace='info',
                             UnitOfTime='Nanoseconds',
                             LogName='s1',
                             MinimumLogValue=10,
                             MaximumLogValue=12,
                             LogValueInterval=1)
        SaveNexusProcessed(InputWorkspace='splboth', Filename='/tmp/splitter.nxs')
        FilterEvents(InputWorkspace='sum',
                     SplitterWorkspace='splboth',
                     InformationWorkspace='info',
                     FilterByPulseTime=True,
                     GroupWorkspaces=True,
                     OutputWorkspaceIndexedFrom1=True,
                     OutputWorkspaceBaseName='split')
        print(f'Workspaces: {mtd.getObjectNames()}')
        DeleteWorkspace('sum')
        DeleteWorkspace('splboth')
        DeleteWorkspace('info')
        DgsReduction(SampleInputWorkspace='split',
                     SampleInputMonitorWorkspace='split_1',
                     IncidentEnergyGuess=50,
                     SofPhiEIsDistribution=False,
                     TimeIndepBackgroundSub=True,
                     TibTofRangeStart=10400,
                     TibTofRangeEnd=12400,
                     OutputWorkspace='reduced')
        SetUB(Workspace='reduced',
              a=5.823,
              b=6.475,
              c=3.186,
              u='0,1,0',
              v='0,0,1')
        CropWorkspaceForMDNorm(InputWorkspace='reduced',
                               XMin=-25,
                               XMax=49,
                               OutputWorkspace='reduced')
        ConvertToMD(InputWorkspace='reduced',
                    QDimensions='Q3D',
                    Q3DFrames='Q_sample',
                    OutputWorkspace='md',
                    MinValues='-11,-11,-11,-25',
                    MaxValues='11,11,11,49')

        # Create background workspace in Q lab
        ConvertToMD(InputWorkspace='reduced_1',
                    QDimensions='Q3D',
                    # Q3DFrames='Q_lab',
                    Q3DFrames='Q_sample',
                    OutputWorkspace='bkgd_md',
                    MinValues='-11,-11,-11,-25',
                    MaxValues='11,11,11,49')


        print(f'[VZ......] Workspaces: {mtd.getObjectNames()}')
        DeleteWorkspace("split")
        DeleteWorkspace("reduced")
        DeleteWorkspace("PreprocessedDetectorsWS")
        DeleteWorkspace("TOFCorrectWS")
        print(f'[VZ......] Type of md: {type(mtd["md"])}')

        MergeMD(InputWorkspaces='md', OutputWorkspace='merged')
        DeleteWorkspace("md")
        MDNorm(InputWorkspace='merged',
               BackgroundWorkspace='bkgd_md',
               Dimension0Name='QDimension1',
               Dimension0Binning='-5,0.05,5',
               Dimension1Name='QDimension2',
               Dimension1Binning='-5,0.05,5',
               Dimension2Name='DeltaE',
               Dimension2Binning='-2,2',
               Dimension3Name='QDimension0',
               Dimension3Binning='-0.5,0.5',
               SymmetryOperations='x,y,z;x,-y,z;x,y,-z;x,-y,-z',
               OutputWorkspace='result',
               OutputDataWorkspace='dataMD',
               OutputNormalizationWorkspace='normMD')
        assert mtd.doesExist('result')
        DeleteWorkspace("dataMD")
        DeleteWorkspace("normMD")
        DeleteWorkspace("merged")

    def validate(self):
        self.tolerance = 1e-8
        return 'result','MDNormHYSPEC.nxs'
