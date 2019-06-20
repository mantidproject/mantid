.. _isis-powder-diffraction-gem-ref:

================================================
ISIS Powder Diffraction Scripts - GEM Reference
================================================

.. contents:: Table of Contents
    :local:

.. _creating_gem_object-isis-powder-diffraction-ref:

Creating GEM Object
--------------------
This method assumes you are familiar with the concept of objects in Python.
If not more details can be read here: :ref:`intro_to_objects-isis-powder-diffraction-ref`

To create a GEM object the following parameters are required:

- :ref:`calibration_directory_gem_isis-powder-diffraction-ref`
- :ref:`output_directory_gem_isis-powder-diffraction-ref`
- :ref:`user_name_gem_isis-powder-diffraction-ref`

Optionally a configuration file may be specified if one exists
using the following parameter:

- :ref:`config_file_gem_isis-powder-diffraction-ref`

See :ref:`configuration_files_isis-powder-diffraction-ref`
on YAML configuration files for more details

Example
^^^^^^^

..  code-block:: python

  from isis_powder import Gem

  calibration_dir = r"C:\path\to\calibration_dir"
  output_dir = r"C:\path\to\output_dir"

  gem_example = Gem(calibration_directory=calibration_dir,
                    output_directory=output_dir,
                    user_name="Mantid")

  # Optionally we could provide a configuration file like so
  # Notice how the file name ends with .yaml
  config_file_path = r"C:\path\to\config_file.yaml
  gem_example = Gem(config_file=config_file_path,
                    user_name="Mantid", ...)

Methods
--------
The following methods can be executed on a GEM object:

- :ref:`create_vanadium_gem_isis-powder-diffraction-ref`
- :ref:`focus_gem_isis-powder-diffraction-ref`
- :ref:`set_sample_gem_isis-powder-diffraction-ref`
- :ref:`create_cal_gem_isis-powder-diffraction-ref`

For information on creating a GEM object see:
:ref:`creating_gem_object-isis-powder-diffraction-ref`

.. _create_vanadium_gem_isis-powder-diffraction-ref:

create_vanadium
^^^^^^^^^^^^^^^^
The *create_vanadium* method allows a user to process a vanadium run.
Whilst processing the vanadium run the scripts can apply any corrections
the user enables and will spline the resulting workspace(s) for later focusing.

On GEM the following parameters are required when executing *create_vanadium*:

- :ref:`calibration_mapping_file_gem_isis-powder-diffraction-ref`
- :ref:`do_absorb_corrections_gem_isis-powder-diffraction-ref`
- :ref:`first_cycle_run_no_gem_isis-powder-diffraction-ref`
- :ref:`mode_gem_isis-powder-diffraction-ref`

If :ref:`do_absorb_corrections_gem_isis-powder-diffraction-ref` is
set to **True** the following parameter is required in addition to the
above:

- :ref:`multiple_scattering_gem_isis-powder-diffraction-ref`

Example
=======

..  code-block:: python

  # Notice how the filename ends with .yaml
  cal_mapping_file = r"C:\path\to\cal_mapping.yaml"

  gem_example.create_vanadium(calibration_mapping_file=cal_mapping_file,
                              do_absorb_corrections=True,
                              first_cycle_run_no=100,
                              mode="PDF",
                              multiple_scattering=False)

.. _focus_gem_isis-powder-diffraction-ref:

focus
^^^^^
The *focus* method processes the user specified run(s). It aligns,
focuses and optionally applies corrections if the user has requested them.

On GEM the following parameters are required when executing *focus*:

- :ref:`calibration_mapping_file_gem_isis-powder-diffraction-ref`
- :ref:`do_absorb_corrections_gem_isis-powder-diffraction-ref`
- :ref:`input_mode_gem_isis-powder-diffraction-ref`
- :ref:`mode_gem_isis-powder-diffraction-ref`
- :ref:`run_number_gem_isis-powder-diffraction-ref`
- :ref:`vanadium_normalisation_gem_isis-powder-diffraction-ref`

