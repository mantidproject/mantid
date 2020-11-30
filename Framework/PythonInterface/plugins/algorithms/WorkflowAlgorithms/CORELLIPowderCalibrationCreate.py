# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
import os
import random
import string
from typing import List, Optional, Union

# DEGUG
import sys
sys.path.insert(0, '/opt/mantidnightly/scripts')

from corelli.calibration.database import load_calibration_set
from corelli.calibration.utils import apply_calibration
from mantid.api import (AnalysisDataService, DataProcessorAlgorithm, IEventWorkspaceProperty, mtd,
                        Progress, TextAxis, Workspace, WorkspaceGroup, WorkspaceUnitValidator)
from mantid.dataobjects import TableWorkspace, Workspace2D
from mantid.simpleapi import (AlgorithmFactory, AlignComponents, AnalysisDataService, CalculateDIFC,
                              CloneWorkspace, CopyInstrumentParameters, ConvertUnits,
                              CreateEmptyTableWorkspace, CreateGroupingWorkspace, CreateWorkspace,
                              DeleteWorkspace, GroupDetectors, GroupWorkspaces, LoadEventNexus,
                              PDCalibration, Rebin)
from mantid.kernel import Direction, FloatBoundedValidator, logger, StringArrayProperty

def unique_workspace_name(n: int = 5, prefix: Optional[str] = '', suffix: Optional[str] =''):
    r"""
    Create a random sequence of `n` lowercase characters that is guaranteed
    not to collide with the name of any existing Mantid workspace registered
    in the analysis data service.

    @param n : size of the sequence
    @param prefix : string to prefix the randon sequence
    @param suffix : string to suffix the randon sequence
    """

    n_seq = ''.join(random.choice(string.ascii_lowercase) for _ in range(n))
    ws_name = '{}{}{}'.format(str(prefix), n_seq, str(suffix))
    while ws_name in AnalysisDataService.getObjectNames():
        characters = [random.choice(string.ascii_lowercase) for _ in range(n)]
        n_seq = ''.join(characters)
        ws_name = '{}{}{}'.format(str(prefix), n_seq, str(suffix))
    return ws_name


def temp_workspace_generator(temp_workspaces):
    r"""
    Create a funtion that serve a unique workspace name and position it to be later deleted

    @param temp_workspaces :: list of workspaces to be deleted after algorithm has finished

    @return function that serves the workspace names
    """
    def inner_function():
        name = unique_workspace_name()
        temp_workspaces.append(name)
        return name
    return inner_function


def insert_bank_numbers(input_workspace: Union[str, Workspace2D],
                        grouping_workspace: Union[str, WorkspaceGroup]):
    r"""
    Label each spectra according to each bank number

    @param input_workspace : workpace with spectra to be labelled
    @param grouping_workspace : group workspace has the group ID (bank number) for each pixel
    """
    input_handle, grouping_handle = mtd[str(input_workspace)], mtd[str(grouping_workspace)]
    group_ids = sorted(list(set(grouping_handle.extractY().flatten())))
    assert input_handle.getNumberHistograms() == len(group_ids)
    axis = TextAxis.create(len(group_ids))
    [axis.setLabel(index, f'bank{group_id}') for index, group_id in enumerate(group_ids)]
    input_handle.replaceAxis(1, axis)


