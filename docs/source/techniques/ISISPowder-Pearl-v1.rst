.. _isis-powder-diffraction-pearl-ref:

=====================================================
ISIS Powder Diffraction Scripts - PEARL Reference
=====================================================

.. contents:: Table of Contents
    :local:

.. _creating_pearl_object-isis-powder-diffraction-ref:

Creating PEARL Object
----------------------

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

..  code-block:: python

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
- :ref:`create_cal_pearl_isis-powder-diffraction-ref`

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

..  code-block:: python

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

- :ref:`attenuation_file_pearl_isis-powder-diffraction-ref`

The following parameters may also be optionally set:

- :ref:`file_ext_pearl_isis-powder-diffraction-ref`
- :ref:`suffix_pearl_isis-powder-diffraction-ref`

Example
=======

..  code-block:: python

  # Notice how the filename ends with .yaml
  cal_mapping_file = r"C:\path\to\cal_mapping.yaml"

  pearl_example.focus(calibration_mapping_file=cal_mapping_file,
                      focus_mode="all", long_mode=True,
                      perform_attenuation=True,
                      attenuation_file="ZTA",
                      run_number="100-110", tt_mode="tt88",
                      vanadium_normalisation=True)

.. _create_cal_pearl_isis-powder-diffraction-ref:

create_cal
^^^^^^^^^^
The *create_cal* method creates the offset calibration file for PEARL
scripts. The following parameters are required:

- :ref:`calibration_mapping_file_pearl_isis-powder-diffraction-ref`
- :ref:`focus_mode_pearl_isis-powder-diffraction-ref`
- :ref:`long_mode_pearl_isis-powder-diffraction-ref`
- :ref:`run_number_pearl_isis-powder-diffraction-ref`

Example
=======

.. code-block:: python

  # Notice how the filename ends with .yaml
  cal_mapping_file = r"C:\path\to\cal_mapping.yaml"

  pearl_example.create_cal(run_number=95671,
                           tt_mode="tt70",
                           long_mode=True,
                           calibration_mapping_file=cal_mapping_file)


.. _state_for_pearl_isis-powder-diffraction-ref:

How the PEARL object holds state
--------------------------------

The PEARL object does not behave as described in
:ref:`how_objects_hold_state_isis-powder-diffraction-ref`. For PEARL,
any settings given in the constructor for the PEARL object, either
explicitly or via a config file, are taken as defaults. If these are
overridden in a call to either
:ref:`focus_pearl_isis-powder-diffraction-ref`,
:ref:`create_vanadium_pearl_isis-powder-diffraction-ref` or
:ref:`create_cal_pearl_isis-powder-diffraction-ref`, then these
settings only apply to that line, and are reverted to the defaults
when the line has finished executing.

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

.. _absorb_out_filename_isis-powder-diffraction-ref:

absorb_corrections_out_filename
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Required if :ref:`gen_absorb_pearl_isis-powder-diffraction-ref` is set to **True**.

The full path to save generated absorption corrects file to.

*Note: The path to the file must include the file extension*

Example Input:

.. code-block:: python

  pearl_example.focus(generate_absorb_corrections=True,
                      absorb_corrections_out_filename=r"C:\PEARL\pearl_absorb_sphere_17_2.nxs"...)

.. _attenuation_file_pearl_isis-powder-diffraction-ref:

attenuation_file
^^^^^^^^^^^^^^^^
Required if :ref:`perform_attenuation_pearl_isis-powder-diffraction-ref`
is set to **True**

The name of the attenuation file to use within the
:ref:`focus_pearl_isis-powder-diffraction-ref` method. The name must match the name
of one of the entries in the :ref:`attenuation_files_pearl_isis-powder-diffraction-ref` setting

The workspace will be attenuated with the specified file
if the :ref:`focus_mode_pearl_isis-powder-diffraction-ref`
is set to **all** or **trans**. For more details see
:ref:`PearlMCAbsorption<algm-PearlMCAbsorption>`

Example Input:

..  code-block:: python

  pearl_example.focus(attenuation_file="ZTA", ...)

.. _attenuation_files_pearl_isis-powder-diffraction-ref:

attenuation_files
^^^^^^^^^^^^^^^^^

