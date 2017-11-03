.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm provides a monitor normalization workspace for subsequent wavelength correction in :ref:`algm-Q1D` or  :ref:`algm-Qxy`.
The settings of the algorithm are provided by the state object. The user can provide a *ScaleFactor* which is normally
obtained during event slicing.

Currently the mask mechanism is implemented for **SANS2D**, **LOQ** and **LARMOR**.


Relevant SANSState entries for SANSNormalizeToMonitor
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The required information for the monitor normalization is retrieved from a state object.

The elements are:

+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| Entry                           | Type           | Description                         | Mandatory          | Default|
+=================================+================+=====================================+====================+========+
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| prompt_peak_correction_min      | Float          | Min time of the prompt peak         | No                 | None   |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| prompt_peak_correction_max      | Float          | Max time of the prompt peak         | No                 | None   |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| prompt_peak_correction_enabled  | Bool           | If using prompt peak correction     | No                 | False  |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| rebin_type                      | RebinType enum | The type of rebinning to be used    | No                 | Rebin  |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| wavelength_low                  | Float          | Lower wavelength bound              | No                 | None   |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| wavelength_high                 | Float          | Upper wavelength bound              | No                 | None   |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| wavelength_step                 | Float          | Wavelength step                     | No                 | None   |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| wavelength_step_type            | RangeStepType  | Wavelength step type                | No                 | None   |
|                                 | enum           |                                     |                    |        |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| incident_monitor                | Integer        | The incident monitor                | Yes                | None   |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| background_TOF_general_start    | Float          | General background corr. start time | No                 | None   |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| background_TOF_general_stop     | Float          | General background corr. stop time  | No                 | None   |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| background_TOF_monitor_start    | Dict           | Monitor num vs background corr.     | No                 | None   |
|                                 |                | start time                          |                    |        |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| background_TOF_monitor_stop     | Dict           | Monitor num vs background corr.     | No                 | None   |
|                                 |                | stop time                           |                    |        |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+


Note that the prompt peak settings are automatically set up for **LOQ**

**Note that these settings should be only populated via the GUI or the Python Interface of ISIS SANS.**



.. categories::

.. sourcelink::
