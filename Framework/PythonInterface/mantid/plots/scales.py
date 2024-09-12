# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid package
"""
Defines a set of custom axis scales
"""

from matplotlib.scale import ScaleBase
from matplotlib.ticker import AutoLocator, NullFormatter, NullLocator, ScalarFormatter
from matplotlib.transforms import Transform
import numpy as np


class PowerScale(ScaleBase):
    """Scales the data using a power-law scaling: x^gamma"""

    # Name required by register_scale. Use this is set_*scale
    # commands
    name = "power"

    def __init__(self, _axis, **kwargs):
        """
        Any keyword arguments passed to ``set_xscale`` and
        ``set_yscale`` will be passed along to the scale's
        constructor.

        gamma: The power used to scale the data.
        """
        super(PowerScale, self).__init__(_axis)
        gamma = kwargs.pop("gamma", None)
        if gamma is None:
            raise ValueError("power scale must specify gamma value")
        self._gamma = float(gamma)

    def get_transform(self):
        """
        Return a PowerTransform that does the actual scaling
        """
        return PowerScale.PowerTransform(self._gamma)

    def set_default_locators_and_formatters(self, axis):
        """
        Set the locators and formatters to automatic and scalar
        """
        axis.set_major_locator(AutoLocator())
        axis.set_major_formatter(ScalarFormatter())
        axis.set_minor_locator(NullLocator())
        axis.set_minor_formatter(NullFormatter())

    def limit_range_for_scale(self, vmin, vmax, minpos):
        """
        Limit the domain to positive values for even or fractional
        indices
        """
        if not self._gamma.is_integer() or self._gamma % 2 == 0:
            if not np.isfinite(minpos):
                minpos = 1e-300  # This value should rarely if ever
                # end up with a visible effect.

            return (minpos if vmin <= 0 else vmin, minpos if vmax <= 0 else vmax)
        else:
            return vmin, vmax

    class PowerTransform(Transform):
        # There are two value members that must be defined.
        # ``input_dims`` and ``output_dims`` specify number of input
        # dimensions and output dimensions to the transformation.
        # These are used by the transformation framework to do some
        # error checking and prevent incompatible transformations from
        # being connected together.  When defining transforms for a
        # scale, which are, by definition, separable and have only one
        # dimension, these members should always be set to 1.
        input_dims = 1
        output_dims = 1
        is_separable = True
        has_inverse = True

        def __init__(self, gamma):
            super(PowerScale.PowerTransform, self).__init__()
            self._gamma = gamma

        def transform_non_affine(self, a):
            """Apply the transform to the given data array"""
            with np.errstate(divide="ignore", invalid="ignore"):
                out = np.power(a, self._gamma)
            if not self._gamma.is_integer():
                # negative numbers to power of fractions are undefined
                # clip them to 0
                out[a <= 0] = 0
            return out

        def inverted(self):
            """
            Return the type responsible for inverting the transform
            """
            return PowerScale.InvertedPowerTransform(self._gamma)

    class InvertedPowerTransform(Transform):
        input_dims = 1
        output_dims = 1
        is_separable = True
        has_inverse = True

        def __init__(self, gamma):
            super(PowerScale.InvertedPowerTransform, self).__init__()
            self._gamma = gamma

        def transform_non_affine(self, a):
            if not self._gamma.is_integer() or self._gamma % 2 == 0:
                with np.errstate(divide="ignore", invalid="ignore"):
                    out = np.power(a, 1.0 / self._gamma)
                # clip negative values to 0
                out[a <= 0] = 0
            else:
                # negative numbers to power of non-integers are undefined and np.power
                # returns nan. In the case of where we have a fractional power with
                # an odd denominator then we can write the power as
                #     (-a)^(1/b) = (-1)^1(a^(1/b)) = -1*a^(1/b)
                negative_indices = a < 0.0
                if np.any(negative_indices):
                    out = np.copy(a)
                    np.negative(a, where=negative_indices, out=out)
                    np.power(out, 1.0 / self._gamma, out=out)
                    np.negative(out, where=negative_indices, out=out)
                else:
                    out = np.power(a, 1.0 / self._gamma)

            return out

        def inverted(self):
            return PowerScale.PowerTransform(self._gamma)


class SquareScale(PowerScale):
    # Convenience type for square scaling
    name = "square"

    def __init__(self, axis, **kwargs):
        kwargs["gamma"] = 2
        super(SquareScale, self).__init__(axis, **kwargs)