A list of attenuation file names and paths that are available for use in the Focus process.
It is expected that this setting will be specified in a configuration yaml file as follows:

.. code-block:: yaml

  attenuation_files:
    - name : ZTA
      path : C:\path\to\anvil_atten_files\PRL112_DC25_10MM_FF.OUT
    - name : WC
      path : C:\path\to\anvil_atten_files\PRL985_WC_HOYBIDE_NK_10MM_FF.OUT

It can alternatively be supplied as part of the call to focus:

..  code-block:: python

  pearl_example.focus(attenuation_file="ZTA", attenuation_files=r'[{"name": "ZTA", "path": r"C:\path\to\anvil_atten_files\PRL112_DC25_10MM_FF.OUT"}]' ...)

*Note: The path to the file must include the file extension*

.. _calibration_directory_pearl_isis-powder-diffraction-ref:

calibration_directory
^^^^^^^^^^^^^^^^^^^^^
This parameter should be the full path to the calibration folder.
Within the folder the following should be present:

- Grouping .cal files:

  - :ref:`tt35_grouping_filename_pearl_isis-powder-diffraction-ref`
  - :ref:`tt70_grouping_filename_pearl_isis-powder-diffraction-ref`
  - :ref:`tt88_grouping_filename_pearl_isis-powder-diffraction-ref`
- Vanadium Absorption File
  (see: :ref:`vanadium_absorb_filename_pearl_isis-powder-diffraction-ref`)
- Folder(s) with the label name specified in mapping file (e.g. "1_1")

  - Inside each folder should be the offset file with name specified in mapping file

The script will also save out vanadium splines into the relevant
label folder which are subsequently loaded and used within the
:ref:`focus_pearl_isis-powder-diffraction-ref` method.

Example Input:

..  code-block:: python

  pearl_example = Pearl(calibration_directory=r"C:\path\to\calibration_dir", ...)

.. _calibration_mapping_file_pearl_isis-powder-diffraction-ref:

calibration_mapping_file
^^^^^^^^^^^^^^^^^^^^^^^^
This parameter gives the full path to the YAML file containing the
calibration mapping. For more details on this file see:
:ref:`calibration_mapping_pearl_isis-powder-diffraction-ref`

*Note: This should be the full path to the file including extension*

Example Input:

..  code-block:: python

  # Notice the filename always ends in .yaml
  pearl_example = Pearl(calibration_mapping_file=r"C:\path\to\file\calibration_mapping.yaml", ...)

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

..  code-block:: python

  # Notice the filename always ends in .yaml
  pearl_example = Pearl(config_file=r"C:\path\to\file\configuration.yaml", ...)

.. _custom_grouping_filename_pearl_isis-powder-diffraction-ref:

custom_grouping_filename
^^^^^^^^^^^^^^^^^^^^^^^^
The name of a custom grouping cal file to use. The file needs to be located
within top level of the :ref:`calibration_directory_pearl_isis-powder-diffraction-ref`

..  code-block:: python

  custom_grouping_filename: "DAC_group.cal"

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

..  code-block:: python

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

..  code-block:: python

  pearl_example.focus(file_ext=".s01", ...)

.. _focus_mode_pearl_isis-powder-diffraction-ref:

focus_mode
^^^^^^^^^^
Determines how the banks are grouped when using the
:ref:`focus_pearl_isis-powder-diffraction-ref` method.
Each mode is further described below.

Accepted values are: **all**, **groups**, **mods**, **trans_subset** and **trans**

all
====
In all mode banks 1-9 (inclusive) are summed into a single spectra
then scaled down to 1/9 of their original values.

The workspace is also attenuated if
:ref:`perform_attenuation_pearl_isis-powder-diffraction-ref`
is set to **True**.

Workspaces containing banks 10-14 are left as
separate workspaces with appropriate names.

groups
======
In groups mode banks 1+2+3, 4+5+6, 7+8+9 are summed into three (3)
separate workspaces. Each workspace is scaled down to a 1/3 of original scale.

The workspaces containing banks 4-9 (inclusive) are then added
into a separate workspace and scaled down to 1/2 original scale.

Banks 10-14 are left as separate workspaces with appropriate names.

trans
======
In trans mode banks 1-9 (inclusive) are summed into a single spectrum
then scaled down to 1/9 original scale.