class CORELLIPowderCalibrationCreate(DataProcessorAlgorithm):

    peak_shapes = ['Gaussian']
    bank_count = 92
    _banks = [f'bank{i}/sixteenpack' for i in range(1, bank_count)]
    adjustment_items = ['Component', 'Xposition', 'Yposition', 'Zposition',
                        'XdirectionCosine', 'YdirectionCosine', 'ZdirectionCosine', 'RotationAngle']

    @staticmethod
    def fitted_in_tof(fitted_in_dspacing: Union[str, Workspace2D],
                      difc: Union[str, TableWorkspace],
                      output_workspace: str,
                      group_workspace: Union[str, WorkspaceGroup] = None):
        r"""
        Create a workspace of fitted spectra in TOF

        @param fitted_in_dspacing : workspace of fitted spectra in d-spacing
        @param difc : table of DIFC parameters
        @param output_workspace : name for the workspace of fitted spectra in TOF
        @param group_workspace : if provided, add `output_workspace` to `group_workspace`

        @returns reference to the `output_workspace`
        """
        dspacing_workspace, difc_workspace = mtd[str(fitted_in_dspacing)], mtd[str(difc)]

        # Validate number of histograms in fitted_in_dspacing is smae as in  difc
        error_message = f'{dspacing_workspace} and {difc_workspace} have different number of spectra'
        assert dspacing_workspace.getNumberHistograms() == difc_workspace.getNumberHistograms(), error_message

        # Divide fitted_in_dspacing by difc, and assign output_workspace to it
        output = Multiply(LHSWokspace=difc_workspace, RHSWorkspace=dspacing_workspace, OutputWorkspace=output_workspace)

        # if group_workspace is not None, add output_workspace to group_workspace
        if group_workspace is not None:
            mtd[str(group_workspace)].add(output_workspace)

        return output

    def category(self):
        return 'Diffraction\\Reduction'

    def seeAlso(self):
        return ['PDCalibration', 'AlignDetectors', 'AlignComponents',
                'CORELLIPowderCalibrationDatabase', 'CORELLIPowderCalibrationApply']

    def summary(self):
        return "Calibrate the detector pixels and create a calibration table"

    def PyInit(self):
        self.declareProperty(
            IEventWorkspaceProperty('InputWorkspace', '',
                                    direction=Direction.Input,
                                    validator=WorkspaceUnitValidator('TOF')),
            doc='Powder event data, ideally from a highly symmetric space group',
        )
        self.declareProperty(name='OutputWorkspacesPrefix', defaultValue='pdcal_', direction=Direction.Input,
                             doc="Prefix to be added to output workspaces")
        # Tube Calibration properties
        self.declareProperty(name='TubeDatabaseDir', defaultValue='', direction=Direction.Input,
                             doc='path to database containing detector heights')
        # PDCalibration properties exposed, grouped
        property_names = ['TofBinning', 'PeakFunction', 'PeakPositions']
        self.copyProperties('PDCalibration', property_names)
        [self.setPropertyGroup(name, 'PDCalibration') for name in property_names]
        # AlignComponents properties exposed, grouped
        property_names = ['SourceMaxTranslation', 'ComponentList', 'ComponentMaxTranlation', 'ComponentMaxRotation']
        self.declareProperty(name='SourceMaxTranslation', defaultValue=0.1, validator=FloatBoundedValidator(0, 0.5),
                             doc='Maximum adjustment of source position along the beam (Z) axis (m)')
        self.declareProperty(StringArrayProperty('ComponentList', values=self._banks, direction=Direction.Input),
                             doc='Comma separated list on banks to refine')
        self.declareProperty(name='ComponentMaxTranlation', defaultValue=0.01, validator=FloatBoundedValidator(0.0, 0.1),
                             doc='Maximum translation of each component along either of the X, Y, Z axes (m)')
        self.declareProperty(name='ComponentMaxRotation', defaultValue=2.0, validator=FloatBoundedValidator(0.0, 10.0),
                             doc='Maximum rotation of each component along either of the X, Y, Z axes (deg)')
        [self.setPropertyGroup(name, 'AlignComponents') for name in property_names]


    def PyExec(self):
        temporary_workspaces = []
        self.temp_ws = temp_workspace_generator(temporary_workspaces)  # name generator for temporary workpaces

        prefix_output = self.getProperty('OutputWorkspacesPrefix').value
        progress_percent_start, progress_percent_end, reports_count = 0.0, 0.01, 5
        progress = Progress(self, progress_percent_start, progress_percent_end, reports_count)
        input_workspace = self.getProperty('InputWorkspace').value
        orientation_diagnostics = list()  # list workspace names that analyze the orientation of the banks

        # Create a grouping workspace whereby we group detectors by banks
        grouping_workspace = self.temp_ws()  # a temporary name
        CreateGroupingWorkspace(InputWorkspace=input_workspace,
                                OutputWorkspace=grouping_workspace,
                                GroupDetectorsBy='bank')

        # Remove delayed emission time from the moderator
        kwargs = dict(InputWorkspace=input_workspace, Emode='Elastic', OutputWorkspace=input_workspace)
        self.run_algorithm('ModeratorTzero', 0.01, 0.1, **kwargs)
        progress.report('ModeratorTzero has been applied')

        # Correct detector heights
        tube_calibration_database = self.getProperty('TubeDatabaseDir').value
        yesno = 'not '  # flags if the tube calibration has been applied to the workspace
        if os.path.isdir(tube_calibration_database):
            tube_calibration_table_name = self.temp_ws()
            tube_calibration_table, tube_calibration_mask = load_calibration_set(input_workspace,
                                                                                 tube_calibration_database,
                                                                                 tube_calibration_table_name, self.temp_ws())
            if tube_calibration_table is not None:
                apply_calibration(input_workspace, tube_calibration_table)
                yesno = ''
        progress.report(f'Tube calibration has {yesno}been applied')

        # Find dSpacing to TOF conversion DIFC parameter
        difc_table = f'{prefix_output}PDCalibration_difc'
        diagnostics_workspaces = prefix_output + 'PDCalibration_diagnostics'  # group workspace
        kwargs = dict(InputWorkspace=input_workspace,
                      TofBinning=self.getProperty('TofBinning').value,
                      PeakFunction=self.getProperty('PeakFunction').value,
                      PeakPositions=self.getProperty('PeakPositions').value,
                      CalibrationParameters='DIFC',
                      OutputCalibrationTable=difc_table,
                      DiagnosticWorkspaces=diagnostics_workspaces)
        PDCalibration(**kwargs)
        progress.report('PDCalibration has been applied')

        # Create one spectra in d-spacing for each bank using the original instrument geometry
        self.fitted_in_dspacing(fitted_in_tof=prefix_output + 'PDCalibration_diagnostics_fitted',
                                workspace_with_instrument=input_workspace,
                                output_workspace=prefix_output + 'PDCalibration_peaks_original',
                                grouping_workspace=grouping_workspace)
        orientation_diagnostics.append(prefix_output + 'PDCalibration_peaks_original')

        # Find the peak centers in TOF units, for the peaks found at each pixel
        peak_centers_in_tof = prefix_output + 'PDCalibration_diagnostics_tof'
        self.centers_in_tof(prefix_output + 'PDCalibration_diagnostics_dspacing',
                            difc_table, peak_centers_in_tof)
        mtd[diagnostics_workspaces].add(peak_centers_in_tof)

        # Find the Histogram of peak deviations (in d-spacing units)
        # for each bank, using the original instrument geometry
        self.histogram_peak_deviations(prefix_output + 'PDCalibration_diagnostics_tof',
                                       input_workspace,
                                       prefix_output + 'peak_deviations_original',
                                       grouping_workspace)
        orientation_diagnostics.append(prefix_output + 'peak_deviations_original')

        # store the DIFC and DIFC_mask wokspace created by PDCalibration in the
        # diagnostics workspace
        mtd[diagnostics_workspaces].add(difc_table)
        mtd[diagnostics_workspaces].add(difc_table + '_mask')

        # Reposition the source along the beam axis
        dz = self.getProperty('SourceMaxTranslation').value
        adjustments_table_name = f'{prefix_output}adjustments'
        kwargs = dict(Workspace=input_workspace,
                      CalibrationTable=difc_table,
                      MaskWorkspace=f'{difc_table}_mask',
                      AdjustmentsTable=adjustments_table_name,
                      FitSourcePosition=True,
                      FitSamplePosition=False,
                      Zposition=True, MinZPosition=-dz, MaxZPosition=dz)  # allow +-10cm wiggling for the source position
        self.run_algorithm('AlignComponents', 0.8, 1.0, **kwargs)

        # Position and rotate the each bank, only after the source has repositioned
        # The instrument embbeded in `input_workspace` is adjusted in-place
        dt = self.getProperty('ComponentMaxTranlation').value  # maximum translation along either axis
        dr = self.getProperty('ComponentMaxRotation').value  # maximum rotation along either axis
        kwargs = dict(Workspace=input_workspace,
                      CalibrationTable=difc_table,
                      MaskWorkspace=f'{difc_table}_mask',
                      AdjustmentsTable=adjustments_table_name + '_banks',
                      FitSourcePosition=False,
                      FitSamplePosition=False,
                      ComponentList=self.getProperty('ComponentList').value,
                      Xposition=True, MinXPosition=-dt, MaxXPosition=dt,  # allow +-dt wiggling
                      Yposition=True, MinYPosition=-dt, MaxYPosition=dt, 
                      Zposition=True, MinZPosition=-dt, MaxZPosition=dt,
                      AlphaRotation=True, MinAlphaRotation=-dr, MaxAlphaRotation=dr,  # allow +-dr degrees wiggling
                      BetaRotation=True, MinBetaRotation=-dr, MaxBetaRotation=dr,
                      GammaRotation=True, MinGammaRotation=-dr, MaxGammaRotation=dr,
                      EulerConvention='YXZ')
        self.run_algorithm('AlignComponents', 0.8, 1.0, **kwargs)
        progress.report('AlignComponents has been applied')

        # AlignComponents produces two unwanted workspaces
        temporary_workspaces.extend(['alignedWorkspace', 'calWS'])

        # Append the banks table to the source table, then delete the banks table
        self._append_second_to_first(adjustments_table_name, adjustments_table_name + '_banks')

        # Create one spectra in d-spacing for each bank using the adjusted instrument geometry.
        # The spectra can be compare to those of prefix_output + 'PDCalibration_peaks_original'
        self.fitted_in_dspacing(fitted_in_tof=prefix_output + 'PDCalibration_diagnostics_fitted',
                                workspace_with_instrument=input_workspace,
                                output_workspace=prefix_output + 'PDCalibration_peaks_adjusted',
                                grouping_workspace=grouping_workspace)
        orientation_diagnostics.append(prefix_output + 'PDCalibration_peaks_adjusted')

        # Find the Histogram of peak deviations (in d-spacing units)
        # for each bank, using the adjusted instrument geometry
        self.histogram_peak_deviations(prefix_output + 'PDCalibration_diagnostics_tof',
                                       input_workspace,
                                       prefix_output + 'peak_deviations_adjusted',
                                       grouping_workspace)
        orientation_diagnostics.append(prefix_output + 'peak_deviations_adjusted')

        # Create a WorkspaceGroup with the orientation diagnostics
        GroupWorkspaces(InputWorkspaces=orientation_diagnostics,
                        OutputWorkspace=prefix_output + 'orientation_diagnostics')

        # clean up at the end (only happens if algorithm completes sucessfully)
        [DeleteWorkspace(name) for name in temporary_workspaces if AnalysisDataService.doesExist(name)]


    def run_algorithm(self, name, start_progress, end_progress, **kwargs):
        algorithm_align = self.createChildAlgorithm(name=name,
                                                    startProgress=start_progress, endProgress=end_progress,
                                                    enableLogging=True)
        [algorithm_align.setProperty(name, value) for name, value in kwargs.items()]
        algorithm_align.execute()
        logger.notice(f'{name} has executed')

    def _append_second_to_first(self, first, second, delete_second=True):
        r"""
        Append the rows of the second table into to the first table

        @param delete_second : delete second table after appending to first
        """
        first_workspace, second_workspace = mtd[str(first)], mtd[str(second)]
        for row in second_workspace:
            row_values = [row[item] for item in self.adjustment_items]
            first_workspace.addRow(row_values)
        if delete_second:
            DeleteWorkspace(second)

    def fitted_in_dspacing(self, fitted_in_tof: Union[str, Workspace2D],
                           workspace_with_instrument: Union[str, Workspace],
                           output_workspace: str,
                           dspacing_bin_width: float = 0.001,
                           grouping_workspace: Optional[str] = None) -> None:
        r"""
        Create one spectra of fitted peaks in d-spacing for each pixel or group of detectors

        This algorithm will perform a unit converstion from TOF to d-spacing. The instrument geometry
        for the conversion is proviged by the instrument embedded in `workspace_with_instrument`,
        instead of the instrument embedded in `fitted_in_tof`.

        @param fitted_in_tof : fitted spectra in TOF, one per pixel
        @param workspace_with_instrument : workspace providing the instrument with the desired geometry
        @param output_workspace : name of the workspace containing the spectra in d-spacing
        @param dspacing_bin_width : bin width for the spectra of `output_workspace`
        @param grouping_workspace: if provided, generate one spectra in d-spacing for each of the
            groups specified in this grouping workspace.
        """
        CloneWorkspace(InputWorkspace=fitted_in_tof, OutputWorkspace=output_workspace)
        CopyInstrumentParameters(InputWorkspace=workspace_with_instrument, OutputWorkspace=output_workspace)
        ConvertUnits(InputWorkspace=output_workspace,
                     OutputWorkspace=output_workspace,
                     Target='dSpacing',
                     Emode='Elastic')
        # Rebin spectra to common bin boundaries. This is required if grouping is desired
        peak_centers_reference = self.getProperty('PeakPositions').value
        dspacing_extra = 1.0  # 1 Angstrom
        dspacing_min = max(0, min(peak_centers_reference) - dspacing_extra)
        dspacing_max = max(peak_centers_reference) + dspacing_extra
        Rebin(InputWorkspace=output_workspace, OutputWorkspace=output_workspace,
              Params=[dspacing_min, dspacing_bin_width, dspacing_max])  # bin width is 0.001 Angstroms
        # Optional groping into banks
        if grouping_workspace is not None:
            GroupDetectors(InputWorkspace=output_workspace, OutputWorkspace=output_workspace,
                           CopyGroupingFromWorkspace=grouping_workspace)
            insert_bank_numbers(output_workspace, grouping_workspace) # label each spectrum with the bank number

    def centers_in_tof(self, centers_in_dspacing, difc, output_workspace):
        r"""
        Convert the peak centers in dSpacing units (Angstroms) to TOF units (microseconds)

        @param centers_in_dspacing : "_dspacing" diagnostics table workspace, output from PDCalibration
        @param difc: "_difc" table workspace, output from PDCalibration
        @param output_workspace: name of the output table, equal to `centers_in_dspacing` except the
            peak centers are now in TOF units
        """
        dspacing_table, difc_table = mtd[str(centers_in_dspacing)], mtd[str(difc)]
        assert dspacing_table.rowCount() == difc_table.rowCount()

        # Create empty table for centers in TOF
        table = CreateEmptyTableWorkspace(OutputWorkspace=output_workspace)
        column_names, column_types = dspacing_table.getColumnNames(), dspacing_table.columnTypes()
        for column_name, column_type in zip(column_names, column_types):
            table.addColumn(name=column_name, type=column_type)

        # Populate the table
        difc_values = np.array(difc_table.column('difc'))
        dspacing_dict = dspacing_table.toDict()  # the table in a data structure we can modify
        column_names = dspacing_table.getColumnNames()
        column_peaks = [name for name in column_names if '@' in name]
        for column in column_peaks:
            dspacing_dict[column] = (difc_values * np.array(dspacing_dict[column])).tolist()  # peaks now in TOF
        for row_index in range(dspacing_table.rowCount()):
            row_new = [dspacing_dict[column][row_index] for column in column_names]
            table.addRow(row_new)

    def histogram_peak_deviations(self,
                                  peak_centers_in_tof: Union[str, TableWorkspace],
                                  workspace_with_instrument: Union[str, Workspace],
                                  output_workspace: str,
                                  grouping_workspace: Union[str, WorkspaceGroup],
                                  deviation_params: List[float] = [-0.1, 0.0001, 0.1]):
        r"""
        Find deviations of the fitted peak centers with respect to the reference values,
        in d-spacing units. Histogram these deviations for all peaks found on each bank

        @param peak_centers_in_tof: table containing the centers of the fitted peaks found
            on each pixel, in TOF units
        @param workspace_with_instrument: any workspace whose embedded instrument will
            be used to calculate the DIFC values per pixel
        @param output_workspace: workspace containing the histograms of peak deviations per bank
        @param grouping_workspace: workspace assigning group ID's (bank numbers) to each pixel
        @param deviation_params: a triad of first histogram boundary, bin width, and
            last histogram boundary, in Angstroms
        """
        # Find DIFC values using the geometry of the instrument embbeded in `workspace_with_instrument`
        difc_workspace = self.temp_ws()  # we need a temporary workpace
        CalculateDIFC(workspace_with_instrument, OutputWorkspace=difc_workspace)
        difc_values = mtd[difc_workspace].extractY().flatten()

        # Save the contents of the table to a python dictionary which we can overwrite easily
        tof_table = mtd[str(peak_centers_in_tof)]
        tof_dict = tof_table.toDict()  # the table as a data structure we can modify

        # Calculate the peak deviations with respect to the reference peak centers, in d-spacing units
        column_names = tof_table.getColumnNames()
        column_peaks = [name for name in column_names if '@' in name]  # column containing the peak centers
        for column in column_peaks:
            dspacing_reference = float(column.replace('@', ''))  # the column name has the reference peak center
            # peak deviations, in d-spacing units, that we use to overwrite tof_dic
            tof_dict[column] = (np.array(tof_dict[column]) / difc_values - dspacing_reference).tolist()

        # Extract the group ID's, which typically corresponds to bank numbers
        # grouping_workspace_y contain the group ID (bank number) of each pixel
        grouping_workspace_y = [int(n) for n in mtd[str(grouping_workspace)].extractY().flatten()]
        group_ids = sorted(list(set(grouping_workspace_y)))  # list of group ID's (bank numbers)
        
        # List all the peak deviations within a group (bank)
        deviations_in_group = {id: [] for id in group_ids}
        for row_index in range(tof_table.rowCount()):  # iterate over each pixel
            group_id = grouping_workspace_y[row_index]  # group ID (bank number) of the current pixel
            # find the peak deviations for all peaks that were found in the current pixel
            deviations_in_pixel = np.array([tof_dict[column][row_index] for column in column_peaks])
            # `nan` is assigned to listed peaks missing in the current pixel. We must get rid of them
            deviations_in_group[group_id].extend(deviations_in_pixel[~np.isnan(deviations_in_pixel)].tolist())

        # Histogram the deviations for all pixels within a group (bank)
        start, step, stop = deviation_params  # start and stop are first and last histogram boundaries
        bins = np.arange(start, stop + step / 2, step)
        histograms = dict()  # one histogram per group ID (bank)
        histogram_empty = np.zeros(len(bins) - 1)  # for banks with no peaks, for instance, whole banks masked
        for group_id, deviations in deviations_in_group.items():
            if len(deviations) == 0:  # no peaks in the group (bank), thus no deviations
                histogram = histogram_empty
            else:
                histogram = np.histogram(deviations, bins)[0]
            histograms[group_id] = histogram
            
        # Create a workspace with the histograms
        spectra = spectra = np.array(list(histograms.values())).flatten()  # single list needed
        CreateWorkspace(DataX=bins, DataY=spectra, NSpec=len(group_ids), UnitX='dSpacing',
                        WorkspaceTitle='Peak deviations per pixel group', OutputWorkspace=output_workspace)
        insert_bank_numbers(output_workspace, grouping_workspace) # label each spectrum with the bank number

AlgorithmFactory.subscribe(CORELLIPowderCalibrationCreate)
