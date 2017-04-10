.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm converts a SANS workspace in wavelength to a workspace in momentum transfer. The conversion is either provided by :ref:`algm-Q1D` or by :ref:`algm-Qxy`. The settings for the algorithm
are provided by the state object. Besides the algorithm which is to be converted three types of 
adjustment workspaces can be provided:

- Wavelength adjustment worspace which performs a correction on the bins but is the same for each pixel.
- Pixel adjustment workspace which performs a correction on each pixel but is the same for each bin.
- Wavelength and pixel adjustment workspace which performs a correction on the bins and pixels individually.

Note that the *OutputParts* option allows for extracting the count and normalization workspaces which are being produced by :ref:`algm-Q1D` or by :ref:`algm-Qxy`.

Currently the mask mechanism is implemented for **SANS2D**, **LOQ** and **LARMOR**.


Relevant SANSState entries for SANSConvertToQ
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The required information for the momentum transfer conversion is retrieved from a state object.

The elementsconert are:

+---------------------------------+------------------------------+---------------------------------------------------------------------+------------+---------------+
| Entry                           | Type                         | Description                                                         | Mandatory  | Default value |
+=================================+==============================+=====================================================================+============+===============+
| reduction_dimensionality        | ReductionDimensionality enum | The dimensionality of the reduction                                 | No         | None          |
+---------------------------------+------------------------------+---------------------------------------------------------------------+------------+---------------+
| use_gravity                     | Bool                         | If a gravity correction is to be applied                            | No         | False          |
+---------------------------------+------------------------------+---------------------------------------------------------------------+------------+---------------+
| gravity_extra_length            | Float                        | The additional length in m if a gravity correction is to be applied | No         | 0.          |
+---------------------------------+------------------------------+---------------------------------------------------------------------+------------+---------------+
| radius_cutoff                   | Float                        | A radius cutoff in m                                                | No         | 0.          |
+---------------------------------+------------------------------+---------------------------------------------------------------------+------------+---------------+
| wavelength_cutoff               | RangeStepType enum           | A wavelength cutoff                                                 | No         | 0.          |
+---------------------------------+------------------------------+---------------------------------------------------------------------+------------+---------------+
| q_min                           | Float                        | The minimal momentum transfer (only relevant for 1D reductions)     | Yes, if 2D setting has not been provided         | None          |
+---------------------------------+------------------------------+---------------------------------------------------------------------+------------+---------------+
| q_max                           | Float                        | The maximal momentum transfer (only relevant for 1D reductions)     | Yes, if 2D setting has not been provided          | None          |
+---------------------------------+------------------------------+---------------------------------------------------------------------+------------+---------------+
| q_1d_rebin_string               | String                       | Rebinning parameters for momentum transfer                          | Yes, if 2D setting has not been provided          | None          |
+---------------------------------+------------------------------+---------------------------------------------------------------------+------------+---------------+
| q_xy_max                        | Float                        | Maximal momentum transfer  (only relevant for 2D reduction)         | Yes, if 1D setting has not been provided          | None          |
+---------------------------------+------------------------------+---------------------------------------------------------------------+------------+---------------+
| q_xy_step                       | Float                        | Momentum transfer step (only relevant for 2D reduction)             | Yes, if 1D setting has not been provided          | None          |
+---------------------------------+------------------------------+---------------------------------------------------------------------+------------+---------------+
| q_xy_step_type                  | RangeStepType enum           | Momentum transfer step type (only relevant for 2D reduction)        | Yes, if 1D setting has not been provided          | None          |
+---------------------------------+------------------------------+---------------------------------------------------------------------+------------+---------------+
| use_q_resolution                | Bool                         | If momentum transfer resolution calculation is to be used           | No        | None          |
+---------------------------------+------------------------------+---------------------------------------------------------------------+------------+---------------+
| q_resolution_collimation_length | Float                        | The collimation length in m                                         | No         | None          |
+---------------------------------+------------------------------+---------------------------------------------------------------------+------------+---------------+
| q_resolution_delta_r            |  Float                       | The virtual ring width on the detector                              | No         | None          |
+---------------------------------+------------------------------+---------------------------------------------------------------------+------------+---------------+
| moderator_file                  |  String                      | Moderator file with time spread information                         | No          | None          |
+---------------------------------+------------------------------+---------------------------------------------------------------------+------------+---------------+
| q_resolution_a1                 |  Float                       | Source aperture radius                                              | Yes, if momentum transfer resolution is selected and retangular aperture is not set           | None          |
+---------------------------------+------------------------------+---------------------------------------------------------------------+------------+---------------+
| q_resolution_a2                 |  Float                       | Sample aperture radius                                              | Yes, if momentum transfer resolution is selected and retangular aperture is not set          | None          |
+---------------------------------+------------------------------+---------------------------------------------------------------------+------------+---------------+
| moderator_file                  |  String                      | Moderator file with time spread information                         | No          | None          |
+---------------------------------+------------------------------+---------------------------------------------------------------------+------------+---------------+
| q_resolution_h1                 |  Float                       | Source aperture height (rectangular)                                | Yes, if momentum transfer resolution is selected and circular aperture is not set           | None          |
+---------------------------------+------------------------------+---------------------------------------------------------------------+------------+---------------+
| q_resolution_h2                 |  Float                       | Sample aperture height (rectangular)                                | Yes, if momentum transfer resolution is selected and circular aperture is not set          | None          |
+---------------------------------+------------------------------+---------------------------------------------------------------------+------------+---------------+
| q_resolution_w1                 |  Float                       | Source aperture width (rectangular)                                 | Yes, if momentum transfer resolution is selected and circular aperture is not set           | None          |
+---------------------------------+------------------------------+---------------------------------------------------------------------+------------+---------------+
| q_resolution_w2                 |  Float                       | Sample aperture width (rectangular)                                 | Yes, if momentum transfer resolution is selected and circular aperture is not set          | None          |
+---------------------------------+------------------------------+---------------------------------------------------------------------+------------+---------------+

Note that the momentum transfer resolution calculation is only applicable for 1D reductions.

.. categories::

.. sourcelink::
