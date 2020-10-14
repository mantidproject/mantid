# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

# transparency range
ALPHA_MIN, ALPHA_MAX = 0.0, 0.8
# fraction of view occupied by marker
VIEW_FRACTION = 0.015


def compute_alpha(z, slicepoint, slicedim_width):
    """Calculate the alpha value based on the peak position and slicepoint
    :param z: Z position of center out of slice plane
    :param slicepoint: float giving current slice point
    :param slicedim_width:
    :returns: float transparency value in range < ALPHA_MAX
    """
    # Apply a linear transform to convert from a distance to an opacity between
    # alpha min & max
    gradient = (ALPHA_MIN - ALPHA_MAX) / (slicedim_width * VIEW_FRACTION)
    distance = abs(slicepoint - z)
    return (gradient * distance) + ALPHA_MAX
