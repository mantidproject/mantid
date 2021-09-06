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
    """
    @brief Calculate the alpha value based on the peak position and slicepoint
    @detail If a value cannot be computed, for zero dimension width for example, then
    ALPHA_MAX is returned.
    returns ALPHA_MAX when z == slicepoint
    rturns ALPHA_MIN when |z - slicepoint| == slicedim_width * VIEW_FRACTION
    Example: if slice_dim_width=0.2 Angstroms, a peak will not be shown when its z coordinate
    lies outside the [0.497, 0.503] range.
    :param z: Z position of center out of slice plane
    :param slicepoint: float giving current slice point
    :param slicedim_width:
    :returns: float transparency value in range < ALPHA_MAX
    """
    # Apply a linear transform to convert from a distance to an opacity between
    # alpha min & max
    try:
        gradient = (ALPHA_MIN - ALPHA_MAX) / (slicedim_width * VIEW_FRACTION)  # negative gradient
        distance = abs(slicepoint - z)
        return (gradient * distance) + ALPHA_MAX
    except ArithmeticError:
        return ALPHA_MAX