If :ref:`do_absorb_corrections_gem_isis-powder-diffraction-ref` is
set to **True** the following parameter is required in addition to the
above:

- :ref:`multiple_scattering_gem_isis-powder-diffraction-ref`

The following parameters may also be optionally set:

- :ref:`file_ext_gem_isis-powder-diffraction-ref`
- :ref:`sample_empty_gem_isis-powder-diffraction-ref`
- :ref:`suffix_gem_isis-powder-diffraction-ref`
- :ref:`texture_mode_isis-powder-diffraction-ref`
- :ref:`unit_to_keep_gem_isis-powder-diffraction-ref`
- :ref:`save_angles_gem_isis-powder-diffraction-ref`

If :ref:`sample_empty_gem_isis-powder-diffraction-ref` is
set then the following parameter is also required:

- :ref:`sample_empty_scale_gem_isis-powder-diffraction-ref`

Example
=======

..  code-block:: python

  # Notice how the filename ends with .yaml
  cal_mapping_file = r"C:\path\to\cal_mapping.yaml"

  gem_example.focus(calibration_mapping_file=cal_mapping_file,
                    do_absorb_corrections=False,
                    file_ext=".s01", input_mode="Individual",
                    mode="Rietveld", run_number="100-105",
                    vanadium_normalisation=True)


.. _set_sample_gem_isis-powder-diffraction-ref:

set_sample
^^^^^^^^^^^
The *set_sample* method allows a user to specify a SampleDetails
object which contains the sample properties used when
:ref:`do_absorb_corrections_gem_isis-powder-diffraction-ref` is **True**
whilst focusing.

For more details on the SampleDetails object and how to set
it see: :ref:`isis-powder-diffraction-sampleDetails-ref`

The following parameter is required when calling *set_sample*

- *sample* - This must be a SampleDetails object with the
  material set already.

Example
=======

..  code-block:: python

  sample_obj = SampleDetails(...)
  sample_obj.set_material(...)

  gem_example.set_sample(sample=sample_obj)

.. _create_cal_gem_isis-powder-diffraction-ref:

create_cal
^^^^^^^^^^
The *create_cal* method creates the offset calibration file for GEM
scripts. The following parameters are required:

- :ref:`calibration_mapping_file_gem_isis-powder-diffraction-ref`
- :ref:`run_number_pearl_isis-powder-diffraction-ref`

Example
=======

.. code-block:: python

  # Notice how the filename ends with .yaml
  cal_mapping_file = r"C:\path\to\cal_mapping.yaml"
  
  gem_example.create_cal(run_number=87618, 
                           calibration_mapping_file=cal_mapping_file)



.. _calibration_mapping_gem-isis-powder-ref:

Calibration Mapping File
-------------------------
The calibration mapping file holds the mapping between
run numbers, current label, offset filename and the empty
and vanadium numbers.

For more details on the calibration mapping file see:
:ref:`cycle_mapping_files_isis-powder-diffraction-ref`

The layout on GEM should look as follows for each block
substituting the below values for appropriate values:

.. code-block:: yaml
  :linenos:

  1-100:
    label: "1_1"
    offset_file_name: "offset_file.cal"
    PDF:
      vanadium_run_numbers: "10"
      empty_run_numbers: "20"
    Rietveld:
      vanadium_run_numbers: "30"
      empty_run_numbers: "40"

Lines 5 and 6 in this example set the vanadium and empty run numbers for
PDF mode. Lines 8 and 9 set the vanadium and empty for
Rietveld mode.

Example
^^^^^^^^
.. code-block:: yaml

  1-100:
    label: "1_1"
    offset_file_name: "offset_file.cal"
    PDF:
      vanadium_run_numbers: "10"
      empty_run_numbers: "20"
    Rietveld:
      vanadium_run_numbers: "30"
      empty_run_numbers: "40"

  101-:
    label: "1_2"
    offset_file_name: "offset_file.cal"
    PDF:
      vanadium_run_numbers: "110"
      empty_run_numbers: "120"
    Rietveld:
      vanadium_run_numbers: "130"
      empty_run_numbers: "140"

