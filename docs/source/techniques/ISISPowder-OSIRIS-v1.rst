.. _isis-powder-diffraction-osiris-ref:

=====================================================
ISIS Powder Diffraction Scripts - OSIRIS Reference
=====================================================

.. contents:: Table of Contents
    :local:

.. _creating_osiris_object_isis-powder-diffraction-ref:

Creating OSIRIS Object
------------------------
This method assumes you are familiar with :ref:`intro_to_objects-isis-powder-diffraction-ref`

To create a OSIRIS object the following parameters are required:

- :ref:`calibration_directory_osiris_isis-powder-diffraction-ref`
- :ref:`output_directory_osiris_isis-powder-diffraction-ref`
- :ref:`user_name_osiris_isis-powder-diffraction-ref`

Optionally a configuration file may be specified if one exists
using the following parameter:

- :ref:`config_file_osiris_isis-powder-diffraction-ref`

See :ref:`configuration_files_isis-powder-diffraction-ref`
on YAML configuration files for more details

Example
^^^^^^^

..  code-block:: python

  from isis_powder.osiris import Osiris

  calibration_dir = r"C:\path\to\calibration_dir"
  output_dir = r"C:\path\to\output_dir"
  config_dir = r"C:\path\to\config.yaml"

  osiris_example = Osiris(user_name="Calib",
                          calibration_directory=calibration_dir,
                          output_directory=output_dir,
                          config_file=config_dir)

Methods
--------
The following methods can be executed on a OSIRIS object:

- :ref:`focus_osiris_isis-powder-diffraction-ref`
- :ref:`create_vanadium_osiris_isis-powder-diffraction-ref`

For information on creating a OSIRIS object see:
:ref:`creating_osiris_object_isis-powder-diffraction-ref`

.. _create_vanadium_osiris_isis-powder-diffraction-ref:

create_vanadium
^^^^^^^^^^^^^^^
The *create_vanadium* method allows a user to process a vanadium run.

On OSIRIS the following parameters are required when executing *create_vanadium*:

- :ref:`run_number_osiris_isis-powder-diffraction-ref`
- :ref:`subtract_empty_can_osiris_isis-powder-diffraction-ref`

Example
=======

..  code-block:: python

  osiris_example.create_vanadium(run_number="119977", subtract_empty_can=False)

.. _focus_osiris_isis-powder-diffraction-ref:

focus
^^^^^^^^^^^^^^^^^^^^^^^^
The *focus* method allows a user to process a series of runs into a
focused dSpace workspace. Whilst processing the runs the scripts can apply any corrections
the user enables.

The available corrections are:

- empty container subtraction, which can be enabled using :ref:`subtract_empty_can_osiris_isis-powder-diffraction-ref` parameter.

- vanadium normalization, which can be enabled using :ref:`do_van_normalisation_osiris_isis-powder-diffraction-ref` parameter.

On OSIRIS the following parameters are required when executing *focus*:

- :ref:`calibration_mapping_file_osiris_isis-powder-diffraction-ref`
- :ref:`do_van_normalisation_osiris_isis-powder-diffraction-ref`
- :ref:`subtract_empty_can_osiris_isis-powder-diffraction-ref`
- :ref:`merge_drange_osiris_isis-powder-diffraction-ref`

Example
=======
..  code-block:: python

  # Notice how the filename ends with .yaml
  cal_mapping_file = r"C:\path\to\cal_mapping.yaml"

  osiris_example.run_diffraction_focusing(run_number="119977-119988",
                                          merge_drange=True,
                                          subtract_empty_can=True,
                                          vanadium_normalisation=True,
                                          calibration_mapping_file=cal_mapping_file)
.. _calibration_mapping_osiris-isis-powder-ref:

Calibration Mapping File
-------------------------
The calibration mapping file holds the mapping between
run numbers, current label, offset filename, empty can run numbers,
and vanadium run numbers.

For more details on the calibration mapping file see:
:ref:`cycle_mapping_files_isis-powder-diffraction-ref`

The layout on OSIRIS should look as follows for each block:

.. code-block:: yaml
  :linenos:

  1-120:
    label: "1_1"
    offset_file_name: "offset_file.cal"
    vanadium_drange1 : "13"
    vanadium_drange2 : "14"
    vanadium_drange3 : "15"
    vanadium_drange4 : "16"
    vanadium_drange5 : "17"
    vanadium_drange6 : "18"
    vanadium_drange7 : "19"
    vanadium_drange8 : "20"
    vanadium_drange9 : "21"
    vanadium_drange10 : "22"
    vanadium_drange11 : "23"
    vanadium_drange12 : "24"
    vanadium_run_numbers : "13-24"
    empty_drange1 : "1"
    empty_drange2 : "2"
    empty_drange3 : "3"
    empty_drange4 : "4"
    empty_drange5 : "5"
    empty_drange6 : "6"
    empty_drange7 : "7"
    empty_drange8 : "8"
    empty_drange9 : "9"
    empty_drange10 : "10"
    empty_drange11 : "11"
    empty_drange12 : "12"
    empty_can_run_numbers : "1-12"

For each drange, any empty containers and vanadium runs must be listed with the associated drange.


Parameters
-----------
The following parameters for OSIRIS are intended for regular use
when using the ISIS Powder scripts.