The workspace is also attenuated if
:ref:`perform_attenuation_pearl_isis-powder-diffraction-ref`
is set to **True**.

All banks are also output as individual workspaces with appropriate names
with no additional processing applied.

trans_subset
============
This mode behaves the same as **trans** except the user can optionally supply which modules in the transverse banks to
focus/sum using the input parameter e.g. *trans_mod_nums="1-3,5"* which would focus modules 1,2,3 and 5. The output
spectrum is similarly normalised by the number of modules requested.

If any module numbers are duplicated or outside the range 1-9 inclusive then all transverse modules are included -
i.e. it defaults to the behaviour of *focus_mode="trans"* and the *trans_mod_nums* arguemnt is ignored.

mods
====
In mods mode every bank is left as individual workspaces with
appropriate names. No additional processing is performed.

Example Input:

..  code-block:: python

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
between 20,000-40,000 μs

This also affects the :ref:`advanced_parameters_pearl_isis-powder-diffraction-ref`
used. More detail can be found for each individual parameter
listed under the advanced parameters section.

Accepted values are: **True** or **False**

Example Input:

..  code-block:: python

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

..  code-block:: python

  pearl_example = Pearl(output_directory=r"C:\path\to\output_dir", ...)

.. _perform_attenuation_pearl_isis-powder-diffraction-ref:

perform_attenuation
^^^^^^^^^^^^^^^^^^^^
Indicates whether to perform attenuation corrections
whilst running :ref:`focus_pearl_isis-powder-diffraction-ref`.
For more details of the corrections performed see:
:ref:`PearlMCAbsorption<algm-PearlMCAbsorption>`

If this is set to **True**
:ref:`attenuation_file_pearl_isis-powder-diffraction-ref`
must be set too.

*Note: This correction will only be performed if 'focus_mode'
is in* **All** or **Trans**.
See: :ref:`focus_mode_pearl_isis-powder-diffraction-ref`
for more details.

Accepted values are: **True** or **False**

Example Input:

..  code-block:: python

  pearl_example.focus(perform_attenuation=True, ...)

.. _run_in_cycle_pearl_isis-powder-diffraction-ref:

run_in_cycle
^^^^^^^^^^^^
Indicates a run from the current cycle to use when calling
:ref:`create_vanadium_pearl_isis-powder-diffraction-ref`.
This does not have the be the first run of the cycle or
the run number corresponding to the vanadium. However it
must be in the correct cycle according to the
:ref:`calibration_mapping_pearl_isis-powder-diffraction-ref`.

Example Input:

..  code-block:: python

  # In this example assume we mean a cycle with run numbers 100-200
  pearl_example.create_vanadium(run_in_cycle=100, ...)

.. _run_number_pearl_isis-powder-diffraction-ref:

run_number
^^^^^^^^^^
Specifies the run number(s) to process when calling the
:ref:`focus_pearl_isis-powder-diffraction-ref` and
:ref:`create_cal_pearl_isis-powder-diffraction-ref` methods.

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

..  code-block:: python

  # Sum and process run numbers 1, 3, 5, 6, 7
  pearl_example.focus(run_number="1, 3, 5-7", ...)
  # Or just a single run
  pearl_example.focus(run_number=100, ...)

.. _suffix_pearl_isis-powder-diffraction-ref:

suffix
^^^^^^
*Optional*

This parameter specifies a suffix to append the names of output files
during a focus.

Example Input:

.. code-block:: python

  pearl_example.focus(suffix="-corr", ...)

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
- :ref:`custom_grouping_filename_pearl_isis-powder-diffraction-ref`

Accepted values are: **tt35**, **tt70**, **tt80** and custom

When calling :ref:`create_vanadium_pearl_isis-powder-diffraction-ref`
**all** can be used to implicitly process all of the ttXX
values indicated above.

When the custom tt_mode is used a focus mode of "Mods" is always used

Example Input:

..  code-block:: python

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

..  code-block:: python

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

..  code-block:: python

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

.. _create_cal_rebin_1_params_pearl_isis-powder-diffraction-ref:

create_cal_rebin_1_params
^^^^^^^^^^^^^^^^^^^^^^^^^
The rebin parameters to use in the first rebin operation in
:ref:`create_cal_pearl_isis-powder-diffraction-ref`. On PEARL this is
set to the following:

.. code-block:: python

  # Long mode OFF:
        create_cal_rebin_1_params: "100,-0.0006,19950"

  # Long mode ON:
        create_cal_rebin_1_params: "20300,-0.0006,39990"


.. _create_cal_rebin_2_params_pearl_isis-powder-diffraction-ref:

create_cal_rebin_2_params
^^^^^^^^^^^^^^^^^^^^^^^^^
The rebin parameters to use in the second rebin operation in
:ref:`create_cal_pearl_isis-powder-diffraction-ref`. On PEARL this is
set to the following:

.. code-block:: python

  create_cal_rebin_2_params: "1.8,0.002,2.1"


.. _cross_corr_reference_spectra_pearl_isis-powder-diffraction-ref:

cross_corr_reference_spectra
^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The Workspace Index of the spectra to correlate all other spectra
against in the cross-correlation step of
:ref:`create_cal_pearl_isis-powder-diffraction-ref`. On PEARL this is
set to the following:

.. code-block:: python

  cross_corr_reference_spectra: 20


.. _cross_corr_ws_index_max_pearl_isis-powder-diffraction-ref:

cross_corr_ws_index_max
^^^^^^^^^^^^^^^^^^^^^^^
The workspace index of the last member of the range of spectra to
cross-correlate against in
:ref:`create_cal_pearl_isis-powder-diffraction-ref`. On PEARL this is
set to the following:

.. code-block:: python

  cross_corr_ws_index_max: 1063


.. _cross_corr_ws_index_min_pearl_isis-powder-diffraction-ref:

cross_corr_ws_index_min
^^^^^^^^^^^^^^^^^^^^^^^
The workspace index of the first member of the range of spectra to
cross-correlate against in
:ref:`create_cal_pearl_isis-powder-diffraction-ref`. On PEARL this is
set to the following:

.. code-block:: python

  cross_corr_ws_index_min: 9


.. _cros_cor_x_max_pearl_isis-powder-diffraction-ref:

cross_cor_x_max
^^^^^^^^^^^^^^^
The ending point of the region to be cross correlated in
:ref:`create_cal_pearl_isis-powder-diffraction-ref`. On PEARL this is
set to the following:

.. code-block:: python

  cross_corr_x_max: 2.1


.. _cros_corr_x_min_pearl_isis-powder-diffraction-ref:

cross_cor_x_min
^^^^^^^^^^^^^^^
The starting point of the region to be cross correlated in
:ref:`create_cal_pearl_isis-powder-diffraction-ref`. On PEARL this is
set to the following:

.. code-block:: python

  cross_corr_x_min: 1.8

.. _custom_focused_bin_widths_pearl_isis-powder-diffraction-ref:

custom_focused_bin_widths
^^^^^^^^^^^^^^^^^^^^^^^^^
The dt-upon-t binning for the focused data when using tt_mode=custom

On PEARL this is set to -0.0006 for all banks in the custom grouping file

.. _custom_focused_cropping_values_pearl_isis-powder-diffraction-ref:

custom_focused_cropping_values
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Indicates a list of TOF values to crop the focused workspace
which was created by :ref:`focus_pearl_isis-powder-diffraction-ref`
on a bank by bank basis when using tt_mode=custom

.. _focused_bin_widths_pearl_isis-powder-diffraction-ref:

focused_bin_widths
^^^^^^^^^^^^^^^^^^
The dt-upon-t binning for the focused data.

On PEARL this is set to -0.0006 for all 14 banks

.. _focused_cropping_values_pearl_isis-powder-diffraction-ref:

focused_cropping_values
^^^^^^^^^^^^^^^^^^^^^^^
Indicates a list of TOF values to crop the focused workspace
which was created by :ref:`focus_pearl_isis-powder-diffraction-ref`
on a bank by bank basis.

This parameter is a list of bank cropping values with
one list entry per bank. The values **must** have a smaller
TOF window than the :ref:`vanadium_tof_cropping_pearl_isis-powder-diffraction-ref`

*Note: The value passed with the*
:ref:`long_mode_pearl_isis-powder-diffraction-ref` *parameter
determines the set of values used.*

On PEARL this is set to the following TOF windows:

..  code-block:: python

  # Long mode OFF:
        focused_cropping_values: [
        (1500, 19900),  # Bank 1
        (1500, 19900),  # Bank 2
        (1500, 19900),  # Bank 3
        (1500, 19900),  # Bank 4
        (1500, 19900),  # Bank 5
        (1500, 19900),  # Bank 6
        (1500, 19900),  # Bank 7
        (1500, 19900),  # Bank 8
        (1500, 19900),  # Bank 9
        (1500, 19900),  # Bank 10
        (1500, 19900),  # Bank 11
        (1500, 19900),  # Bank 12
        (1500, 19900),  # Bank 13
        (1500, 19900)   # Bank 14
      ]

  # Long mode ON:
        focused_cropping_values: [
        (20300, 38830),  # Bank 1
        (20300, 38830),  # Bank 2
        (20300, 38830),  # Bank 3
        (20300, 38830),  # Bank 4
        (20300, 38830),  # Bank 5
        (20300, 38830),  # Bank 6
        (20300, 38830),  # Bank 7
        (20300, 38830),  # Bank 8
        (20300, 38830),  # Bank 9
        (20300, 38830),  # Bank 10
        (20300, 38830),  # Bank 11
        (20300, 38830),  # Bank 12
        (20300, 38830),  # Bank 13
        (20300, 38830)   # Bank 14
      ]


.. _gen_absorb_pearl_isis-powder-diffraction-ref:

generate_absorb_corrections
^^^^^^^^^^^^^^^^^^^^^^^^^^^
Indicates whether to generate the absorption corrections, instead of
reading them from a file. Allowed values are: **True** and
**False**. If set to **True**,
:ref:`absorb_out_filename_isis-powder-diffraction-ref` must also be
set. On PEARL this is set to the following:

.. code-block:: python

  generate_absorb_corrections: False

.. _get_det_offsets_d_ref_pearl_isis-powder-diffraction-ref:

get_det_offsets_d_ref
^^^^^^^^^^^^^^^^^^^^^
Center of reference peak in d-space for GetDetectorOffsets in
:ref:`create_cal_pearl_isis-powder-diffraction-ref`. On PEARL this is
set to the following:

.. code-block:: python

  get_det_offsets_d_ref: 1.912795


.. _get_det_offsets_step_pearl_isis-powder-diffraction-ref:

get_det_offsets_step
^^^^^^^^^^^^^^^^^^^^
Step size used to bin d-spacing data in GetDetectorOffsets when
running :ref:`create_cal_pearl_isis-powder-diffraction-ref`. On PEARL
this is set to the following:

.. code-block:: python

  get_det_offsets_step: 0.002


.. _get_det_offsets_x_max_pearl_isis-powder-diffraction-ref:

get_det_offsets_x_max
^^^^^^^^^^^^^^^^^^^^^
Maximum of CrossCorrelation data to search for peak, usually negative,
in :ref:`create_cal_pearl_isis-powder-diffraction-ref`. On PEARL this
is set to the following:

.. code-block:: python

  get_det_offsets_x_max: -200


.. _get_det_offsets_x_min_pearl_isis-powder-diffraction-ref:

get_det_offsets_x_min
^^^^^^^^^^^^^^^^^^^^^
Minimum of CrossCorrelation data to search for peak, usually negative,
in :ref:`create_cal_pearl_isis-powder-diffraction-ref`. On PEARL this
is set to the following:

.. code-block:: python

  get_det_offsets_x_min: -200


.. _monitor_lambda_crop_range_pearl_isis-powder-diffraction-ref:

monitor_lambda_crop_range
^^^^^^^^^^^^^^^^^^^^^^^^^
The range in dSpacing to crop a monitor spectra to when generating a
spline of the current to the target. This is should be stored as a tuple of
both values (lower and upper bound).

*Note: The value passed with the*
:ref:`long_mode_pearl_isis-powder-diffraction-ref` *parameter
determines the set of values used.*

On PEARL this is set to the following:

..  code-block:: python

  # Long mode OFF:
    monitor_lambda_crop_range: (0.03, 6.00)

  # Long mode ON:
    monitor_lambda_crop_range: (5.9, 12.0)

.. _monitor_integration_range_pearl_isis-powder-diffraction-ref:

monitor_integration_range
^^^^^^^^^^^^^^^^^^^^^^^^^
The maximum and minimum values for a bin whilst
integrating the monitor spectra.
Any values that fall outside of this range will not be considered.
This should be stored as a tuple of both values (lower and upper bound).
See: :ref:`Integration<algm-Integration>` for more details.

*Note: The value passed with the*
:ref:`long_mode_pearl_isis-powder-diffraction-ref` *parameter
determines the set of values used.*

On PEARL this is set to the following:

..  code-block:: python

  # Long mode OFF:
  monitor_integration_range: (0.6, 5.0)

  # Long mode ON:
  monitor_integration_range: (6, 10)

.. _monitor_mask_regions_pearl_isis-powder-diffraction-ref:

monitor_mask_regions
^^^^^^^^^^^^^^^^^^^^

The masks applied to monitor spectra when normalising a workspace.

On PEARL this is set to the following:

.. code-block:: python

  monitor_mask_regions: [[3.45, 2.96, 2.1,  1.73],
                         [3.7,  3.2,  2.26, 1.98]]

.. _monitor_spectrum_number_pearl_isis-powder-diffraction-ref:

monitor_spectrum_number
^^^^^^^^^^^^^^^^^^^^^^^
The workspace spectrum number that represents a
monitor which can be used to calculate current.

On PEARL this is set to the following:

..  code-block:: python

  monitor_spectrum_number: 1,


.. _monitor_spline_coefficient_pearl_isis-powder-diffraction-ref:

monitor_spline_coefficient
^^^^^^^^^^^^^^^^^^^^^^^^^^
Determines the spline coefficient to use whilst
processing the monitor spectra to normalise by
current. For more details see:
:ref:`SplineBackground <algm-SplineBackground>`

On PEARL this is set to the following:

..  code-block:: python

  monitor_spline_coefficient: 20

.. _raw_data_tof_cropping_pearl_isis-powder-diffraction-ref:

raw_data_tof_cropping
^^^^^^^^^^^^^^^^^^^^^
Determines the TOF window to crop all spectra down to before any
processing in the :ref:`create_vanadium_pearl_isis-powder-diffraction-ref`
and :ref:`focus_pearl_isis-powder-diffraction-ref` methods.

This helps remove negative counts where at very low TOF
the empty counts can exceed the captured neutron counts
of the run to process. It also is used
to crop to the correct TOF window depending on the
value of the :ref:`long_mode_pearl_isis-powder-diffraction-ref` parameter.

*Note: The value passed with the*
:ref:`long_mode_pearl_isis-powder-diffraction-ref` *parameter
determines the set of values used.*

On PEARL this is set to the following:

..  code-block:: python

  # Long mode OFF:
    raw_data_tof_cropping: (0, 19995)

  # Long mode ON:
    raw_data_tof_cropping: (20280, 39000)

.. _spline_coefficient_pearl_isis-powder-diffraction-ref:

spline_coefficient
^^^^^^^^^^^^^^^^^^
Determines the spline coefficient to use after processing
the vanadium in :ref:`create_vanadium_pearl_isis-powder-diffraction-ref`
method. For more details see: :ref:`SplineBackground <algm-SplineBackground>`
This value is lower on long-mode to deal with the increased noise.

*Note that if this value is changed 'create_vanadium'
will need to be called again.*

On PEARL this is set to the following:

..  code-block:: python

  # Long mode OFF:
    spline_coefficient: 60

  # Long mode ON:
    spline_coefficient: 5

.. _subtract_empty_instrument_pearl_isis-powder-diffraction-ref:

subtract_empty_instrument
^^^^^^^^^^^^^^^^^^^^^^^^^
Provides the option to disable subtracting empty instrument runs from
the run being focused. This is useful for focusing empties, as
subtracting an empty from itself, or subtracting the previous cycle's
empty from this cycle's, creates meaningless data. Set to **False** to
disable empty subtraction.

On PEARL this is set to the following:

.. code-block:: python

  subtract_empty_instrument: True

.. _tt35_grouping_filename_pearl_isis-powder-diffraction-ref:

tt35_grouping_filename
^^^^^^^^^^^^^^^^^^^^^^
Determines the name of the grouping cal file which is located
within top level of the :ref:`calibration_directory_pearl_isis-powder-diffraction-ref`
if :ref:`tt_mode_pearl_isis-powder-diffraction-ref` is set to **tt35**

