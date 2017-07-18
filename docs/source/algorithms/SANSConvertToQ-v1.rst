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
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The required information for the momentum transfer conversion is retrieved from a state object.

The elements are:

+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| Entry                           | Type           | Description                         | Mandatory          | Default|
+=================================+================+=====================================+====================+========+
| reduction_dimensionality        | Reduction-     | The dimensionality of the reduction | No                 | None   |
|                                 | Dimensionality |                                     |                    |        |
|                                 | enum           |                                     |                    |        |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| use_gravity                     | Bool           | If a gravity correction is to       | No                 | False  |
|                                 |                | be applied                          |                    |        |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| gravity_extra_length            | Float          | The additional length in m if a     | No                 | 0.     |
|                                 |                | gravity correction is to be applied |                    |        |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| radius_cutoff                   | Float          | A radius cutoff in m                | No                 | 0.     |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| wavelength_cutoff               | RangeStepType  | A wavelength cutoff                 | No                 | 0.     |
|                                 | enum           |                                     |                    |        |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| q_min                           | Float          | The minimal momentum transfer       | Yes, if 2D setting | None   |
|                                 |                | (only relevant for 1D reductions)   | has not been prov. |        |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| q_max                           | Float          | The maximal momentum transfer       | Yes, if 2D setting | None   |
|                                 |                | (only relevant for 1D reductions)   | has not been prov. |        |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| q_1d_rebin_string               | String         | Rebinning parameters for momentum   | Yes, if 2D setting | None   |
|                                 |                | transfer                            | has not been prov. |        |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| q_xy_max                        | Float          | Maximal momentum transfer           | Yes, if 1D setting | None   |
|                                 |                | (only relevant for 2D reduction)    | has not been prov. |        |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| q_xy_max                        | Float          | Maximal momentum transfer           | Yes, if 1D setting | None   |
|                                 |                | (only relevant for 2D reduction)    | has not been prov. |        |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| q_xy_step                       | Float          | Momentum transfer step              | Yes, if 1D setting | None   |
|                                 |                | (only relevant for 2D reduction)    | has not been prov. |        |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| q_xy_step_type                  | RangeStepType  | Momentum transfer step type         | Yes, if 1D setting | None   |
|                                 | enum           | (only relevant for 2D reduction)    | has not been prov. |        |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| use_q_resolution                | Bool           | If momentum transfer resolution     | No                 | None   |
|                                 |                | calculation is to be used           |                    |        |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| q_resolution_collimation_length | Float          | The collimation length in m         | No                 | None   |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| q_resolution_delta_r            |  Float         | The virtual ring width on the       | No                 | None   |
|                                 |                | detector                            |                    |        |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| moderator_file                  |  String        | Moderator file with time spread     | No                 | None   |
|                                 |                | information                         |                    |        |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| q_resolution_a1                 |  Float         | Source aperture radius              | If use_q_resolution| None   |
|                                 |                | information                         | is set and rect.   |        |
|                                 |                | information                         | app. is not set    |        |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| q_resolution_a2                 |  Float         | Sample aperture radius              | If use_q_resolution| None   |
|                                 |                | information                         | is set and rect.   |        |
|                                 |                | information                         | app. is not set    |        |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| q_resolution_h1                 |  Float         | Source aperture height              | If use_q_resolution| None   |
|                                 |                | (rectangular)                       | is set and circ.   |        |
|                                 |                |                                     | app. is not set    |        |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| q_resolution_h2                 |  Float         | Sample aperture height              | If use_q_resolution| None   |
|                                 |                | (rectangular)                       | is set and circ.   |        |
|                                 |                |                                     | app. is not set    |        |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| q_resolution_w1                 |  Float         | Source aperture width               | If use_q_resolution| None   |
|                                 |                | (rectangular)                       | is set and circ.   |        |
|                                 |                |                                     | app. is not set    |        |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| q_resolution_w2                 |  Float         | Sample aperture width               | If use_q_resolution| None   |
|                                 |                | (rectangular)                       | is set and circ.   |        |
|                                 |                |                                     | app. is not set    |        |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+


Note that the momentum transfer resolution calculation is only applicable for 1D reductions.

**Note that these settings should be only populated via the GUI or the Python Interface of ISIS SANS.**


.. categories::

.. sourcelink::
