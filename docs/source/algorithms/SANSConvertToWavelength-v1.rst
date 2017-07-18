.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm converts a SANS workspace to wavelength and rebins it. The settings for the wavelength conversion and the rebining are stored in the state object. Currently the mask mechanism
is implemented for **SANS2D**, **LOQ** and **LARMOR**.



Relevant SANSState entries for SANSConvertToWavelength
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The required information for the wavelength conversion is retrieved from a state object.


The elements of the wavelength conversion state are:

+----------------------+--------------------+----------------------------------------------+------------+---------------+
| Entry                | Type               | Description                                  | Mandatory  | Default value |
+======================+====================+==============================================+============+===============+
| rebin_type           | RebinType enum     | The type of rebin, ie Rebin or Interpolating | No         | None          |
+----------------------+--------------------+----------------------------------------------+------------+---------------+
| wavelength_low       | Float              | Lower wavelength bound                       | No         | None          |
+----------------------+--------------------+----------------------------------------------+------------+---------------+
| wavelength_high      | Float              | Upper wavelength bound                       | No         | None          |
+----------------------+--------------------+----------------------------------------------+------------+---------------+
| wavelength_step      | Float              | Wavelength step                              | No         | None          |
+----------------------+--------------------+----------------------------------------------+------------+---------------+
| wavelength_step_type | RangeStepType enum | Wavelength step type                         | No         | None          |
+----------------------+--------------------+----------------------------------------------+------------+---------------+

**Note that these settings should be only populated via the GUI or the Python Interface of ISIS SANS.**


.. categories::

.. sourcelink::
