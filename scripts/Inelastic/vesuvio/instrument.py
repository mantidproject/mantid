#pylint: disable=too-few-public-methods
"""
Holds an instrument class for VESUVIO
"""


class VESUVIO(object):
    """
        Contains all the Vesuvio specific data
    """

    def __init__(self):
        self.name = 'Vesuvio'
        # Need to load these from the parameter file

        # Crop range (VMS defaults)
        self.tof_range = [50, 562]

        # Spectra ranges
        self.backward_spectra = (3, 134)
        self.backward_banks = ((3, 46), (47, 90), (91, 134))

        self.forward_spectra = (135, 198)
        self.forward_banks = ((135, 142), (143, 150), (151, 158), (159, 166),
                              (167, 174), (175, 182), (183, 190), (191, 198))