.. _calibration_directory_osiris_isis-powder-diffraction-ref:

calibration_directory
^^^^^^^^^^^^^^^^^^^^^
This parameter should be the full path to the calibration folder.
Within the folder the following should be present:

- Grouping .cal file
- Folder(s) with the label name specified in mapping file (e.g. "1_1")
  - Inside each folder should be the offset file with name specified in the mapping file

Example Input:

..  code-block:: python

  osiris_example = Osiris(calibration_directory=r"C:\path\to\calibration_dir", ...)

.. _calibration_mapping_file_osiris_isis-powder-diffraction-ref:

calibration_mapping_file
^^^^^^^^^^^^^^^^^^^^^^^^^
This parameter gives the full path to the YAML file containing the
calibration mapping. For more details on this file see:
:ref:`calibration_mapping_osiris-isis-powder-ref`

*Note: This should be the full path to the file including extension*

Example Input:

..  code-block:: python

  # Notice the filename always ends in .yaml
  osiris_example = Osiris(calibration_mapping_file=r"C:\path\to\file\calibration_mapping.yaml", ...)

.. _config_file_osiris_isis-powder-diffraction-ref:

config_file
^^^^^^^^^^^
The full path to the YAML configuration file. This file is
described in detail here: :ref:`configuration_files_isis-powder-diffraction-ref`
It is recommended to set this parameter at object creation instead
of on a method as it will warn if any parameters are overridden
in the scripting window.

*Note: This should be the full path to the file including extension*

Example Input:

..  code-block:: python

  # Notice the filename always ends in .yaml
  osiris_example = Osiris(config_file=r"C:\path\to\file\configuration.yaml", ...)

.. _do_van_normalisation_osiris_isis-powder-diffraction-ref:

do_van_normalisation
^^^^^^^^^^^^^^^^^^^^
Indicates whether to divide the focused workspace within
:ref:`focus_osiris_isis-powder-diffraction-ref` mode with an
associated vanadium run.

Accepted values are: **True** or **False**

Example Input:

..  code-block:: python

  osiris_example = Osiris(do_van_normalisation=True, ...)

.. _file_ext_osiris_isis-powder-diffraction-ref:

file_ext
^^^^^^^^
*Optional*

Specifies a file extension to use for the
:ref:`focus_osiris_isis-powder-diffraction-ref` method.

This should be used to process partial runs. When
processing full runs (i.e. completed runs) it should not
be specified as Mantid will automatically determine the
best extension to use.

*Note: A leading dot (.) is not required but
is preferred for readability*

Example Input:

..  code-block:: python

  osiris_example = Osiris(file_ext=".s01", ...)

.. _merge_drange_osiris_isis-powder-diffraction-ref:

merge_drange
^^^^^^^^^^^^
Indicates whether to merge summed workspaces of different dranges after running the
:ref:`focus_osiris_isis-powder-diffraction-ref` method.

Accepted values are: **True** or **False**

Example Input:

..  code-block:: python

  osiris_example = Osiris(merge_drange=True, ...)

.. _output_directory_osiris_isis-powder-diffraction-ref:

output_directory
^^^^^^^^^^^^^^^^
Specifies the path to the output directory to save resulting files
into. The script will automatically create a folder
with the label determined from the
:ref:`calibration_mapping_file_polaris_isis-powder-diffraction-ref`
and within that create another folder for the current
:ref:`user_name_polaris_isis-powder-diffraction-ref`.

Within this folder processed data will be saved out in
several formats.

Example Input:

..  code-block:: python

  osiris_example = Osiris(output_directory=r"C:\path\to\output_dir", ...)

.. _run_number_osiris_isis-powder-diffraction-ref:

run_number
^^^^^^^^^^
Specifies the run number(s) to process when calling the
:ref:`focus_osiris_isis-powder-diffraction-ref` method.

This parameter accepts a single value or a range
of values with the following syntax:

**-** : Indicates an inclusive range of runs
(e.g. *1-10* would process 1, 2, 3....8, 9, 10)

**,** : Indicates a gap between runs
(e.g. *1, 3, 5, 7* would process run numbers 1, 3, 5, 7)

These can be combined like so:
*1-3, 5, 8-10* would process run numbers 1, 2, 3, 5, 8, 9, 10.

In addition the input_mode parameter determines what effect a range of inputs
has on the data to be processed

Example Input:

..  code-block:: python

  # Process run number 1, 3, 5, 6, 7
  osiris_example = Osiris(run_number="1, 3, 5-7", ...)
  # Or just a single run
  osiris_example = Osiris(run_number=100, ...)

.. _user_name_osiris_isis-powder-diffraction-ref:

user_name
^^^^^^^^^
Specifies the name of the current user when creating a
new OSIRIS object. This is only used when saving data to
sort data into respective user folders.

Example Input:

..  code-block:: python

  osiris_example = Osiris(user_name="Mantid", ...)

.. _subtract_empty_can_osiris_isis-powder-diffraction-ref:

subtract_empty_can
^^^^^^^^^^^^^^^^^^
Provides the option to disable subtracting empty canister runs from
the run being focused. Set to **False** to disable empty subtraction.

Example Input:

.. code-block:: python

  subtract_empty_can: True


.. categories:: Techniques