Parameters
-----------
The following parameters for GEM are intended for regular use
when using the ISIS Powder scripts.

.. _calibration_directory_gem_isis-powder-diffraction-ref:

calibration_directory
^^^^^^^^^^^^^^^^^^^^^
This parameter should be the full path to the calibration folder.
Within the folder the following should be present:

- Grouping .cal file (see: :ref:`grouping_file_name_gem_isis-powder-diffraction-ref`)
- Folder(s) with the label name specified in mapping file (e.g. "1_1")
  - Inside each folder should be the offset file with name specified in mapping file

The script will also save out vanadium splines into the relevant
label folder which are subsequently loaded and used within the
:ref:`focus_gem_isis-powder-diffraction-ref` method.

Example Input:

..  code-block:: python

  gem_example = Gem(calibration_directory=r"C:\path\to\calibration_dir", ...)

.. _calibration_mapping_file_gem_isis-powder-diffraction-ref:

calibration_mapping_file
^^^^^^^^^^^^^^^^^^^^^^^^
This parameter gives the full path to the YAML file containing the
calibration mapping. For more details on this file see:
:ref:`calibration_mapping_gem-isis-powder-ref`

*Note: This should be the full path to the file including extension*

Example Input:

..  code-block:: python

  # Notice the filename always ends in .yaml
  gem_example = Gem(calibration_mapping_file=r"C:\path\to\file\calibration_mapping.yaml", ...)

.. _config_file_gem_isis-powder-diffraction-ref:

config_file
^^^^^^^^^^^^
The full path to the YAML configuration file. This file is
described in detail here: :ref:`configuration_files_isis-powder-diffraction-ref`
It is recommended to set this parameter at object creation instead
of on a method as it will warn if any parameters are overridden
in the scripting window.

*Note: This should be the full path to the file including extension*

Example Input:

..  code-block:: python

  # Notice the filename always ends in .yaml
  gem_example = Gem(config_file=r"C:\path\to\file\configuration.yaml", ...)

.. _do_absorb_corrections_gem_isis-powder-diffraction-ref:

do_absorb_corrections
^^^^^^^^^^^^^^^^^^^^^
Indicates whether to perform vanadium absorption corrections
in :ref:`create_vanadium_gem_isis-powder-diffraction-ref` mode.
In :ref:`focus_gem_isis-powder-diffraction-ref` mode
sample absorption corrections require the sample be
set first with the :ref:`set_sample_gem_isis-powder-diffraction-ref`
method.

Accepted values are: **True** or **False**

*Note: If this is set to 'True'*
:ref:`multiple_scattering_gem_isis-powder-diffraction-ref`
*must be set*


Example Input:

..  code-block:: python

  gem_example.create_vanadium(do_absorb_corrections=True, ...)

  # Or (this assumes sample details have already been set)
  gem_example.focus(do_absorb_corrections=True, ...)


.. _file_ext_gem_isis-powder-diffraction-ref:

file_ext
^^^^^^^^^
*Optional*

Specifies a file extension to use when using the
:ref:`focus_gem_isis-powder-diffraction-ref` method.

This should be used to process partial runs. When
processing full runs (i.e. completed runs) it should not
be specified as Mantid will automatically determine the
best extension to use.

*Note: A leading dot (.) is not required but
is preferred for readability*

Example Input:

..  code-block:: python

  gem_example.focus(file_ext=".s01", ...)

.. _first_cycle_run_no_gem_isis-powder-diffraction-ref:

first_cycle_run_no
^^^^^^^^^^^^^^^^^^^
Indicates a run from the current cycle to use when calling
:ref:`create_vanadium_gem_isis-powder-diffraction-ref`.
This does not have the be the first run of the cycle or
the run number corresponding to the vanadium. However it
must be in the correct cycle according to the
:ref:`calibration_mapping_gem-isis-powder-ref`.

Example Input:

..  code-block:: python

  # In this example assume we mean a cycle with run numbers 100-200
  gem_example.create_vanadium(first_cycle_run_no=100, ...)

