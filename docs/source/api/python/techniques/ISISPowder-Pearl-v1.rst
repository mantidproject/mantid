.. _isis-powder-diffraction-pearl-ref:

=====================================================
ISIS Powder Diffraction Scripts - PEARL Reference
=====================================================

.. contents:: Table of Contents
    :local:

.. _creating_pearl_object-isis-powder-diffraction-ref:

Creating PEARL Object
--------------------
This method assumes you are familiar with the concept of objects in Python.
If not more details can be read here: :ref:`intro_to_objects-isis-powder-diffraction-ref`

To create a PEARL object the following parameters are required:

- :ref:`calibration_directory_pearl_isis-powder-diffraction-ref` 
- :ref:`output_directory_pearl_isis-powder-diffraction-ref` 
- :ref:`user_name_pearl_isis-powder-diffraction-ref` 

Optionally a configuration file may be specified if one exists 
using the following parameter:

- :ref:`config_file_pearl_isis-powder-diffraction-ref`

See :ref:`configuration_files_isis-powder-diffraction-ref`
on YAML configuration files for more details

Example
^^^^^^^

.. code-block:: Python

  from isis_powder import Pearl
  
  calibration_dir = r"C:\path\to\calibration_dir"
  output_dir = r"C:\path\to\output_dir"
  
  pearl_example = Pearl(calibration_directory=calibration_dir,
                        output_directory=output_dir,
                        user_name="Mantid")

  # Optionally we could provide a configuration file like so
  # Notice how the file name ends with .yaml
  config_file_path = r"C:\path\to\config_file.yaml
  pearl_example = Pearl(config_file=config_file_path,
                        user_name="Mantid", ...)

Methods
--------
The following methods can be executed on a PEARL object:

- :ref:`create_vanadium_pearl_isis-powder-diffraction-ref`
- :ref:`focus_pearl_isis-powder-diffraction-ref`

For information on creating a PEARL object see:
:ref:`creating_pearl_object-isis-powder-diffraction-ref`

.. _create_vanadium_pearl_isis-powder-diffraction-ref:

create_vanadium
^^^^^^^^^^^^^^^
The *create_vanadium* method allows a user to process a vanadium run.
Whilst processing the vanadium run the scripts can apply any corrections
the user enables and will spline the resulting workspace(s) for later focusing.

On PEARL the following parameters are required when executing *create_vanadium*:

- :ref:`calibration_mapping_file_pearl_isis-powder-diffraction-ref`
- :ref:`do_absorb_corrections_pearl_isis-powder-diffraction-ref`
- :ref:`long_mode_pearl_isis-powder-diffraction-ref`
- :ref:`run_in_cycle_pearl_isis-powder-diffraction-ref`
- :ref:`tt_mode_pearl_isis-powder-diffraction-ref`

Example
=======

.. code-block:: Python

  # Notice how the filename ends with .yaml
  cal_mapping_file = r"C:\path\to\cal_mapping.yaml"

  pearl_example.create_vanadium(calibration_mapping_file=cal_mapping_file,
                                do_absorb_corrections=True, long_mode=False,
                                run_in_cycle="100", tt_mode="tt88")

.. _focus_pearl_isis-powder-diffraction-ref:

focus
^^^^^
The *focus* method processes the user specified run(s). It aligns,
focuses and optionally applies corrections if the user has requested them.

On PEARL the following parameters are required when executing *focus*:

- :ref:`calibration_mapping_file_pearl_isis-powder-diffraction-ref`
- :ref:`focus_mode_pearl_isis-powder-diffraction-ref`
- :ref:`long_mode_pearl_isis-powder-diffraction-ref`
- :ref:`perform_attenuation_pearl_isis-powder-diffraction-ref`
- :ref:`run_number_pearl_isis-powder-diffraction-ref`
- :ref:`tt_mode_pearl_isis-powder-diffraction-ref`
- :ref:`vanadium_normalisation_pearl_isis-powder-diffraction-ref`


The following parameter is required if 
:ref:`perform_attenuation_pearl_isis-powder-diffraction-ref` is set to **True**

- :ref:`attenuation_file_path_pearl_isis-powder-diffraction-ref`

