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

from mantid.simpleapi import (config, Load, SetGoniometer, GenerateEventsFilter, FilterEvents, DeleteWorkspace,
                              DgsReduction, SetUB, CropWorkspaceForMDNorm, ConvertToMD, MergeMD,
                              MDNorm, CloneWorkspace, AddSampleLogMultiple, mtd, SaveMD,
                              MinusMD)
import pytest
# FIXME import systemtesting


class MDNormHYSPECBackgroundTest:  # FIXME systemtesting.MantidSystemTest):

    tolerance = 1e-8

    @staticmethod
    def prepare_md(input_ws_name, merged_md_name, min_log_value, max_log_value):
        # Filter
        GenerateEventsFilter(InputWorkspace=input_ws_name,
                             OutputWorkspace='splboth',
                             InformationWorkspace='info',
                             UnitOfTime='Nanoseconds',
                             LogName='s1',
                             MinimumLogValue=min_log_value,
                             MaximumLogValue=max_log_value,
                             LogValueInterval=1)
        FilterEvents(InputWorkspace='sum',
                     SplitterWorkspace='splboth',
                     InformationWorkspace='info',
                     FilterByPulseTime=True,
                     GroupWorkspaces=True,
                     OutputWorkspaceIndexedFrom1=True,
                     OutputWorkspaceBaseName='split')
        # Clean memory
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
        # Clean memory
        DeleteWorkspace('split')

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

        MergeMD(InputWorkspaces='md', OutputWorkspace=merged_md_name)

    @staticmethod
    def prepare_background(input_md, reference_sample_mde, background_md):
        dgs_data = CloneWorkspace(input_md)
        data_MDE = mtd[reference_sample_mde]
        if mtd.doesExist('background_MDE'):
            DeleteWorkspace('background_MDE')

        for i in range(data_MDE.getNumExperimentInfo()):
            phi, chi, omega = data_MDE.getExperimentInfo(i).run().getGoniometer().getEulerAngles('YZY')
            AddSampleLogMultiple(Workspace=dgs_data,
                                 LogNames='phi, chi, omega',
                                 LogValues='{},{},{}'.format(phi,chi,omega))
            SetGoniometer(Workspace=dgs_data, Goniometers='Universal')
            ConvertToMD(InputWorkspace=dgs_data,
                        QDimensions='Q3D',
                        dEAnalysisMode='Direct',
                        Q3DFrames="Q_sample",
                        MinValues='-11,-11,-11,-25',
                        MaxValues='11,11,11,49',
                        PreprocDetectorsWS='-',
                        OverwriteExisting=False,
                        OutputWorkspace=background_md)

    def requiredMemoryMB(self):
        return 5000

    def requiredFiles(self):
        return ['HYS_13656_event.nxs']

    def old_solution(self):
        # do MDNorm to sample data
        MDNorm(InputWorkspace='merged',
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

        # old way - use reduced_1 as the background
        self.prepare_background(input_md='reduced_1', reference_sample_mde='merged',
                                background_md='background_MDE')

        # do MDNorm to background
        MDNorm(InputWorkspace='background_MDE',
               Dimension0Name='QDimension1',
               Dimension0Binning='-5,0.05,5',
               Dimension1Name='QDimension2',
               Dimension1Binning='-5,0.05,5',
               Dimension2Name='DeltaE',
               Dimension2Binning='-2,2',
               Dimension3Name='QDimension0',
               Dimension3Binning='-0.5,0.5',
               SymmetryOperations='x,y,z;x,-y,z;x,y,-z;x,-y,-z',
               OutputWorkspace='backgroundMDH',
               OutputDataWorkspace='background_dataMD',
               OutputNormalizationWorkspace='background_normMD')

        # Clean data
        clean_data = MinusMD('result', 'backgroundMDH')

        return clean_data

    @staticmethod
    def test_property_setup():
        """Test set up properties

        :return:
        """
        # Test raise exception for wrong background MDE type
        with pytest.raises(RuntimeError):
            # Create background workspace in Q lab
            ConvertToMD(InputWorkspace='reduced_1',
                        QDimensions='Q3D',
                        Q3DFrames='Q_sample',
                        OutputWorkspace='bkgd_md',
                        MinValues='-11,-11,-11,-25',
                        MaxValues='11,11,11,49')

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

            DeleteWorkspace(Workspace='bkgd_md')

        return True

    def runTest(self):
        """ This is the old way to do MDNorm to sample, background and finally clean background from sample
        Now it serves as a benchmark to generate expected result for enhanced MDNorm
        """
        # Set facility that load data
        config.setFacility('SNS')
        Load(Filename='HYS_13656', OutputWorkspace='sum')
        SetGoniometer(Workspace='sum', Axis0='s1,0,1,0,1')

        # prepare sample MD
        self.prepare_md(input_ws_name='sum', merged_md_name='merged', min_log_value=10, max_log_value=12)

        # Test
        self.test_property_setup()

        # Prepare background workspace
        self.prepare_single_exp_info_background(input_md_name='reduced_1',
                                                output_md_name='bkgd_md',
                                                target_qframe='Q_lab')

        # Test new solutions
        r = MDNorm(InputWorkspace='merged',
                   BackgroundWorkspace='bkgd_md',  # TODO Enable/Disable
                   Dimension0Name='QDimension1',
                   Dimension0Binning='-5,0.05,5',
                   Dimension1Name='QDimension2',
                   Dimension1Binning='-5,0.05,5',
                   Dimension2Name='DeltaE',
                   Dimension2Binning='-2,2',
                   Dimension3Name='QDimension0',
                   Dimension3Binning='-0.5,0.5',
                   SymmetryOperations='x,y,z;x,-y,z;x,y,-z;x,-y,-z',
                   OutputWorkspace='clean_data',
                   OutputDataWorkspace='dataMD',
                   OutputNormalizationWorkspace='normMD',
                   OutputBackgroundNormalizationWorkspace='normBkgdMD')
        assert hasattr(r, 'OutputBackgroundNormalizationWorkspace')
        clean_data = r.OutputWorkspace

        SaveMD(InputWorkspace=clean_data, Filename='/tmp/clean.nxs')
        SaveMD(InputWorkspace='normMD', Filename='/tmp/sample_norm.nxs')
        SaveMD(InputWorkspace='dataMD', Filename='/tmp/sample_data.nxs')
        SaveMD(InputWorkspace='background_normMD', Filename='/tmp/normed_background.nxs')
        SaveMD(InputWorkspace='background_MDE', Filename='/tmp/background_data.nxs')

        assert mtd.doesExist('clean_data')

    def validate(self):
        self.tolerance = 1e-8
        return 'result', 'MDNormHYSPEC.nxs'

    @staticmethod
    def prepare_single_exp_info_background(input_md_name, output_md_name, target_qframe='Q_lab'):
        """Prepare a background MDE containing only 1 ExperimentInfo

        :param self:
        :return:
        """
        if target_qframe not in ['Q_sample', 'Q_lab']:
            raise ValueError(f'QFrame with name {target_qframe} is not recognized.')

        # Create background workspace in Q lab
        ConvertToMD(InputWorkspace=input_md_name,
                    QDimensions='Q3D',
                    Q3DFrames=target_qframe,
                    OutputWorkspace=output_md_name,
                    MinValues='-11,-11,-11,-25',
                    MaxValues='11,11,11,49')


MDNormHYSPECBackgroundTest().runTest()