.. _input_mode_gem_isis-powder-diffraction-ref:

input_mode
^^^^^^^^^^
Indicates how to interpret the parameter
:ref:`run_number_gem_isis-powder-diffraction-ref` whilst
calling the :ref:`focus_gem_isis-powder-diffraction-ref`
method.
If the input_mode is set to *Summed* it will process
to sum all runs specified. If set to *Individual* it
will process all runs individually (i.e. One at a time)

Accepted values are: **Summed** and **Individual**

*Note: This parameter is not case sensitive*

Example Input:

..  code-block:: python

  gem_example.focus(input_mode="Summed", ...)

.. _mode_gem_isis-powder-diffraction-ref:

mode
^^^^
The current chopper mode to use in the
:ref:`create_vanadium_gem_isis-powder-diffraction-ref`
and :ref:`focus_gem_isis-powder-diffraction-ref` method.
This determines which vanadium and empty run numbers
to use whilst processing.

Accepted values are: **PDF** and **Rietveld**

*Note: This parameter is not case sensitive*

Example Input:

..  code-block:: python

  gem_example.create_vanadium(mode="PDF", ...)
  # Or
  gem_example.focus(mode="Rietveld", ...)

.. _multiple_scattering_gem_isis-powder-diffraction-ref:

multiple_scattering
^^^^^^^^^^^^^^^^^^^^
Indicates whether to account for the effects of multiple scattering
when calculating absorption corrections. If
:ref:`do_absorb_corrections_gem_isis-powder-diffraction-ref` is
set to **True** this parameter must be set.

Accepted values are: **True** or **False**

*Note: Calculating multiple scattering effects will add around
10-30 minutes to the script runtime depending on the speed of
the computer you are using*

Example Input:

..  code-block:: python

  gem_example.create_vanadium(multiple_scattering=True, ...)
  # Or
  gem_example.focus(multiple_scattering=False, ...)

.. _output_directory_gem_isis-powder-diffraction-ref:

output_directory
^^^^^^^^^^^^^^^^
Specifies the path to the output directory to save resulting files
into. The script will automatically create a folder
with the label determined from the
:ref:`calibration_mapping_file_gem_isis-powder-diffraction-ref`
and within that create another folder for the current
:ref:`user_name_gem_isis-powder-diffraction-ref`.

Within this folder processed data will be saved out in
several formats.

Example Input:

..  code-block:: python

  gem_example = Gem(output_directory=r"C:\path\to\output_dir", ...)

.. _run_number_gem_isis-powder-diffraction-ref:

run_number
^^^^^^^^^^
Specifies the run number(s) to process when calling the
:ref:`focus_gem_isis-powder-diffraction-ref` method.

This parameter accepts a single value or a range
of values with the following syntax:

**-** : Indicates a range of runs inclusive
(e.g. *1-10* would process 1, 2, 3....8, 9, 10)

**,** : Indicates a gap between runs
(e.g. *1, 3, 5, 7* would process run numbers 1, 3, 5, 7)

These can be combined like so:
*1-3, 5, 8-10* would process run numbers 1, 2, 3, 5, 8, 9, 10.

In addition the :ref:`input_mode_gem_isis-powder-diffraction-ref`
parameter determines what effect a range of inputs has
on the data to be processed

Example Input:

..  code-block:: python

  # Process run number 1, 3, 5, 6, 7
  gem_example.focus(run_number="1, 3, 5-7", ...)
  # Or just a single run
  gem_example.focus(run_number=100, ...)

.. _sample_empty_gem_isis-powder-diffraction-ref:

sample_empty
^^^^^^^^^^^^^
*Optional*

This parameter specifies a/several sample empty run(s)
to subtract from the run in the
:ref:`focus_gem_isis-powder-diffraction-ref` method.
If multiple runs are specified it will sum these runs
before subtracting the result.

This input uses the same syntax as
:ref:`run_number_gem_isis-powder-diffraction-ref`.
Please visit the above page for more details.