The following parameter may also be optionally set:

- :ref:`file_ext_pearl_isis-powder-diffraction-ref`

Example
=======

.. code-block:: Python

  # Notice how the filename ends with .yaml
  cal_mapping_file = r"C:\path\to\cal_mapping.yaml"

  attenuation_path = r"C:\path\to\attenuation_file"

  pearl_example.focus(calibration_mapping_file=cal_mapping_file,
                      focus_mode="all", long_mode=True,
                      perform_attenuation=True,
                      attenuation_file_path=attenuation_path,
                      run_number="100-110", tt_mode="tt88",
                      vanadium_normalisation=True)

.. _calibration_mapping_pearl_isis-powder-diffraction-ref:

Calibration Mapping File
------------------------
The calibration mapping file holds the mapping between
run numbers, current label, offset filename and the empty 
and vanadium numbers.

For more details on the calibration mapping file see:
:ref:`cycle_mapping_files_isis-powder-diffraction-ref`

The layout on PEARL should look as follows
substituting the example values included for appropriate values:

.. code-block:: yaml

  1-100:
    label: "1_1"
    offset_file_name: "offset_file.cal"
    empty_run_numbers: "10"
    vanadium_run_numbers: "20"

Example
^^^^^^^^
.. code-block:: yaml

  1-100:
    label: "1_1"
    offset_file_name: "offset_file.cal"
    empty_run_numbers: "10"
    vanadium_run_numbers: "20"

  101-:
    label: "1_2"
    offset_file_name: "offset_file.cal"
    empty_run_numbers: "110"
    vanadium_run_numbers: "120"

Parameters
-----------
The following parameters for PEARL are intended for regular use
when using the ISIS Powder scripts.

.. _attenuation_file_path_pearl_isis-powder-diffraction-ref:

attenuation_file_path
^^^^^^^^^^^^^^^^^^^^^
Required if :ref:`perform_attenuation_pearl_isis-powder-diffraction-ref`
is set to **True**

The full path to the attenuation file to use within the
:ref:`focus_pearl_isis-powder-diffraction-ref` method.

The workspace will be attenuated with the specified file
if the :ref:`focus_mode_pearl_isis-powder-diffraction-ref`
is set to **all** or **trans**. For more details see
:ref:`PearlMCAbsorption<algm-PearlMCAbsorption>`

*Note: The path to the file must include the file extension*

Example Input:

.. code-block:: Python

  attenuation_path = r"C:\path\to\attenuation_file.out
  pearl_example(attenuation_file_path=attenuation_path, ...)

.. _calibration_directory_pearl_isis-powder-diffraction-ref:

calibration_directory
^^^^^^^^^^^^^^^^^^^^^
This parameter should be the full path to the calibration folder.
Within the folder the following should be present:

- Grouping .cal files:
  - :ref:`tt35_grouping_filename_pearl_isis-powder-diffraction-ref`
  - :ref:`tt70_grouping_filename_pearl_isis-powder-diffraction-ref`
  - :ref:`tt80_grouping_filename_pearl_isis-powder-diffraction-ref`
- Vanadium Absorption File 
  (see: :ref:`vanadium_absorb_filename_pearl_isis-powder-diffraction-ref`)
- Folder(s) with the label name specified in mapping file (e.g. "1_1")
  - Inside each folder should be the offset file with name specified in mapping file

The script will also save out vanadium splines into the relevant
label folder which are subsequently loaded and used within the
:ref:`focus_pearl_isis-powder-diffraction-ref` method. 

Example Input:

.. code-block:: Python

  calibration_dir = r"C:\path\to\calibration_dir"
  pearl_example = Pearl(calibration_directory=calibration_dir, ...)

.. _calibration_mapping_file_pearl_isis-powder-diffraction-ref:

calibration_mapping_file
^^^^^^^^^^^^^^^^^^^^^^^^
This parameter gives the full path to the YAML file containing the 
calibration mapping. For more details on this file see:
:ref:`calibration_mapping_pearl-isis-powder-ref`

*Note: This should be the full path to the file including extension*

Example Input:

.. code-block:: Python

  # Notice the filename always ends in .yaml
  cal_mapping_file = r"C:\path\to\file\calibration_mapping.yaml"
  pearl_example = Pearl(calibration_mapping_file=cal_mapping_file, ...)

.. _config_file_pearl_isis-powder-diffraction-ref:

config_file
^^^^^^^^^^^
The full path to the YAML configuration file. This file is 
described in detail here: :ref:`configuration_files_isis-powder-diffraction-ref`
It is recommended to set this parameter at object creation instead
of on a method as it will warn if any parameters are overridden 
in the scripting window.

*Note: This should be the full path to the file including extension*

Example Input:

.. code-block:: Python

  # Notice the filename always ends in .yaml
  configuration_file = r"C:\path\to\file\configuration.yaml"
  pearl_example = Pearl(config_file=configuration_file, ...)

.. _do_absorb_corrections_pearl_isis-powder-diffraction-ref:

do_absorb_corrections
^^^^^^^^^^^^^^^^^^^^^
Indicates whether to perform vanadium absorption corrections 
when calling :ref:`create_vanadium_pearl_isis-powder-diffraction-ref`.
If set to True the vanadium absorption file
(described here: :ref:`vanadium_absorb_filename_pearl_isis-powder-diffraction-ref`)
will be loaded and the vanadium sample will be divided by the pre-calculated
absorption corrections.

Accepted values are: **True** or **False**

Example Input:

.. code-block:: Python

  pearl_example.create_vanadium(do_absorb_corrections=True, ...)

.. _file_ext_pearl_isis-powder-diffraction-ref:

file_ext
^^^^^^^^
*Optional*

Specifies a file extension to use when using the 
:ref:`focus_pearl_isis-powder-diffraction-ref` method.

This should be used to process partial runs. When 
processing full runs (i.e. completed runs) it should not
be specified as Mantid will automatically determine the
best extension to use.

*Note: A leading dot (.) is not required but 
is preferred for readability*

Example Input:

.. code-block:: Python

  pearl_example.focus(file_ext=".s01", ...)

.. _focus_mode_pearl_isis-powder-diffraction-ref:

focus_mode
^^^^^^^^^^
Determines how the banks are grouped when using the
:ref:`focus_pearl_isis-powder-diffraction-ref` method.
Each mode is further described below.

Accepted values are: **All**, **Groups**, **Mods** and **Trans**

All
====
In all mode banks 1-9 (inclusive) are summed into a single spectra 
then scaled down to 1/9 of their original values. 

The workspace is also attenuated if 
:ref:`perform_attenuation_pearl_isis-powder-diffraction-ref`
is set to **True**. 

Workspaces containing banks 10-14 are left as 
separate workspaces with appropriate names.

Groups
======
In groups mode banks 1+2+3, 4+5+6, 7+8+9 are summed into three (3) 
separate workspaces. Each workspace is scaled down to a 1/3 of original scale. 

The workspaces containing banks 4-9 (inclusive) are then added 
into a separate workspace and scaled down to 1/2 original scale. 

Banks 10-14 are left as separate workspaces with appropriate names.

Trans
======
In trans mode banks 1-9 (inclusive) are summed into a single spectra 
then scaled down to 1/9 original scale. 

The workspace is also attenuated if 
:ref:`perform_attenuation_pearl_isis-powder-diffraction-ref`
is set to **True**. 

All banks are also output as individual workspaces with appropriate names
with no additional processing applied.

Mods
====
In mods mode every bank is left as individual workspaces with 
appropriate names. No additional processing is performed.

Example Input:

.. code-block:: Python

  pearl_example.focus(focus_mode="all", ...)

.. _long_mode_pearl_isis-powder-diffraction-ref:

long_mode
^^^^^^^^^
Determines the TOF window to process data in. This
affects both the :ref:`create_vanadium_pearl_isis-powder-diffraction-ref`
and :ref:`focus_pearl_isis-powder-diffraction-ref` methods.

As this affects the vanadium spline used the 
:ref:`create_vanadium_pearl_isis-powder-diffraction-ref` method
will need to be called once for each *long_mode* value (**True** and/or **False**)
if the user intends to use a different mode. This will create
a spline for the relevant mode which is automatically used when focusing.

