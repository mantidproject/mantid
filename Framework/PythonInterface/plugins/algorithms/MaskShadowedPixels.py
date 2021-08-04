# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,invalid-name
import mantid
import numpy as np


class MaskShadowedPixels(mantid.api.PythonAlgorithm):

    def category(self):
        return "DataHandling\\Nexus"

    def seeAlso(self):
        return ["MaskShadowedPixels"]

    def name(self):
        return "MaskShadowedPixels"

    def summary(self):
        return "Save constant wavelength powder diffraction data to a GSAS file in FXYE format"

    def PyInit(self):
        self.declareProperty(mantid.api.WorkspaceProperty('InputWorkspace',
                                                          '',
                                                          mantid.kernel.Direction.Input),
                             "Workspace to save")
        self.declareProperty(mantid.api.FileProperty('OutputFilename',
                                                     '',
                                                     action=mantid.api.FileAction.Save,
                                                     extensions=['.gss']),
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
        # Get input
        raw_workspace = self.getProperty('InputWorkspace').value
        assert raw_workspace, 'Input workspace cannot be None'

        # Prepare inputs
        config = self.parse_yaml(self.getProperty('ConfigurationFile').value)
        assert isinstance(config, dict)

        # Get instrument information
        pixel_tube_map, tube_8pack_map, eight_pack_bank_map, collimation_status_8pack_dict =\
            self.get_instrument_info(raw_workspace)
        num_tubes = len(tube_8pack_map)
        num_banks = 6

        # Calculate solid angle
        solid_angle_array = self.calculate_solid_angle(raw_workspace)

        # Divide summed intensity for each pixel by solid angle
        # TODO - process inp;ut workspaces if raw_workspace has multiple bins
        # FIXME - watch out array dimension
        vec_intensity = raw_workspace.extractY()
        vec_intensity /= solid_angle_array

        # calculate summed intensity for each tube
        vec_tube_intensity = self.sum_to_tubes(vec_intensity, pixel_tube_map, num_tubes)
        assert isinstance(vec_tube_intensity, np.ndarray)

        for tube_index in range(num_tubes):
            collimation_status = collimation_status_8pack_dict[tube_index]
            if collimation_status == 'Collimated':
                do_something_1
            elif collimation_status == 'Half Collimated':
                do_something_2
            else:
                # not collimated
                do_something_0

        # END OF Part1 ---

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

        # END OF Part2 ----
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

        # END OF Part3

        # Process for output

        return

    # TODO - return can be either dictionary or namedtuple
    def parse_yaml(self, file_name: str) -> dict:
        """Parse configuration YAML file

        Parameters
        ----------
        file_name

        Returns
        -------
        dictionary

        """

        return dict()

    def calculate_solid_angle(self, workspace):
        """Calculate solid angle for each detector

        Parameters
        ----------
        workspace

        Returns
        -------
        numpy.ndarray
            shape = (N, ) or (N, 1), where N is the number of detectors

        """
        return np.ndarray()

    def sum_to_tubes(self, pixel_count_array, pixel_tube_map_array, num_tubes: int):
        """ Calculate summed intensity for each tube

        Parameters
        ----------
        pixel_count_array
        pixel_tube_map_array: numpy.ndarray
            integer array with size of detectors number and each element as the tube index for each pixel
        num_tubes

        Returns
        -------
        numpy.ndarray

        """
        tube_intensity_array = np.ndarray(shape=(num_tubes, ), dtype='float')

        for tube_index in range(num_tubes):
            counts_sum = np.sum(pixel_count_array[np.where(pixel_tube_map_array == tube_index)])
            tube_intensity_array[tube_index] = counts_sum

        return tube_intensity_array


# Register the algorithm
mantid.api.AlgorithmFactory.subscribe(MaskShadowedPixels)