Example Input:

..  code-block:: python

  # Our sample empty is a single number
  gem_example.focus(sample_empty=100, ...)
  # Or a range of numbers
  gem_example.focus(sample_empty="100-110", ...)

.. _sample_empty_scale_gem_isis-powder-diffraction-ref:

sample_empty_scale
^^^^^^^^^^^^^^^^^^

Required if :ref:`sample_empty_gem_isis-powder-diffraction-ref`
is set to **True**

Sets a factor to scale the sample empty run(s) to before
subtracting. This value is multiplied after summing the
sample empty runs and before subtracting the empty from
the data set. For more details see: :ref:`Scale <algm-Scale-v1>`.

Example Input:

..  code-block:: python

  # Scale sample empty to 90% of original
  gem_example.focus(sample_empty_scale=0.9, ...)

.. _texture_mode_isis-powder-diffraction-ref:

texture_mode
^^^^^^^^^^^^
If set to **True**, then this specifies that the reduction is to be
carried out using Gem's 160-bank texture mode, as opposed to the
standard 6 banks. This means using altered cropping values for the
vanadium and sample workspaces, and using Men Xie's grouping file
(which must be placed in the top level of your
:ref:`calibration_directory_gem_isis-powder-diffraction-ref`).

Example Input:

.. code-block:: python

   gem_example.focus(texture_mode=True, ...)

.. _unit_to_keep_gem_isis-powder-diffraction-ref:

unit_to_keep
^^^^^^^^^^^^^
*Optional*

Specifies a single unit to keep in Mantid after processing using
the :ref:`focus_gem_isis-powder-diffraction-ref` method.
For example if *dSpacing* is set after processing only banks
in d-spacing will be present.

Accepted values are: **dSpacing** and **TOF**

*Note: All units will still be saved out in the*
:ref:`output_directory_gem_isis-powder-diffraction-ref`
*regardless of this property*

*Note: This parameter is not case sensitive*

Example Input:

..  code-block:: python

  gem_example.focus(unit_to_keep="dSpacing", ...)

.. _suffix_gem_isis-powder-diffraction-ref:

suffix
^^^^^^
*Optional*

This parameter specifies a suffix to append the names of output files
during a focus.

Example Input:

.. code-block:: python

  gem_example.focus(suffix="-corr", ...)

.. _user_name_gem_isis-powder-diffraction-ref:

user_name
^^^^^^^^^
Specifies the name of the current user when creating a
new GEM object. This is only used when saving data to
sort data into respective user folders.
See :ref:`output_directory_gem_isis-powder-diffraction-ref`
for more details.

Example Input:

..  code-block:: python

  gem_example = Gem(user_name="Mantid", ...)

.. _vanadium_normalisation_gem_isis-powder-diffraction-ref:

vanadium_normalisation
^^^^^^^^^^^^^^^^^^^^^^
Indicates whether to divide the focused workspace within
:ref:`focus_gem_isis-powder-diffraction-ref` mode with a
previously generated vanadium spline.

This requires a vanadium to have been previously created
with the :ref:`create_vanadium_gem_isis-powder-diffraction-ref`
method

Accepted values are: **True** or **False**

Example Input:

..  code-block:: python

  gem_example.focus(vanadium_normalisation=True, ...)

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

.. _focused_cropping_values_gem_isis-powder-diffraction-ref:

focused_cropping_values
^^^^^^^^^^^^^^^^^^^^^^^
Indicates a list of TOF values to crop the focused workspace
which was created by :ref:`focus_gem_isis-powder-diffraction-ref`
on a bank by bank basis.

This parameter is a list of bank cropping values with
one list entry per bank. The values **must** have a smaller
TOF window than the :ref:`vanadium_cropping_values_gem_isis-powder-diffraction-ref`

On GEM this is set to the following TOF windows:

..  code-block:: python

  # texture_mode = False (or not supplied)
  focused_cropping_values = [(550, 19900),  # Bank 1
                             (550, 19900),  # Bank 2
                             (550, 19900),  # Bank 3
                             (550, 19900),  # Bank 4
                             (550, 19480),  # Bank 5
                             (550, 17980)   # Bank 6
                             ]

  # texture_mode = True
  focused_cropping_values = [(448, 29344),  # Bank 1
                             (390, 19225),  # Bank 2
			     (390, 18673),  # Bank 3
			         ...        # Too many to list here - see gem_advanced_config.py
			     (600, 16828),  # Bank 158
			     (600, 16822),  # Bank 159
			     (600, 16827)   # Bank 160
			     ]

.. _grouping_file_name_gem_isis-powder-diffraction-ref:

grouping_file_name
^^^^^^^^^^^^^^^^^^
Determines the name of the grouping cal file which is located
within top level of the :ref:`calibration_directory_gem_isis-powder-diffraction-ref`.

The grouping file determines the detector ID to bank mapping to use
whilst focusing the spectra into banks.

On GEM this is set to the following:

..  code-block:: python

  # texture_mode = False (or not supplied)
  grouping_file_name: "GEM_Instrument_grouping.cal"

  # texture_mode = True
  grouping_file_name: "offsets_xie_test_2.cal"

.. _gsas_calib_filename_gem_isis-powder-diffraction-ref:

gsas_calib_filename
^^^^^^^^^^^^^^^^^^^
The name of the GSAS calibration file used to generate MAUD input
files when running a focus in :ref:`texture_mode_isis-powder-diffraction-ref`.

on GEM this is set to the following (this file is distributed with Mantid):

.. code-block:: python

  gsas_calib_filename: "GEM_PF1_PROFILE.IPF"

.. _maud_grouping_scheme_gem_isis-powder-diffraction-ref:

maud_grouping_scheme
^^^^^^^^^^^^^^^^^^^^
When saving MAUD files (typically only done when running in
:ref:`texture_mode_isis-powder-diffraction-ref`), there are too many banks to have
calibration parameters for each bank. Instead, the normal 6-bank calibration file is used
(see :ref:`gsas_calib_filename_gem_isis-powder-diffraction-ref`), and each of the 160
texture banks is assigned the calibration parameters of one of the 6 banks in the file.

This parameter associates each of the 160 banks to one of the big banks. It is a list of bank IDs,
where the value at element ``i`` is a number between 1 and 6, indicating which of the 6 banks to
associate texture bank ``i`` with.

On GEM this is set to the following:

.. code-block:: python

  maud_grouping_scheme: [1] * 3 + [2] * 8 + [3] * 20 + [4] * 42 + [5] * 52 + [6] * 35

.. _raw_tof_cropping_values_gem_isis-powder-diffraction-ref:

raw_tof_cropping_values
^^^^^^^^^^^^^^^^^^^^^^^
Determines the TOF window to crop all spectra down to before any
processing in the :ref:`create_vanadium_gem_isis-powder-diffraction-ref`
and :ref:`focus_gem_isis-powder-diffraction-ref` methods.

This helps remove negative counts where at very low TOF
the empty counts can exceed the captured neutron counts
of the run to process.

On GEM this is set to the following:

..  code-block:: python

  raw_tof_cropping_values: (500, 20000)

.. _save_angles_gem_isis-powder-diffraction-ref:

save_angles
^^^^^^^^^^^

If set to **True**, this saves the scattering angles (theta and eta)
of each focused bank to the 4-column MAUD format (the old
``grouping.new`` format) using :ref:`SaveBankScatteringAngles
<algm-SaveBankScatteringAngles>`.

If:ref:`texture_mode_isis-powder-diffraction-ref` is set to **True**
this is enabled, and disabled if it is set to **False**.

.. _save_maud_calib_gem_isis-powder-diffraction-ref:

save_maud_calib
^^^^^^^^^^^^^^^

If set to **True**, this uses the focus output and
:ref:`gsas_calib_filename_gem_isis-powder-diffraction-ref`
to create a MAUD calibration file, using
:ref:`SaveGEMMAUDParamFile <algm-SaveGEMMAUDParamFile>`.

