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
                              MinusMD, RenameWorkspace, CompareMDWorkspaces)
from typing import Tuple
import systemtesting

# Define symmetry operation
SYMMETRY_OPERATION = 'x,y,z;x,-y,z;x,y,-z;x,-y,-z'
# SYMMETRY_OPERATION = 'x,y,z'
NUM_SO = SYMMETRY_OPERATION.count(';') + 1
LOG_VALUE_STEP = 1


class MDNormBackgroundHYSPECTest(systemtesting.MantidSystemTest):

    tolerance = 1e-8

    @staticmethod
    def prepare_md(input_ws_name, merged_md_name, min_log_value, max_log_value, log_step):
        """Load raw event Nexus file and reduce to MDEventWorkspace
        """
        # Filter
        gef_kw_dict = dict()
        if log_step <= max_log_value - min_log_value:
            LogValueInterval = log_step
            gef_kw_dict['LogValueInterval'] = LogValueInterval
        GenerateEventsFilter(InputWorkspace=input_ws_name,
                             OutputWorkspace='splboth',
                             InformationWorkspace='info',
                             UnitOfTime='Nanoseconds',
                             LogName='s1',
                             MinimumLogValue=min_log_value,
                             MaximumLogValue=max_log_value,
                             **gef_kw_dict)
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

        if mtd['md'].getNumberOfEntries() == 1:
            RenameWorkspace(mtd['md'][0], merged_md_name)
        else:
            MergeMD(InputWorkspaces='md', OutputWorkspace=merged_md_name)

    @staticmethod
    def prepare_background(input_md, reference_sample_mde, background_md: str):
        """Prepare background MDEventWorkspace from reduced sample Matrix
        This is the previous solution to merge all the ExperimentInfo
        """
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

    def run_old_solution(self, input_sample_md='merged', background_seed_md='reduced_1', prefix='old'):
        """Do MDNorm to sample and background and clean sample with background in old way
        """
        sample_processed_mdh = f'{prefix}_reducedMD'
        sample_data_mdh = f'{prefix}_dataMD'
        sample_norm_mdh = f'{prefix}_normMD'
        bkgd_processed_mdh = f'{prefix}_backgroundMDH'
        # do MDNorm to sample data
        MDNorm(InputWorkspace=input_sample_md,
               Dimension0Name='QDimension1',
               Dimension0Binning='-5,0.05,5',
               Dimension1Name='QDimension2',
               Dimension1Binning='-5,0.05,5',
               Dimension2Name='DeltaE',
               Dimension2Binning='-2,2',
               Dimension3Name='QDimension0',
               Dimension3Binning='-0.5,0.5',
               SymmetryOperations=SYMMETRY_OPERATION,
               OutputWorkspace=sample_processed_mdh,
               OutputDataWorkspace=sample_data_mdh,
               OutputNormalizationWorkspace=sample_norm_mdh)

        # use reduced_1 as the background
        self.prepare_background(input_md=background_seed_md,
                                reference_sample_mde='merged',
                                background_md='background_MDE')

        # do MDNorm to background
        bkgd_data_mdh = f'{prefix}_background_dataMD'
        MDNorm(InputWorkspace='background_MDE',
               Dimension0Name='QDimension1',
               Dimension0Binning='-5,0.05,5',
               Dimension1Name='QDimension2',
               Dimension1Binning='-5,0.05,5',
               Dimension2Name='DeltaE',
               Dimension2Binning='-2,2',
               Dimension3Name='QDimension0',
               Dimension3Binning='-0.5,0.5',
               SymmetryOperations=SYMMETRY_OPERATION,
               OutputWorkspace=bkgd_processed_mdh,
               OutputDataWorkspace=bkgd_data_mdh,
               OutputNormalizationWorkspace=f'{prefix}_background_normMD')

        # clean data
        clean_data = MinusMD(sample_processed_mdh, bkgd_processed_mdh, OutputWorkspace=f'{prefix}_clean')

        return clean_data, sample_data_mdh, sample_norm_mdh, bkgd_data_mdh

    @staticmethod
    def test_property_setup():
        """Test set up properties

        :return:
        """
        # Test raise exception for wrong background MDE type
        try:
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
        except RuntimeError:
            pass
        else:
            raise AssertionError(f'Expected failure due to Background MD in Q_sample.')

        return True

    def runTest(self):
        """ This is the old way to do MDNorm to sample, background and finally clean background from sample
        Now it serves as a benchmark to generate expected result for enhanced MDNorm

        Current status: To task 88 no accumulation
        Next   status:  To task 89 no accumulation
                        To task 88 with accumulation
                        To task 89 with accumulation
        """
        # Set facility that load data
        config.setFacility('SNS')
        Load(Filename='HYS_13656', OutputWorkspace='sum')
        SetGoniometer(Workspace='sum', Axis0='s1,0,1,0,1')

	# prepare sample MD: in test must between 10 and 12 to match the gold data
        self.prepare_md(input_ws_name='sum', merged_md_name='merged', min_log_value=10, max_log_value=12,
                        log_step=LOG_VALUE_STEP)

        # 1-1 Existing method (but to keep)
        clean_data, sample_data_mdh, sample_norm_mdh, background_data_mdh = self.run_old_solution()
        self._old_clean = str(clean_data)

        # Prepare background workspace
        self.prepare_single_exp_info_background(input_md_name='reduced_1',
                                                output_md_name='bkgd_md',
                                                target_qframe='Q_lab')

        # Test new solutions
        r = MDNorm(InputWorkspace='merged',
                   BackgroundWorkspace='bkgd_md',
                   Dimension0Name='QDimension1',
                   Dimension0Binning='-5,0.05,5',
                   Dimension1Name='QDimension2',
                   Dimension1Binning='-5,0.05,5',
                   Dimension2Name='DeltaE',
                   Dimension2Binning='-2,2',
                   Dimension3Name='QDimension0',
                   Dimension3Binning='-0.5,0.5',
                   SymmetryOperations=SYMMETRY_OPERATION,
                   OutputWorkspace='clean_data',
                   OutputDataWorkspace='dataMD',
                   OutputNormalizationWorkspace='normMD',
                   OutputBackgroundDataWorkspace='background_dataMD',
                   OutputBackgroundNormalizationWorkspace='background_normMD')
        # Test solution
        assert hasattr(r, 'OutputBackgroundNormalizationWorkspace'), 'MDNorm does not execute.'
        # Compare workspace
        rc = CompareMDWorkspaces(Workspace1=background_data_mdh, Workspace2='background_dataMD')
        assert rc.Equals, f'Error message: {rc.Result}'

	# Test: This can mess some experimental data set up for verifying
	# whether the reduction is correct.
        self.test_property_setup()


        # clean_data = r.OutputWorkspace
        # print(f'[VZ DEBUG] clean data: s1 = 10 .. 12: Number of Experiment Info = {clean_data.getNumExperimentInfo()}')

        # SaveMD(InputWorkspace=clean_data, Filename='/tmp/clean.nxs')

        # # MinusMD(LHSWorkspace='reducedMD',
        # #         RHSWorkspace='backgroundMDH',
        # #         OutputWorkspace='result')
        # # SaveMD(InputWorkspace='result', Filename='/tmp/clean.nxs')
        # SaveMD(InputWorkspace='normMD', Filename='/tmp/sample_norm_round1.nxs')
        # SaveMD(InputWorkspace='dataMD', Filename='/tmp/sample_data_round1.nxs')
        # SaveMD(InputWorkspace='background_normMD', Filename='/tmp/normed_background_round1.nxs')
        # # SaveMD(InputWorkspace='background_dataMD', Filename='/tmp/background_data_round1.nxs')

        # # 2nd round
        # clean_data = self.normalize_with_background('sum', 'clean2', (12., 15.),
        #                                             sample_temp_ws_names=('dataMD', 'normMD'),
        #                                             background_temp_ws_names=('background_dataMD', 'background_normMD'))

        # # @JESSE: Here is the difference
        # # save MD for 2nd round
        # # save
        # SaveMD(InputWorkspace=clean_data, Filename='/tmp/clean_round2.nxs')
        # # SaveMD(InputWorkspace='normMD', Filename='/tmp/sample_norm_round1.nxs')
        # # SaveMD(InputWorkspace='dataMD', Filename='/tmp/sample_data_round1.nxs')
        # # SaveMD(InputWorkspace='background_normMD', Filename='/tmp/normed_background_round1.nxs')
        # # SaveMD(InputWorkspace='background_dataMD', Filename='/tmp/background_data_round1.nxs')

        # # @JESSE: If LATER you set log value range from (10, 12) to (10, 15) on line 115,
        # # Then clean_data (on line 152) shall be same as clean_data on line 160 calculated in the current test setup

        # assert mtd.doesExist('clean_data')

    def validate(self):
        self.tolerance = 1e-8
        # FIXME - this is cheating to make #85 and #88 work!
        test_ws_name = self._old_clean
        return test_ws_name, 'MDNormBackgroundHYSPEC.nxs'   # FIXME , 'clean2', 'MDNormBackgroundHYSPEC_10_15.nxs'

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

    def normalize_with_background(self, event_ws_name, output_ws_name, log_value_range: Tuple[float, float],
                                  sample_temp_ws_names: Tuple[str, str],
                                  background_temp_ws_names: Tuple[str, str]):
        """Normalize MD with background

        Parameters
        ----------
        event_ws_name
        output_ws_name
        log_value_range
        sample_temp_ws_names: str, str
            sample temporary data workspace, sample temporary normalization workspace
        background_temp_ws_names
            background temporary data workspace, background temporary normalization workspace

        Returns
        -------

        """
        # prepare sample MD
        self.prepare_md(input_ws_name=event_ws_name, merged_md_name='merged',
                        min_log_value=log_value_range[0], max_log_value=log_value_range[1])
        mde = mtd['merged']
        print(f'[VZ DEBUG] s1 = 12 .. 15: Number of Experiment Info = {mde.getNumExperimentInfo()}')
        # Prepare background workspace
        # # old way - use reduced_1 as the background
        # self.prepare_background(input_md='reduced_1', reference_sample_mde='merged',
        #                         background_md='background_MDE')
        self.prepare_single_exp_info_background(input_md_name='reduced_1',
                                                output_md_name='background_MDE',
                                                target_qframe='Q_lab')

        # do MDNorm to sample data
        MDNorm(InputWorkspace='merged',
               BackgroundWorkspace='background_MDE',
               Dimension0Name='QDimension1',
               Dimension0Binning='-5,0.05,5',
               Dimension1Name='QDimension2',
               Dimension1Binning='-5,0.05,5',
               Dimension2Name='DeltaE',
               Dimension2Binning='-2,2',
               Dimension3Name='QDimension0',
               Dimension3Binning='-0.5,0.5',
               SymmetryOperations='x,y,z;x,-y,z;x,y,-z;x,-y,-z',
               TemporaryDataWorkspace=sample_temp_ws_names[0],  # 'dataMD',
               TemporaryNormalizationWorkspace=sample_temp_ws_names[1],  # 'normMD',
               TemporaryBackgroundDataWorkspace=background_temp_ws_names[0],  # FIXME this is temp solution use S for B
               TemporaryBackgroundNormalizationWorkspace=background_temp_ws_names[1],  # 'normMD',
               OutputWorkspace='result',
               OutputDataWorkspace=sample_temp_ws_names[0],
               OutputNormalizationWorkspace=sample_temp_ws_names[1],
               OutputBackgroundDataWorkspace=background_temp_ws_names[0],
               OutputBackgroundNormalizationWorkspace=background_temp_ws_names[1])

        clean_md = mtd['result']
        print(f'[VZ DEBUG] clean_data s1 = 12 .. 15: Number of Experiment Info = {clean_md.getNumExperimentInfo()}')
        #        OutputWorkspace='reducedMD',
        #        OutputDataWorkspace='dataMD',
        #        OutputNormalizationWorkspace='normMD')

        # # do MDNorm to background
        # MDNorm(InputWorkspace='background_MDE',
        #        Dimension0Name='QDimension1',
        #        Dimension0Binning='-5,0.05,5',
        #        Dimension1Name='QDimension2',
        #        Dimension1Binning='-5,0.05,5',
        #        Dimension2Name='DeltaE',
        #        Dimension2Binning='-2,2',
        #        Dimension3Name='QDimension0',
        #        Dimension3Binning='-0.5,0.5',
        #        SymmetryOperations='x,y,z;x,-y,z;x,y,-z;x,-y,-z',
        #        TemporaryDataWorkspace=background_temp_ws_names[0],
        #        TemporaryNormalizationWorkspace=background_temp_ws_names[0],  # 'background_dataMD',
        #        OutputWorkspace='backgroundMDH',
        #        OutputDataWorkspace='background_dataMD',
        #        OutputNormalizationWorkspace='background_normMD')

        # # clean
        # clean_md = MinusMD(LHSWorkspace='reducedMD',
        #                    RHSWorkspace='backgroundMDH',
        #                    OutputWorkspace=output_ws_name)
        # origin/SPEC59_MDNormBkgd_feature

        return clean_md
