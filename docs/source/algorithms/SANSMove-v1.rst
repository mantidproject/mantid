.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm moves a SANS workspace according to the settings in the state object. Additionally the user can specify
the beam centre. Note that if the beam centre is also specified in the state object, then the manual selection takes
precedence. The way we perform a move is highly-instrument and in fact data-dependent. Currently the move mechanism
is implemented for **SANS2D**, **LOQ** and **LARMOR**.


Relevant SANSState entries for SANSMove
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The required information for the move operation is retrieved from a state object. It contains information which is
specific to each instrument and to the specific IDF.


Common elements of the move state object are:

+---------------------------+---------------------------+-------------------------------------------------+------------+------------------------+
| Entry                     | Type                      | Description                                     | Mandatory  | Default value          |
+===========================+===========================+=================================================+============+========================+
| sample_offset             | Float                     | The offset of the sample in m                   | No         | 0.0                    |
+---------------------------+---------------------------+-------------------------------------------------+------------+------------------------+
| sample_offset_direction   | CanonicalCoordinates enum | The direction of the sample offset              | No         | CanonicalCoordinates.Z |
+---------------------------+---------------------------+-------------------------------------------------+------------+------------------------+
| detectors                 | Dict                      | Dictionary of detectors.                        | auto setup | auto setup             |
+---------------------------+---------------------------+-------------------------------------------------+------------+------------------------+
| monitor_names             | Dict                      | A dictionary with monitor index vs monitor name | auto setup | auto setup             |
+---------------------------+---------------------------+-------------------------------------------------+------------+------------------------+

The detectors dictionary above maps to a state object for the individual detectors:

+--------------------------+--------+------------------------------------------------------------------------+------------+---------------+
| Entry                    | Type   | Description                                                            | Mandatory  | Default value |
+==========================+========+========================================================================+============+===============+
| x_translation_correction | Float  | X translation for the detector in m                                    | No         | 0.0           |
+--------------------------+--------+------------------------------------------------------------------------+------------+---------------+
| y_translation_correction | Float  | Y translation for the detector in m                                    | No         | 0.0           |
+--------------------------+--------+------------------------------------------------------------------------+------------+---------------+
| z_translation_correction | Float  | X translation for the detector in m                                    | No         | 0.0           |
+--------------------------+--------+------------------------------------------------------------------------+------------+---------------+
| rotation_correction      | Float  | Rotation correction for the detector in degrees                        | No         | 0.0           |
+--------------------------+--------+------------------------------------------------------------------------+------------+---------------+
| side_correction          | Float  | Side correction for the detector in m                                  | No         | 0.0           |
+--------------------------+--------+------------------------------------------------------------------------+------------+---------------+
| radius_correction        | Float  | Radius correction for the detector in m                                | No         | 0.0           |
+--------------------------+--------+------------------------------------------------------------------------+------------+---------------+
| x_tilt_correction        | Float  | X tilt correction for the detector in degrees                          | No         | 0.0           |
+--------------------------+--------+------------------------------------------------------------------------+------------+---------------+
| y_tilt_correction        | Float  | Y tilt correction for the detector in degrees                          | No         | 0.0           |
+--------------------------+--------+------------------------------------------------------------------------+------------+---------------+
| z_tilt_correction        | Float  | Z tilt correction for the detector in degrees                          | No         | 0.0           |
+--------------------------+--------+------------------------------------------------------------------------+------------+---------------+
| sample_centre_pos1       | Float  | Position 1 of the beam centre in m or degree, depending on the setting | No         | 0.0           |
+--------------------------+--------+------------------------------------------------------------------------+------------+---------------+
| sample_centre_pos2       | Float  | Position 2 of the beam centre in m                                     | No         | 0.0           |
+--------------------------+--------+------------------------------------------------------------------------+------------+---------------+
| detector_name            | String | Detector name                                                          | auto setup | auto setup    |
+--------------------------+--------+------------------------------------------------------------------------+------------+---------------+
| detector_name_short      | String | Short detector name                                                    | auto setup | auto setup    |
+--------------------------+--------+------------------------------------------------------------------------+------------+---------------+


The individual instruments have additional settings.


For LOQ:

+-----------------+-------+-------------------------------------------------+--------------+---------------+
| Entry           | Type  | Description                                     | Mandatory    | Default value |
+=================+=======+=================================================+==============+===============+
+-----------------+-------+-------------------------------------------------+--------------+---------------+
| center_position | Float | The centre position                             | No           | 317.5 / 1000. |
+-----------------+-------+-------------------------------------------------+--------------+---------------+


For SANS2D:

+---------------------------+-------+-------------------------------------------------+------------+---------------+
| Entry                     | Type  | Description                                     | Mandatory  | Default value |
+===========================+=======+=================================================+============+===============+
+---------------------------+-------+-------------------------------------------------+------------+---------------+
| hab_detector_radius       | Float | Radius for the front detector in m              | auto setup | 306.0 / 1000. |
+---------------------------+-------+-------------------------------------------------+------------+---------------+
| hab_detector_default_sd_m | Float | Default sd for front detector in m              | auto setup | 4.            |
+---------------------------+-------+-------------------------------------------------+------------+---------------+
| hab_detector_default_x_m  | Float | Default x for the front detector in m           | auto setup | 1.1           |
+---------------------------+-------+-------------------------------------------------+------------+---------------+
| lab_detector_default_sd_m | Float | Default sd for the rear detector in m           | auto setup | 4.            |
+---------------------------+-------+-------------------------------------------------+------------+---------------+
| hab_detector_x            | Float | X for the front detector in m                   | auto setup | 0.            |
+---------------------------+-------+-------------------------------------------------+------------+---------------+
| hab_detector_z            | Float | Z for the front detector in m                   | auto setup | 0.            |
+---------------------------+-------+-------------------------------------------------+------------+---------------+
| hab_detector_rotation     | Float | Rotation for the front detector                 | auto setup | 0.            |
+---------------------------+-------+-------------------------------------------------+------------+---------------+
| lab_detector_x            | Float | X for the rear detector in m                    | auto setup | 0.            |
+---------------------------+-------+-------------------------------------------------+------------+---------------+
| lab_detector_z            | Float | Z for the rear detector in m                    | auto setup | 0.            |
+---------------------------+-------+-------------------------------------------------+------------+---------------+
| monitor_4_offset          | Float | Offset for monitor 4                            | No         | 0.            |
+---------------------------+-------+-------------------------------------------------+------------+---------------+


For LARMOR

+----------------+-------+-------------------------------------------------+------------+---------------+
| Entry          | Type  | Description                                     | Mandatory  | Default value |
+================+=======+=================================================+============+===============+
+----------------+-------+-------------------------------------------------+------------+---------------+
| bench_rotation | Float | The angle for the bench rotation                | No         | 0.            |
+----------------+-------+-------------------------------------------------+------------+---------------+


**Note that these settings should be only populated via the GUI or the Python Interface of ISIS SANS.**


Move options: *InitialMove*, *ElementaryDisplacement*, *SetToZero*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The *InitialMove* setting is relevant when loading data before a reduction. It will apply all corrections which are specified in the state object.

The *ElementaryDisplacement* will perform a relative translation/rotation according to the specified beam centre value.

The *SetToZero* places the component into the default position.


Beam Centre
~~~~~~~~~~~~~~~~

The beam centre for a reduction is normally specified in the state object, but it can also be specified in manually here.
If the beam centre is specified explicitly, then it is being used instead of the setting in the state object.


.. categories::

.. sourcelink::
