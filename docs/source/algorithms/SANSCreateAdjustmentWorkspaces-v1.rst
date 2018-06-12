.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm provides a wavelength-adjustment workspace, a pixel-adjustment workspace and a wavelength-and-pixel-adjustment
workspace which are used in :ref:`algm-Q1D` or  :ref:`algm-Qxy`. The wavelength-adjustment and the pixel-adjustment workspaces
are obtained from :ref:`algm-SANSCreateWavelengthAndPixelAdjustment` and the wavelength-and-pixel-adjustment workspace is
obtained from :ref:`algm-SANSWideAngleCorrection`. The relevant settings are provided via the state object. Note
that none of these workspaces is required for a minimal reduction.

Currently the mask mechanism is implemented for **SANS2D**, **LOQ** and **LARMOR**.


Relevant SANSState entries for SANSCreateAdjustmentWorkspaces
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The required information for the creation of the wavelength-adjustment workspace, the pixel-adjustment workspace
and the wavelength-and-pixel-adjustment workspace is  retrieved from a state object. Note that this state
is a composite of states which are consumed in the child algorithms.

The elements are:

+---------------------------------+-----------+---------------------------------------------------+-----------+--------+
| Entry                           | Type      | Description                                       | Mandatory | Default|
+=================================+===========+===================================================+===========+========+
| calculate_transmission          | sub state | State used in                                     | Yes       | None   |
|                                 |           | :ref:`algm-SANSCalculateTransmission`             |           |        |
+---------------------------------+-----------+---------------------------------------------------+-----------+--------+
| normalize_to_monitor            | sub state | State used in                                     | Yes       | None   |
|                                 |           | :ref:`algm-SANSNormalizeToMonitor`                |           |        |
+---------------------------------+-----------+---------------------------------------------------+-----------+--------+
| wavelength_and_pixel_adjustment | sub state | State used in                                     | Yes       | None   |
|                                 |           | :ref:`algm-SANSCreateWavelengthAndPixelAdjustment`|           |        |
+---------------------------------+-----------+---------------------------------------------------+-----------+--------+
| wide_angle_correction           | Bool      | The wavelength step type                          | No        | False  |
+---------------------------------+-----------+---------------------------------------------------+-----------+--------+

**Note that these settings should be only populated via the GUI or the Python Interface of ISIS SANS.**


.. categories::

.. sourcelink::
