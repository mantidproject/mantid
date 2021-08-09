# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,invalid-name
import mantid
import numpy as np
from collections import namedtuple


class MaskShadowedPixels(mantid.api.PythonAlgorithm):
    def category(self):
        return "DataHandling\\Nexus"

    def seeAlso(self):
        return ["MaskShadowedPixels"]

    def name(self):
        return "MaskShadowedPixels"

    def summary(self):
        return "Generate maks file for NOMAD to exclude shadowed pixels."

    def PyInit(self):
        self.declareProperty(mantid.api.WorkspaceProperty('InputWorkspace', '', mantid.kernel.Direction.Input), "Workspace to save")
        self.declareProperty(mantid.api.FileProperty('OutputFilename', '', action=mantid.api.FileAction.Save, extensions=['.gss']),
                             doc='Name of the GSAS file to save to')

        # TODO - Add missing input properties

    def validateInputs(self):
        """Validate input properties
        (virtual method)
        """
        issues = dict()

        return issues

    def PyExec(self):
        """
        Main method to execute the algorithm
        """
        # Prepare input workspace: normalize by solid angle
        raw_workspace = self.getProperty('InputWorkspace').value
        assert raw_workspace, 'Input workspace cannot be None'
        intensity_array = self.prepare_input(raw_workspace)  # return (101378,) array

        # Parse YMAL configuration
        config = self.parse_yaml(self.getProperty('ConfigurationFile').value)
        assert isinstance(config, dict)

        # Get instrument information
        instrument_info = self.set_nomad_constants()

        # Initialize the empty mask list
        mask_array = np.zeros(shape=(instrument_info.num_pixels,), dtype='float')
        assert mask_array

        # Determine thresholds for a given tube (part 1)
        # TASK 335
        self.determine_tube_threshold(intensity_array, instrument_info, config)
        # END OF Part1 ---

        # Calculate the median for a particular Group of banks (part 2)
        # TASK 331
        self.calculate_banks_medians(config)
        # END OF Part2 ----

        # Determine thresholds for a given bank (part 3)
        # TASK 333
        self.determine_banks_thredsholds(config)
        # END OF Part3 ---

        # Process for output

        return

    def prepare_input(self, workspace):
        """Calculate count on each detector pixels and normalized by solid angle correction

        Parameters
        ----------
        workspace

        Returns
        -------
        numpy.ndarray
            shape = (101378,)

        """
        # Calculate the count on each detector pixel
        # Get counts array: shape = (101378, Bins)
        vec_intensity = workspace.extractY()
        # Sum along Y axis (axis = 1): shape = (101378, )
        vec_intensity = vec_intensity.sum(axis=1)

        # Calculate solid angle
        solid_angle_array = self.calculate_solid_angle(workspace).extractY().flatten()

        vec_intensity /= solid_angle_array

        return vec_intensity

    @staticmethod
    def set_nomad_constants():
        """Set NOMAD geometry constants for numpy operation

        Returns
        -------
        namedtutple
            named tuple for NOMAD pixel, tube, 8 pack and bank constants

        """
        info_dict = dict()

        info_dict['num_banks'] = 6
        info_dict['num_8packs_per_bank'] = [0, 6, 15, 23, 30, 45, 49]  # [i, i+1) is the range of 8 packs for bank i
        info_dict['num_8packs'] = 49
        info_dict['num_pixels_per_tube'] = 256
        info_dict['num_tubes_per_8pack'] = 8
        info_dict['num_tubes'] = info_dict['num_8packs'] * info_dict['num_tubes_per_8pack']
        info_dict['num_pixels'] = info_dict['num_tubes'] * info_dict['num_pixels_per_tube']

        # convert to namedtuple and return
        instrument = namedtuple("nomad", info_dict)

        return instrument(**info_dict)

    @staticmethod
    def calculate_solid_angle(workspace):
        """Calculate solid angle for each detector

        Parameters
        ----------


        Returns
        -------
        numpy.ndarray
            shape = (N, ) or (N, 1), where N is the number of detectors

        """
        assert workspace
        # TODO - To be implemented
        return np.ndarray()

    def calculate_banks_medians(self, config):
        # TASK 331
        for bank_index in range(num_banks):
            if bank_index == 0:
                # first bank
                # calculate median value over all tubes contained in the bank
                do_something
            else:
                # calculate median value over all tubes contained in non-collimated eight-packs in bank
                for eight_pack_index in bank_8packs:
                    for tube_index in eight_pack_tubes:
                        if collimation_status_8pack_dict[tube_index] == 'Not Collimated':
                            do_someting

    def determine_banks_thresholds(self, config):
        # TASK 333
        for bank_index in non_flat_panel_banks:
            for eight_pack_index in bank_8packs:
                # FIXME - question: where is the definition of collimated 8packs?  we only have collimated banks
                if collimated_8pack:
                    #
                    do_something
                else:
                    do_something

        for bank_index in flat_panel_banks:
            for eight_pack_index in bank_8packs:
                do_something

    @staticmethod
    def sum_to_tubes(pixel_count_array, instrument_info):
        """ Calculate summed intensity for each tube

        Parameters
        ----------
        pixel_count_array
        instrument_info: namedtuple
            NOMAD inst instrument information

        Returns
        -------
        numpy.ndarray
            shape = (392, )
        """
        # Reshape to tubes
        tube_count_array = pixel_count_array.reshape(shape=(instrument_info.num_tubes,
                                                            instrument_info.num_pixels_per_tube))

        # Sum
        tube_count_array = tube_count_array.sum(axis=1)

        return tube_count_array


# Register the algorithm
mantid.api.AlgorithmFactory.subscribe(MaskShadowedPixels)
