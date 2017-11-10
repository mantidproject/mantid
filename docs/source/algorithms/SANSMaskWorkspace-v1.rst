.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm masks a SANS workspace according to the settings in the state object. The user can specify which detector
to mask. Currently the mask mechanism
is implemented for **SANS2D**, **LOQ** and **LARMOR**.

There are several types of masking which are currently supported:

- Time/Bin masking.
- Radius masking.
- Mask files.
- Angle masking.
- Spectrum masking which includes individual spectra, spectra ranges, spectra blocks and spectra cross blocks. These masks are partially specified on a detector level (see below).
- Beam stop masking.


Relevant SANSState entries for SANSMaskWorkspace
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The required information for the mask operation is retrieved from a state object. The state for masking is a composite of 
general settings and detector specific settings (see below).


The elements of the general mask state are:

+-----------------------+-----------------+---------------------------------------------+------------+---------------+
| Entry                 | Type            | Description                                 | Mandatory  | Default value |
+=======================+=================+=============================================+============+===============+
| radius_min            | Float           | Minimum radius in m for the radius mask     | No         | None          |
+-----------------------+-----------------+---------------------------------------------+------------+---------------+
| radius_min            | Float           | Maximum radius in m for the radius mask     | No         | None          |
+-----------------------+-----------------+---------------------------------------------+------------+---------------+
| bin_mask_general_start| List of Float   | A list of start values for time masking     | No         | None          |
+-----------------------+-----------------+---------------------------------------------+------------+---------------+
| bin_mask_general_stop | List of Float   | A list of stop values for time masking      | No         | None          |
+-----------------------+-----------------+---------------------------------------------+------------+---------------+
| mask_files            | List of String  | A list of mask file names                   | No         | None          |
+-----------------------+-----------------+---------------------------------------------+------------+---------------+
| phi_min               | Float           | Minimum angle for angle masking             | No         | None          |
+-----------------------+-----------------+---------------------------------------------+------------+---------------+
| phi_max               | Float           | Maximum angle for angle masking             | No         | None          |
+-----------------------+-----------------+---------------------------------------------+------------+---------------+
| use_mask_phi_mirror   | Bool            | If a mirrored angle mask is to be used      | No         | None          |
+-----------------------+-----------------+---------------------------------------------+------------+---------------+
| beam_stop_arm_width   | Float           | Size fo the beam stop arm in m              | No         | None          |
+-----------------------+-----------------+---------------------------------------------+------------+---------------+
| beam_stop_arm_angle   | Float           | Angle of the beam stop arm                  | No         | None          |
+-----------------------+-----------------+---------------------------------------------+------------+---------------+
| beam_stop_arm_pos1    | Float           | First coordinate of the beam stop position  | No         | None          |
+-----------------------+-----------------+---------------------------------------------+------------+---------------+
| beam_stop_arm_pos2    | Float           | Second coordinate of the beam stop position | No         | None          |
+-----------------------+-----------------+---------------------------------------------+------------+---------------+
| clear                 | Bool            | If the spectra mask is to be cleared        | No         | None          |
+-----------------------+-----------------+---------------------------------------------+------------+---------------+
| clear_time            | Float           | If the time mask is to be cleared           | No         | None          |
+-----------------------+-----------------+---------------------------------------------+------------+---------------+
| single_spectra        | List of Integer | List of spectra which are to be masked      | No         | None          |
+-----------------------+-----------------+---------------------------------------------+------------+---------------+
| spectrum_range_start  | List of Integer | List of specra where a range mask starts    | No         | None          |
+-----------------------+-----------------+---------------------------------------------+------------+---------------+
| spectrum_range_stop   | List of Integer | List of specra where a range mask stops     | No         | None          |
+-----------------------+-----------------+---------------------------------------------+------------+---------------+
| detectors             | Dict            | A map to detector-specific settings         | No         | None          |
+-----------------------+-----------------+---------------------------------------------+------------+---------------+
| idf_path              | String          | The path to the IDF                         | auto setup | auto setup    |
+-----------------------+-----------------+---------------------------------------------+------------+---------------+



The detectors dictionary above maps to a mask state object for the individual detectors :

+-----------------------------+-----------------+--------------------------------------+------------+---------------+
| Entry                       | Type            | Description                          | Mandatory  | Default value |
+=============================+=================+======================================+============+===============+
| single_vertical_strip_mask  | List of Integer | A list of vertical strip masks       | No         | 0.0           |
+-----------------------------+-----------------+--------------------------------------+------------+---------------+
| range_vertical_strip_start  | List of Integer | A list of start spectra for vertical | No         | 0.0           |
|                             |                 | strip mask ranges                    |            |               |
+-----------------------------+-----------------+--------------------------------------+------------+---------------+
| range_vertical_strip_stop   | List of Integer | A list of stop spectra for vertical  | No         | 0.0           |
|                             |                 | strip mask ranges                    |            |               |
+-----------------------------+-----------------+--------------------------------------+------------+---------------+
| single_horizontal_strip_mask| List of Integer | A list of horizontal strip masks     | No         | 0.0           |
+-----------------------------+-----------------+--------------------------------------+------------+---------------+
| range_horizontal_strip_start| List of Integer | A list of start spectra for          | No         | 0.0           |
|                             |                 | horizontal strip mask ranges         |            |               |
+-----------------------------+-----------------+--------------------------------------+------------+---------------+
| range_horizontal_strip_stop | List of Integer | A list of stop spectra for           | No         | 0.0           |
|                             |                 | horizontal strip mask ranges         |            |               |
+-----------------------------+-----------------+--------------------------------------+------------+---------------+
| block_horizontal_start      | List of Integer | A list of start spectra for the      | No         | 0.0           |
|                             |                 | horizontal part of block masks       |            |               |
+-----------------------------+-----------------+--------------------------------------+------------+---------------+
| block_horizontal_stop       | List of Integer | A list of stop spectra for the       | No         | 0.0           |
|                             |                 | horizontal part of block masks       |            |               |
+-----------------------------+-----------------+--------------------------------------+------------+---------------+
| block_vertical_start        | List of Integer | A list of start spectra for the      | No         | 0.0           |
|                             |                 | vertical part of block masks         |            |               |
+-----------------------------+-----------------+--------------------------------------+------------+---------------+
| block_vertical_stop         | List of Integer | A list of stop spectra for the       | No         | 0.0           |
|                             |                 | vertical part of block masks         |            |               |
+-----------------------------+-----------------+--------------------------------------+------------+---------------+
| block_cross_horizontal      | List of Integer | A list of spectra for the horizontal | No         | 0.0           |
|                             |                 | part of cross block masks            |            |               |
+-----------------------------+-----------------+--------------------------------------+------------+---------------+
| block_cross_vertical        | List of Integer | A list of spectra for the vertical   | No         | 0.0           |
|                             |                 | part of cross block masks            |            |               |
+-----------------------------+-----------------+--------------------------------------+------------+---------------+


**Note that these settings should be only populated via the GUI or the Python Interface of ISIS SANS.**


Mask options for the detector: *LAB*, *HAB*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The *LAB* (low angle bank) setting selects the first detector of the instrument.

The *HAB* (high angle bank) setting selects the first detector of the instrument.


.. categories::

.. sourcelink::
