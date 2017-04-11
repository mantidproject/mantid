.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm provides a transmission workspace for subsequent wavelength correction in :ref:`algm-Q1D` or  :ref:`algm-Qxy`.
The settings of the algorithm are provided by the state object. The user provides a *TransmissionWorkspace*,
*DirectWorkspace* and the data type which is to be used, ie *Sample* or *Can*. The *OutputWorkspace* is a fitted
workspace, but the unfitted data set is available via *UnfittedData*.

Currently the mask mechanism is implemented for **SANS2D**, **LOQ** and **LARMOR**.


Relevant SANSState entries for SANSCalculateTransmission
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The required information for the transmission calculation is retrieved from a state object.

The elements are:

+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| Entry                           | Type           | Description                         | Mandatory          | Default|
+=================================+================+=====================================+====================+========+
| transmission_radius_on_detector | Float          | A radius on the detector            | No                 | None   |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| transmission_roi_files          | List of String | A list of ROI file names            | No                 | None   |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| transmission_mask_files         | List of String | A list of mask file names           | No                 | None   |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| default_transmission_monitor    | Integer        | The default transmission monitor    | auto setup         | auto   |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| transmission_monitor            | Integer        | The transmission monitor            | No                 | None   |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| default_incident_monitor        | Integer        | The default incident monitor        | auto setup         | auto   |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| incident_monitor                | Integer        | The incident monitor                | No                 | None   |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| prompt_peak_correction_min      | Float          | Min time of the prompt peak         | No                 | None   |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| prompt_peak_correction_max      | Float          | Max time of the prompt peak         | No                 | None   |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| prompt_peak_correction_enabled  | Bool           | If using prompt peak correction     | No                 | False  |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| use_full_wavelength_range       | Bool           | If using full wavelength range      | No                 | False  |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| wavelength_full_range_low       | Float          | Min of instrument's full wav. range | auto setup         | auto   |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| wavelength_full_range_high      | Float          | Max of instrument's full wav. range | auto setup         | auto   |
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
| background_TOF_roi_start        | Float          | ROI background corr. start time     | No                 | None   |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| background_TOF_roi_stop         | Float          | ROI background corr. stop time      | No                 | None   |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| fit                             | Dict           | DataType enum (Sample and Can) vs   | No                 | None   |
|                                 |                | fit state                           |                    |        |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+

There is a fit state for the sample and the can which contains the following settings:

+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| Entry                           | Type           | Description                         | Mandatory          | Default|
+=================================+================+=====================================+====================+========+
| fit_type                        | FitType enum   | The type of fit, ie log, lin, poly. | No                 | log    |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| polynomial_order                | Integer        | The polynomial order when using     | No                 | 0      |
|                                 |                | polynomial fitting                  |                    |        |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| wavelength_low                  | Float          | Lower wavelength bound              | No                 | None   |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| wavelength_high                 | Float          | Upper wavelength bound              | No                 | None   |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+


Note that the prompt peak settings are automatically set up for **LOQ**

**Note that these settings should be only populated via the GUI or the Python Interface of ISIS SANS.**


.. categories::

.. sourcelink::