The grouping file determines the detector ID to bank mapping to use
whilst focusing the spectra into banks.

On PEARL this is set to the following:

..  code-block:: python

  tt35_grouping_filename: "pearl_group_12_1_TT35.cal"

.. _tt70_grouping_filename_pearl_isis-powder-diffraction-ref:

tt70_grouping_filename
^^^^^^^^^^^^^^^^^^^^^^
Determines the name of the grouping cal file which is located
within top level of the :ref:`calibration_directory_pearl_isis-powder-diffraction-ref`
if :ref:`tt_mode_pearl_isis-powder-diffraction-ref` is set to **tt70**

The grouping file determines the detector ID to bank mapping to use
whilst focusing the spectra into banks.

On PEARL this is set to the following:

..  code-block:: python

  tt70_grouping_filename: "pearl_group_12_1_TT70.cal"

.. _tt88_grouping_filename_pearl_isis-powder-diffraction-ref:

tt88_grouping_filename
^^^^^^^^^^^^^^^^^^^^^^
Determines the name of the grouping cal file which is located
within top level of the :ref:`calibration_directory_pearl_isis-powder-diffraction-ref`
if :ref:`tt_mode_pearl_isis-powder-diffraction-ref` is set to **tt88**

The grouping file determines the detector ID to bank mapping to use
whilst focusing the spectra into banks.

On PEARL this is set to the following:

..  code-block:: python

  tt88_grouping_filename: "pearl_group_12_1_TT88.cal"

.. _vanadium_absorb_filename_pearl_isis-powder-diffraction-ref:

vanadium_absorb_filename
^^^^^^^^^^^^^^^^^^^^^^^^
Determines the name of the precalculated vanadium absorption
correction values to apply when running
:ref:`create_vanadium_pearl_isis-powder-diffraction-ref`.

This file must be located within the top level of the
:ref:`calibration_directory_pearl_isis-powder-diffraction-ref`
directory.

On PEARL this is set to the following:

..  code-block:: python

  vanadium_absorb_filename: "pearl_absorp_sphere_10mm_newinst2_long.nxs"

.. _nxs_filename_pearl_isis-powder-diffraction-ref:

nxs_filename
^^^^^^^^^^^^
A template for the filename of the generated NeXus file.

.. _gss_filename_pearl_isis-powder-diffraction-ref:

gss_filename
^^^^^^^^^^^^
A template for the filename of the generated GSAS file.

.. _dat_files_directory_pearl_isis-powder-diffraction-ref:

dat_files_directory
^^^^^^^^^^^^^^^^^^^
The subdirectory of the output directory where the .xye files are saved

.. _tof_xye_filename_pearl_isis-powder-diffraction-ref:

tof_xye_filename
^^^^^^^^^^^^^^^^
A template for the filename of the generated TOF XYE file.

.. _dspacing_xye_filename_pearl_isis-powder-diffraction-ref:

dspacing_xye_filename
^^^^^^^^^^^^^^^^^^^^^
A template for the filename of the generated dSpacing XYE file.


.. _vanadium_tof_cropping_pearl_isis-powder-diffraction-ref:

vanadium_tof_cropping
^^^^^^^^^^^^^^^^^^^^^
Determines the TOF window to crop all banks to
within the :ref:`create_vanadium_pearl_isis-powder-diffraction-ref`
method. This is applied after focusing and before a spline is taken.

It is used to remove low counts at the start and end of the vanadium run
to produce a spline which better matches the data.

This parameter is stored as a tuple of both values (lower and upper bound).
The values **must** have a larger TOF window than the
:ref:`focused_cropping_values_pearl_isis-powder-diffraction-ref`
and a smaller window than :ref:`raw_data_tof_cropping_pearl_isis-powder-diffraction-ref`.

*Note: The value passed with the*
:ref:`long_mode_pearl_isis-powder-diffraction-ref` *parameter
determines the set of values used.*

On PEARL this is set to the following:

..  code-block:: python

  # Long mode OFF:
    vanadium_tof_cropping: (1400, 19990)
  # Long mode ON:
    vanadium_tof_cropping: (20295, 39000)

.. categories:: Techniques