If:ref:`texture_mode_isis-powder-diffraction-ref` is set to **True**
this is enabled, and disabled if it is set to **False**.

.. _save_maud_gem_isis-powder-diffraction-ref:

save_maud
^^^^^^^^^

If set to **True**, this saves the focus output to the Maud
three-column format (``.gem`` file extension). If
:ref:`texture_mode_isis-powder-diffraction-ref` is set to **True**
this is enabled, and disabled if it is set to **False**.

.. _spline_coefficient_gem_isis-powder-diffraction-ref:

spline_coefficient
^^^^^^^^^^^^^^^^^^^
Determines the spline coefficient to use after processing
the vanadium in :ref:`create_vanadium_gem_isis-powder-diffraction-ref`
method. For more details see: :ref:`SplineBackground <algm-SplineBackground>`

*Note that if this value is changed 'create_vanadium'
will need to be called again.*

On GEM this is set to the following:

..  code-block:: python

  spline_coefficient: 30

.. _vanadium_cropping_values_gem_isis-powder-diffraction-ref:

vanadium_cropping_values
^^^^^^^^^^^^^^^^^^^^^^^^
Determines the TOF windows to crop to on a bank by bank basis
within the :ref:`create_vanadium_gem_isis-powder-diffraction-ref`
method. This is applied after focusing and before a spline is taken.

It is used to remove low counts at the start and end of the vanadium run
to produce a spline which better matches the data.

This parameter is a list of bank cropping values with
one list entry per bank. The values **must** have a larger
TOF window than the :ref:`focused_cropping_values_gem_isis-powder-diffraction-ref`
and a smaller window than :ref:`raw_tof_cropping_values_gem_isis-powder-diffraction-ref`.

On GEM this is set to the following:

..  code-block:: python

  # texture_mode = False (or not supplied)
  vanadium_cropping_values = [(510, 19997),  # Bank 1
                              (510, 19997),  # Bank 2
                              (510, 19997),  # Bank 3
                              (510, 19997),  # Bank 4
                              (510, 19500),  # Bank 5
                              (510, 18000)   # Bank 6
                              ]

  # texture_mode = True
  vanadium_cropping_values = [(75, 34933),   # Bank 1
                              (65, 22887),   # Bank 2
			      (65, 22230),   # Bank 3
			          ...        # Too many banks to list here -see gem_advanced_config.py
			      (100, 20034),  # Bank 158
			      (100, 20026),  # Bank 159
			      (100, 20033)   # Bank 160
			      ]

.. _vanadium_sample_details_gem_isis-powder-diffraction-ref:

Vanadium sample details
^^^^^^^^^^^^^^^^^^^^^^^

.. _chemical_formula_sample_details_gem_isis-powder-diffraction-ref:

chemical_formula
================

The chemical formula for the Vanadium rod.
This is a rod consisting of 94.86% Vanadium and 5.14% Niobium.
Because this is not an elemental formula,
:ref:`number_density_sample_details_gem_isis-powder-diffraction-ref`
must also be set.

On GEM this is set to the following:

.. code-block:: python

  chemical_formula = "V0.9486 Nb0.0514"

.. _number_density_sample_details_gem_isis-powder-diffraction-ref:

number_density
==============

The number density corresponding to the
:ref:`chemical_formula_sample_details_gem_isis-powder-diffraction-ref`
used. This is in units of atoms/Angstrom^3.

On GEM this is set to the following:

.. code-block:: python

  number_density = 0.071

cylinder_sample_height
======================

The height of the Vanadium rod.

On GEM this is set to the following:

.. code-block:: python

  cylinder_sample_height = 4.0

cylinder_sample_radius
======================

The radius of the Vanadium rod.

On GEM this is set to the following:

.. code-block:: python

  cylinder_sample_radius = 0.4

cylinder_position
=================

The position of the Vanadium rod in [x, y, z]

On GEM this is set to the following:

.. code-block:: python

  cylinder_position = [0.0, 0.0, 0.0]

.. categories:: Techniques