When *long_mode* is **False** the TOF window processed is 
between 0-20,000 μs

When *long_mode* is **True** the TOF window processed is 
between 0-40,000 μs

This also affects the :ref:`advanced_parameters_pearl_isis-powder-diffraction-ref`
used. More detail can be found for each individual parameter 
listed under the advanced parameters section.

Accepted values are: **True** or **False**

Example Input:

.. code-block:: Python

  pearl_example.create_vanadium(long_mode=False, ...)
  # Or
  pearl_example.focus(long_mode=True, ...)


.. _output_directory_pearl_isis-powder-diffraction-ref:

output_directory
^^^^^^^^^^^^^^^^
Specifies the path to the output directory to save resulting files
into. The script will automatically create a folder
with the label determined from the 
:ref:`calibration_mapping_file_pearl_isis-powder-diffraction-ref`
and within that create another folder for the current
:ref:`user_name_pearl_isis-powder-diffraction-ref`. 

Within this folder processed data will be saved out in
several formats.

Example Input:

.. code-block:: Python

  output_dir = r"C:\path\to\output_dir"
  pearl_example = Pearl(output_directory=output_dir, ...)

.. _perform_attenuation_pearl_isis-powder-diffraction-ref:

perform_attenuation
^^^^^^^^^^^^^^^^^^^^
Indicates whether to perform attenuation corrections
whilst running :ref:`focus_pearl_isis-powder-diffraction-ref`.
For more details of the corrections performed see:
:ref:`PearlMCAbsorption<algm-PearlMCAbsorption>`

If this is set to **True** 
:ref:`attenuation_file_path_pearl_isis-powder-diffraction-ref`
must be set too. 

*Note: This correction will only be performed if
*focus_mode* is in **All* or **Trans**. See:
:ref:`focus_mode_pearl_isis-powder-diffraction-ref`
for more details.*

Accepted values are: **True** or **False**

Example Input:

.. code-block:: Python

  pearl_example.focus(perform_attenuation=True, ...)

.. _run_in_cycle_pearl_isis-powder-diffraction-ref:

run_in_cycle
^^^^^^^^^^^^
Indicates a run from the current cycle to use when calling
:ref:`create_vanadium_pearl_isis-powder-diffraction-ref`.
This does not have the be the first run of the cycle or
the run number corresponding to the vanadium. However it
must be in the correct cycle according to the 
:ref:`calibration_mapping_pearl-isis-powder-ref`.

Example Input:

.. code-block:: Python

  # In this example assume we mean a cycle with run numbers 100-200
  pearl_example.create_vanadium(run_in_cycle=100, ...)

.. _run_number_pearl_isis-powder-diffraction-ref:

run_number
^^^^^^^^^^
Specifies the run number(s) to process when calling the
:ref:`focus_pearl_isis-powder-diffraction-ref` method.

This parameter accepts a single value or a range 
of values with the following syntax:

**-** : Indicates a range of runs inclusive 
(e.g. *1-10* would process 1, 2, 3....8, 9, 10)

**,** : Indicates a gap between runs 
(e.g. *1, 3, 5, 7* would process run numbers 1, 3, 5, 7)

These can be combined like so:
*1-3, 5, 8-10* would process run numbers 1, 2, 3, 5, 8, 9, 10.

On Pearl any ranges of runs indicates the runs will be summed
before any additional processing takes place. For example
a run input of *1, 3, 5* will sum runs 1, 3 and 5 together
before proceeding to focus them.

Example Input:

.. code-block:: Python

  # Sum and process run numbers 1, 3, 5, 6, 7
  pearl_example.focus(run_number="1, 3, 5-7", ...)
  # Or just a single run
  pearl_example.focus(run_number=100, ...)

.. _tt_mode_pearl_isis-powder-diffraction-ref:

tt_mode
^^^^^^^^
Specifies the detectors to be considered from the 
grouping files. This is used in the 
:ref:`create_vanadium_pearl_isis-powder-diffraction-ref` and
:ref:`focus_pearl_isis-powder-diffraction-ref` methods. 

For more details of the grouping file which is selected between
see the following:

