.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm scales a SANS workspace according to the settings in the state object. The scaling includes division by the volume of the sample and 
multiplication by an absolute scale. Currently the mask mechanism is implemented for **SANS2D**, **LOQ** and **LARMOR**.


Relevant SANSState entries for SANSScale
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The required information for the scale operation is retrieved from a state object.


The elements of the scale state are:

+---------------------+------------------+--------------------------------------------------------+------------+---------------+
| Entry               | Type             | Description                                            | Mandatory  | Default value |
+=====================+==================+========================================================+============+===============+
| shape               | SampleShape enum | The shape of the sample                                | No         | None          |
+---------------------+------------------+--------------------------------------------------------+------------+---------------+
| thickness           | Float            | The sample thickness in m                              | No         | None          |
+---------------------+------------------+--------------------------------------------------------+------------+---------------+
| width               |  Float           | The sample width in m                                  | None       |               |
+---------------------+------------------+--------------------------------------------------------+------------+---------------+
| height              | Float            | The sample height in m                                 | None       |               |
+---------------------+------------------+--------------------------------------------------------+------------+---------------+
| scale               | Float            | The absolute scale                                     | No         | None          |
+---------------------+------------------+--------------------------------------------------------+------------+---------------+
| shape_from_file     | SampleShape enum | The shape of the sample as stored on the data file     | auto setup | Cylinder      |
+---------------------+------------------+--------------------------------------------------------+------------+---------------+
| thickness_from_file | Float            | The thickness of the sample as stored on the data file | auto setup | 1.            |
+---------------------+------------------+--------------------------------------------------------+------------+---------------+
| width_from_file     | Float            | The width of the sample as stored on the data file     | auto setup | 1.            |
+---------------------+------------------+--------------------------------------------------------+------------+---------------+
| height_from_file    | Float            | The height of the sample as stored on the data file    | auto setup | 1.            |
+---------------------+------------------+--------------------------------------------------------+------------+---------------+


**Note that these settings should be only populated via the GUI or the Python Interface of ISIS SANS.**



.. categories::

.. sourcelink::
