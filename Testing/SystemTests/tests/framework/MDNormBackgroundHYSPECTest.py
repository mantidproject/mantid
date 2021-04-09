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
                              MDNorm, CloneWorkspace, AddSampleLogMultiple, mtd,
                              MinusMD, RenameWorkspace, CompareMDWorkspaces)
from typing import Tuple
import systemtesting
from collections import namedtuple

# Define symmetry operation
SYMMETRY_OPERATION = 'x,y,z;x,-y,z;x,y,-z;x,-y,-z'
# SYMMETRY_OPERATION = 'x,y,z'
NUM_SO = SYMMETRY_OPERATION.count(';') + 1
LOG_VALUE_STEP = 1

# Returned workspaces
MDNormResults = namedtuple('MDNormResults', 'cleaned sample_data, sample_norm, bkgd_data, bkgd_norm')


class MDNormBackgroundHYSPECTest(systemtesting.MantidSystemTest):

    tolerance = 1e-8

    @staticmethod
    def prepare_md(input_ws_name, merged_md_name, min_log_value, max_log_value, log_step, prefix) -> str:
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
        FilterEvents(InputWorkspace=input_ws_name,
                     SplitterWorkspace='splboth',
                     InformationWorkspace='info',
                     FilterByPulseTime=True,
                     GroupWorkspaces=True,
                     OutputWorkspaceIndexedFrom1=True,
                     OutputWorkspaceBaseName='split')
        # Clean memory
        DeleteWorkspace('splboth')
        DeleteWorkspace('info')

        reduced_ws_group = f'reduced_{prefix}'
        DgsReduction(SampleInputWorkspace='split',
                     SampleInputMonitorWorkspace='split_1',
                     IncidentEnergyGuess=50,
                     SofPhiEIsDistribution=False,
                     TimeIndepBackgroundSub=True,
                     TibTofRangeStart=10400,
                     TibTofRangeEnd=12400,
                     OutputWorkspace=reduced_ws_group)
        # Clean memory
        DeleteWorkspace('split')

        SetUB(Workspace=reduced_ws_group,
              a=5.823,
              b=6.475,
              c=3.186,
              u='0,1,0',
              v='0,0,1')
        CropWorkspaceForMDNorm(InputWorkspace=reduced_ws_group,
                               XMin=-25,
                               XMax=49,
                               OutputWorkspace=reduced_ws_group)
        ConvertToMD(InputWorkspace=reduced_ws_group,
                    QDimensions='Q3D',
                    Q3DFrames='Q_sample',
                    OutputWorkspace='md',
                    MinValues='-11,-11,-11,-25',
                    MaxValues='11,11,11,49')

        if mtd['md'].getNumberOfEntries() == 1:
            RenameWorkspace(mtd['md'][0], merged_md_name)
        else:
            MergeMD(InputWorkspaces='md', OutputWorkspace=merged_md_name)

        return reduced_ws_group

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
                                 LogValues='{},{},{}'.format(phi, chi, omega))
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

    def run_old_solution(self, input_sample_md, background_seed_md, prefix, temp_norm_result=None):
        """Do MDNorm to sample and background and clean sample with background in old way
        """
        sample_processed_mdh = f'{prefix}_reducedMD'
        sample_data_mdh = f'{prefix}_dataMD'
        sample_norm_mdh = f'{prefix}_normMD'
        bkgd_processed_mdh = f'{prefix}_backgroundMDH'

        # do MDNorm to sample data
        data_temp_dict = dict()
        if temp_norm_result:
            data_temp_dict['TemporaryDataWorkspace'] = temp_norm_result.sample_data
            data_temp_dict['TemporaryNormalizationWorkspace'] = temp_norm_result.sample_norm
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
               OutputNormalizationWorkspace=sample_norm_mdh,
               **data_temp_dict)

        # use reduced_1 as the background
        self.prepare_background(input_md=background_seed_md,
                                reference_sample_mde=input_sample_md,
                                background_md='background_MDE')

        # do MDNorm to background
        data_temp_dict = dict()
        if temp_norm_result:
            data_temp_dict['TemporaryDataWorkspace'] = temp_norm_result.bkgd_data
            data_temp_dict['TemporaryNormalizationWorkspace'] = temp_norm_result.bkgd_norm
        bkgd_data_mdh = f'{prefix}_background_dataMD'
        bkgd_norm_mdh = f'{prefix}_background_normMD'
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
               OutputNormalizationWorkspace=bkgd_norm_mdh,
               **data_temp_dict)

        # clean data
        clean_data = MinusMD(sample_processed_mdh, bkgd_processed_mdh, OutputWorkspace=f'{prefix}_clean')

        # Output
        norm_step1 = {"cleaned": clean_data,
                      "sample_data": sample_data_mdh,
                      "sample_norm": sample_norm_mdh,
                      "bkgd_data": bkgd_data_mdh,
                      "bkgd_norm": bkgd_norm_mdh}

        return MDNormResults(**norm_step1)

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
        dgs_red_group_name1 = self.prepare_md(input_ws_name='sum', merged_md_name='merged1',
                                              min_log_value=10, max_log_value=12,
                                              log_step=LOG_VALUE_STEP, prefix='step1')

        # 1-1 Existing method (but to keep)
        benchmark = self.run_old_solution('merged1', f'{dgs_red_group_name1}_1', 'oldstep1')
        self._old_clean = str(benchmark.cleaned)

        # Prepare background workspace
        self.prepare_single_exp_info_background(input_md_name=f'{dgs_red_group_name1}_1',
                                                output_md_name='bkgd_md1',
                                                target_qframe='Q_lab')

        # Test new solutions
        r = MDNorm(InputWorkspace='merged1',
                   BackgroundWorkspace='bkgd_md1',
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
        # Compare workspaces: binned background data workspace
        rc = CompareMDWorkspaces(Workspace1=benchmark.bkgd_data, Workspace2='background_dataMD')
        assert rc.Equals, f'Error message: {rc.Result}'
        # Compare workspaces: background normalization workspace
        r89 = CompareMDWorkspaces(Workspace1=benchmark.bkgd_norm, Workspace2='background_normMD', Tolerance=1E-9)
        assert r89.Equals, f'Incompatible background normalization workspaces: {r89.Result}'
        # Compare the cleaned MD
        r_out = CompareMDWorkspaces(Workspace1=benchmark.cleaned, Workspace2='clean_data', Tolerance=1E-16)
        if not r_out.Equals:
            raise ValueError(f'Sample output data does not match: {r_out.Result}')

        # Round 2: Test accumulation mode
        dgs_red_group_name1 = self.prepare_md(input_ws_name='sum', merged_md_name='merged2', min_log_value=13,
                                              max_log_value=15, log_step=LOG_VALUE_STEP, prefix='step2')
        # Old solution
        benchmark2 = self.run_old_solution('merged2', f'{dgs_red_group_name1}_2', 'oldstep2', benchmark)
        # 1-2 Test new solutions
        self.prepare_single_exp_info_background(input_md_name=f'{dgs_red_group_name1}_2',
                                                output_md_name='bkgd_md2',
                                                target_qframe='Q_lab')
        r = MDNorm(InputWorkspace='merged2',
                   BackgroundWorkspace='bkgd_md2',
                   TemporaryDataWorkspace='dataMD',
                   TemporaryNormalizationWorkspace='normMD',
                   TemporaryBackgroundDataWorkspace='background_dataMD',
                   TemporaryBackgroundNormalizationWorkspace='background_normMD',
                   Dimension0Name='QDimension1',
                   Dimension0Binning='-5,0.05,5',
                   Dimension1Name='QDimension2',
                   Dimension1Binning='-5,0.05,5',
                   Dimension2Name='DeltaE',
                   Dimension2Binning='-2,2',
                   Dimension3Name='QDimension0',
                   Dimension3Binning='-0.5,0.5',
                   SymmetryOperations=SYMMETRY_OPERATION,
                   OutputWorkspace='clean_data_2',
                   OutputDataWorkspace='dataMD2',
                   OutputNormalizationWorkspace='normMD2',
                   OutputBackgroundDataWorkspace='background_dataMD2',
                   OutputBackgroundNormalizationWorkspace='background_normMD2')

        # Compare data workspace
        r_sd = CompareMDWorkspaces(Workspace1=benchmark2.sample_data, Workspace2='dataMD', Tolerance=1E-7)
        assert r_sd.Equals, f'Sample binned data MD not equal: {r_sd.Result}'

        # Compare gold (old solution) and new: passed test in #88
        r = CompareMDWorkspaces(Workspace1=benchmark2.bkgd_data, Workspace2='background_dataMD2')
        if not r.Equals:
            raise ValueError(f'Accumulation mode: Background binned data MD: {r.Result}')

        # Compare gold (old solution) and new: passed test in #88
        r_bn = CompareMDWorkspaces(Workspace1=benchmark2.bkgd_norm, Workspace2='background_normMD2', Tolerance=1E-9)
        if not r_bn.Equals:
            raise ValueError(f'Accumulation mode: Background normalization MD: {r_bn.Result}')

        # Test MDNorm's checking on inputs
        # Note that this test must be put after all the tests because
        # it will modify some workspaces and thus mess some experimental data set up to verify
        self.test_property_setup('merged1', f'{dgs_red_group_name1}_1')

    def validate(self):
        self.tolerance = 1e-8
        test_ws_name = 'clean_data'
        return test_ws_name, 'MDNormBackgroundHYSPEC.nxs'

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
        # Prepare background workspace
        # old way - use reduced_1 as the background
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
               TemporaryDataWorkspace=sample_temp_ws_names[0],
               TemporaryNormalizationWorkspace=sample_temp_ws_names[1],
               TemporaryBackgroundDataWorkspace=background_temp_ws_names[0],
               TemporaryBackgroundNormalizationWorkspace=background_temp_ws_names[1],
               OutputWorkspace='result',
               OutputDataWorkspace=sample_temp_ws_names[0],
               OutputNormalizationWorkspace=sample_temp_ws_names[1],
               OutputBackgroundDataWorkspace=background_temp_ws_names[0],
               OutputBackgroundNormalizationWorkspace=background_temp_ws_names[1])

        clean_md = mtd['result']

        return clean_md

    @staticmethod
    def test_property_setup(input_mde: str, reduced_background: str):
        """Test set up properties

        :return:
        """
        # Test raise exception for wrong background MDE type
        try:
            # Create background workspace in Q lab
            ConvertToMD(InputWorkspace=reduced_background,
                        QDimensions='Q3D',
                        Q3DFrames='Q_sample',
                        OutputWorkspace='bkgd_md',
                        MinValues='-11,-11,-11,-25',
                        MaxValues='11,11,11,49')

            MDNorm(InputWorkspace=input_mde,
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