- :ref:`tt35_grouping_filename_pearl_isis-powder-diffraction-ref`
- :ref:`tt70_grouping_filename_pearl_isis-powder-diffraction-ref`
- :ref:`tt88_grouping_filename_pearl_isis-powder-diffraction-ref`

Accepted values are: **tt35**, **tt70** and **tt80**

When calling :ref:`create_vanadium_pearl_isis-powder-diffraction-ref`
**all** can be used to implicitly process all of the supported 
values indicated above.

Example Input:

.. code-block:: Python

  pearl_example.create_vanadium(tt_mode="all", ...)
  # Or
  pearl_example.focus(tt_mode="tt35", ...)

.. _user_name_pearl_isis-powder-diffraction-ref:

user_name
^^^^^^^^^
Specifies the name of the current user when creating a 
new PEARL object. This is only used when saving data to
sort data into respective user folders. 
See :ref:`output_directory_pearl_isis-powder-diffraction-ref`
for more details.

Example Input:

.. code-block:: Python

  pearl_example = Pearl(user_name="Mantid", ...)

.. _vanadium_normalisation_pearl_isis-powder-diffraction-ref:

vanadium_normalisation
^^^^^^^^^^^^^^^^^^^^^^
Indicates whether to divide the focused workspace within 
:ref:`focus_pearl_isis-powder-diffraction-ref` mode with a
previously generated vanadium spline. 

This requires a vanadium to have been previously created
with the :ref:`create_vanadium_pearl_isis-powder-diffraction-ref`
method

Accepted values are: **True** or **False**

Example Input:

.. code-block:: Python

  pearl_example.focus(vanadium_normalisation=True, ...)

.. _advanced_parameters_pearl_isis-powder-diffraction-ref:

Advanced Parameters
--------------------
.. warning:: These values are not intended to be changed and should
             reflect optimal defaults for the instrument. For more
             details please read: 
             :ref:`instrument_advanced_properties_isis-powder-diffraction-ref`
             
             This section is mainly intended to act as reference of the
             current settings distributed with Mantid

All values changed in the advanced configuration file
requires the user to restart Mantid for the new values to take effect. 
Please read :ref:`instrument_advanced_properties_isis-powder-diffraction-ref`
before proceeding to change values within the advanced configuration file.

.. _focused_cropping_values_pearl_isis-powder-diffraction-ref:

focused_cropping_values
^^^^^^^^^^^^^^^^^^^^^^^

.. _monitor_lambda_crop_range_pearl_isis-powder-diffraction-ref:

monitor_lambda_crop_range
^^^^^^^^^^^^^^^^^^^^^^^^^

.. _monitor_integration_range_pearl_isis-powder-diffraction-ref:

monitor_integration_range
^^^^^^^^^^^^^^^^^^^^^^^^^

.. _monitor_spectrum_number_pearl_isis-powder-diffraction-ref:

monitor_spectrum_number
^^^^^^^^^^^^^^^^^^^^^^^

.. _monitor_spline_coefficient_pearl_isis-powder-diffraction-ref:

monitor_spline_coefficient
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. _raw_data_tof_cropping_pearl_isis-powder-diffraction-ref:

raw_data_tof_cropping
^^^^^^^^^^^^^^^^^^^^^

.. _spline_coefficient_pearl_isis-powder-diffraction-ref:

spline_coefficient
^^^^^^^^^^^^^^^^^^

.. _tt35_grouping_filename_pearl_isis-powder-diffraction-ref:

tt35_grouping_filename
^^^^^^^^^^^^^^^^^^^^^^

.. _tt70_grouping_filename_pearl_isis-powder-diffraction-ref:

tt70_grouping_filename
^^^^^^^^^^^^^^^^^^^^^^

.. _tt88_grouping_filename_pearl_isis-powder-diffraction-ref:

tt88_grouping_filename
^^^^^^^^^^^^^^^^^^^^^^

.. _vanadium_absorb_filename_pearl_isis-powder-diffraction-ref:

vanadium_absorb_filename
^^^^^^^^^^^^^^^^^^^^^^^^

.. _vanadium_tof_cropping_pearl_isis-powder-diffraction-ref:

vanadium_tof_cropping
^^^^^^^^^^^^^^^^^^^^^