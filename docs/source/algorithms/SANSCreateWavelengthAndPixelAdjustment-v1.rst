.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm provides a wavelength-adjustment workspace and a pixel-adjustment workspace which are used in :ref:`algm-Q1D` or  :ref:`algm-Qxy`.
The wavelength-adjustment workspace is created by combining the transmission workspace obtained
from  :ref:`algm-SANSCalculateTransmission`, the monitor normalization workspace obtained from :ref:`algm-SANSNormalizeToMonitor` and
efficiency correction files which are provided by the state object. The pixel-adjustment settings are also obtained
from the state object.


Currently the mask mechanism is implemented for **SANS2D**, **LOQ** and **LARMOR**.


Relevant SANSState entries for SANSCreateWavelengthAndPixelAdjustment
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The required information for the creation of the wavelength-adjustment workspace and the pixel-adjustment workspace is retrieved from a state object.

The elements are:

+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| Entry                           | Type           | Description                         | Mandatory          | Default|
+=================================+================+=====================================+====================+========+
| wavelength_low                  | Float          | Lower wavelength bound              | No                 | None   |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| wavelength_high                 | Float          | Upper wavelength bound              | No                 | None   |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| wavelength_step                 | Float          | Wavelength step                     | No                 | None   |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| wavelength_step_type            | RangeStepType  | The wavelength step type            | No                 | None   |
|                                 | enum           |                                     |                    |        |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| adjustment_files                | Dict           | Detector vs StateAdjustmentFiles    | No                 | None   |
|                                 |                | object                              |                    |        |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+
| idf_path                        | String         | The path to the IDF                 | auto setup         | auto   |
+---------------------------------+----------------+-------------------------------------+--------------------+--------+


**Note that these settings should be only populated via the GUI or the Python Interface of ISIS SANS.**


.. categories::

.. sourcelink::
