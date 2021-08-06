# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,invalid-name
import mantid
import numpy as np
import yaml
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

    # TODO - return can be either dictionary or namedtuple
    def parse_yaml(self, file_name: str) -> dict:
        """Parse configuration YAML file

        Parameters
        ----------
        file_name: str
            name of the YAML file that contains the configuration for NOMAD

        Returns
        -------
        dictionary
        """
        with open(file_name, 'r') as stream:
            config = yaml.safe_load(stream)
        return config

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

    # In-progress Task 335
    def determine_tube_threshold(self, vec_intensity, nomad_info, config):
        """

        Refer to diagram
        https://code.ornl.gov/sns-hfir-scse/diffraction/powder/powder-diffraction/
        uploads/99f4b65655e05edd476999942ff6fb98/Step-1.png
        from box 'Select a tube' to 'All tubes finished'


        Parameters
        ----------
        vec_intensity: numpy.ndarray
            solid angle corrected counts. shape = (101378, )

        Returns
        -------

        """
        # Workflow: process full-collimated, half-collimated and not-collimated separately

        # reshape the input intensities to 2D matrix: each row is a tube, i.e., (num_tubes, num_pixel_per_tube)
        # or say (392, 256)
        tube_pixel_intensity_array = vec_intensity.flatten().reshpae((nomad_info.num_tubes,
                                                                      nomad_info.num_pixels_per_tube))

        self._determine_full_collimated_tubes_thresholds(tube_pixel_intensity_array, nomad_info, config)

        self._determine_half_collimated_tubes_thresholds(tube_pixel_intensity_array, nomad_info, config)

        self._determine_none_collimated_tubes_thresholds(tube_pixel_intensity_array, nomad_info, config)

    def _determine_full_collimated_tubes_thresholds(self, vec_intensity, nomad_info, config):
        """Determine the full-collimated tubes' thresholds

        Parameters
        ----------
        vec_intensity
        nomad_info
        config

        Returns
        -------

        """
        # Get the boolean array for full-collimated tubes: shape = (392, )
        full_collimated_tubes = self.get_tubes_by_collimation_status(config)
        full_collimated_pixels = np.repeat(full_collimated_tubes, nomad_info.num_tubes)

        # Calculate median value the tubes
        tube_median_array = np.median(vec_intensity, axis=1)
        tube_median_array = np.repeat(tube_median_array, nomad_info.num_tubes)

        # Condition 3:
        # summed pixel intensity < (1+(low_pixel-1)*3) * m or
        # summed pixel intensity > (1+(high_pixel-1)*3) * m
        # where low_pixel and high_pixel are parameters defined in the provided YAML configuration file above.
        lower_boundaries = (1. + (config.low_pixel - 1) * 3.) * tube_median_array
        upper_boundaries = (1. + (config.high_pixel - 1) * 3.) * tube_median_array

        # Check pixels meets conditions 3 for pixels in full_collimated_tubes
        masked_pixels = np.where((tube_median_array < lower_boundaries or tube_median_array > upper_boundaries)
                                 and full_collimated_pixels)

        return masked_pixels

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
