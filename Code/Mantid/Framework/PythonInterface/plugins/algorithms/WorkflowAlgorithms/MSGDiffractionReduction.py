from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *
from mantid import config

import os.path, math


class MSGDiffractionReduction(PythonAlgorithm):

    def category(self):
        return 'Diffraction;PythonAlgorithms'


    def summary(self):
        return 'Calculates the scattering & transmission for Indirect Geometry spectrometers.'


    def PyInit(self):
        self.declareProperty(StringArrayProperty(name='InputFiles'),
                             doc='Comma separated list of input files.')

        self.declareProperty(name='SumFiles', defaultValue=False,
                             doc='Enabled to sum spectra from each input file.')

        self.declareProperty(name='IndividualGrouping', defaultValue=False,
                             doc='Do not group results into a single spectra.')

        self.declareProperty(name='Instrument', defaultValue='IRIS',
                             validator=StringListValidator(['IRIS', 'OSIRIS', 'TOSCA', 'VESUVIO']),
                             doc='Instrument used for run')

        self.declareProperty(name='Mode', defaultValue='diffspec',
                             validator=StringListValidator(['diffspec', 'diffonly']),
                             doc='Diffraction mode used')

        self.declareProperty(IntArrayProperty(name='DetectorRange'),
                             doc='Range of detectors to use.')

        self.declareProperty(name='RebinParam', defaultValue='',
                             doc='Rebin parameters.')

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', '',
                             direction=Direction.Output),
                             doc='Group name for the result workspaces.')

        self.declareProperty(StringArrayProperty(name='SaveFormats'),
                             doc='Save formats to save output in.')


    def validateInputs(self):
        """
        Checks for issues with user input.
        """
        issues = dict()

        # Validate input files
        input_files = self.getProperty('InputFiles').value
        if len(input_files) == 0:
            issues['InputFiles'] = 'InputFiles must contain at least one filename'

        # Validate detector range
        detector_range = self.getProperty('DetectorRange').value
        if len(detector_range) != 2:
            issues['DetectorRange'] = 'DetectorRange must be an array of 2 values only'
        else:
            if detector_range[0] > detector_range[1]:
                issues['DetectorRange'] = 'DetectorRange must be in format [lower_index,upper_index]'

        # Validate save formats
        save_formats = self.getProperty('SaveFormats').value
        valid_formats = ['gss', 'nxs', 'ascii']
        for s_format in save_formats:
            if s_format not in valid_formats:
                issues['SaveFormats'] = 'Contains invalid save formats'
                break

        return issues


    def PyExec(self):
        from IndirectCommon import StartTime, EndTime
        from IndirectDiffractionReduction import MSGDiffractionReducer

        StartTime('MSGDiffractionReduction')

        input_files = self.getProperty('InputFiles').value
        sum_files = self.getProperty('SumFiles').value
        individual_grouping = self.getProperty('IndividualGrouping').value
        instrument_name = self.getPropertyValue('Instrument')
        mode = self.getPropertyValue('Mode')
        detector_range = self.getProperty('DetectorRange').value
        rebin_string = self.getPropertyValue('RebinParam')
        output_ws_group = self.getPropertyValue('OutputWorkspace')
        save_formats = self.getProperty('SaveFormats').value

        ipf_filename = instrument_name + '_diffraction_' + mode + '_Parameters.xml'

        reducer = MSGDiffractionReducer()
        reducer.set_instrument_name(instrument_name)
        reducer.set_detector_range(int(detector_range[0] - 1), int(detector_range[1] - 1))
        reducer.set_parameter_file(ipf_filename)
        reducer.set_sum_files(sum_files)
        reducer.set_save_formats(save_formats)

        if individual_grouping:
            reducer.set_grouping_policy('Individual')

        for in_file in input_files:
            reducer.append_data_file(in_file)

        if rebin_string != '':
            reducer.set_rebin_string(rebin_string)

        if instrument_name == 'VESUVIO':
            reducer.append_load_option('Mode', 'FoilOut')

        reducer.reduce()

        result_ws_list = reducer.get_result_workspaces()
        GroupWorkspaces(InputWorkspaces=result_ws_list, OutputWorkspace=output_ws_group)
        self.setProperty('OutputWorkspace', output_ws_group)

        EndTime('MSGDiffractionReduction')


AlgorithmFactory.subscribe(MSGDiffractionReduction)
