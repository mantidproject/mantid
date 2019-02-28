.. _isis-powder-diffraction-hrpd-ref:

==================================================
ISIS Powder Diffraction Scripts - HRPD Reference
==================================================

.. contents:: Table of Contents
    :local:

.. _creating_hrpd_object-isis-powder-diffraction-ref:

Creating an HRPD Object
-----------------------

This method assumes you are familiar with the concept of objects in Python.
If not more details can be read here: :ref:`intro_to_objects-isis-powder-diffraction-ref`

To create an HRPD object the following parameters are required:

- :ref:`calibration_directory_hrpd_isis-powder-diffraction-ref`
- :ref:`output_directory_hrpd_isis-powder-diffraction-ref`
- :ref:`user_name_hrpd_isis-powder-diffraction-ref`

Optionally a configuration file may be specified if one exists using
the following parameter:

- :ref:`config_file_hrpd_isis-powder-diffraction-ref`

See :ref:`configuration_files_isis-powder-diffraction-ref` for more
details on YAML configuration files.

Example
^^^^^^^

.. code-block:: python

   from isis_powder import HRPD

   calibration_dir = r"C:\path\to\calibration_dir"
   output_dir = r"C:\path\to\output_dir"

   hrpd_example = hrpd.HRPD(calibration_directory=calibration_dir,
                            output_directory=output_dir,
			    user_name="user's name")

   # Optionally we could provide a configuration file like so
   config_file_path = r"C:\path\to\config_file.yaml"
   hrpd_example = hrpd.HRPD(config_file=config_file_path,
                            user_name="user's name", ...)

Methods
-------
The following methods can be executed on an HRPD object:

- :ref:`create_vanadium_hrpd_isis-powder-diffraction-ref`
- :ref:`focus_hrpd_isis-powder-diffraction-ref`
- :ref:`set_sample_hrpd_isis-powder-diffraction-ref`
  
.. _create_vanadium_hrpd_isis-powder-diffraction-ref:

create_vanadium
^^^^^^^^^^^^^^^

The *create_vanadium* method allows a user to process a vanadium
run. Whilst processing the vanadium run, the scripts can apply any
corrections the user enables and will spline the resulting
workspace(s) for later focusing.

On HRPD the following parameters are required when executing
*create_vanadium*:

- :ref:`calibration_mapping_file_hrpd_isis-powder-diffraction-ref`
- :ref:`do_absorb_corrections_hrpd_isis-powder-diffraction-ref`
- :ref:`first_cycle_run_no_hrpd_isis-powder-diffraction-ref`
- :ref:`window_hrpd_isis-powder-diffraction-ref`
  
If :ref:`do_absorb_corrections_hrpd_isis-powder-diffraction-ref` is set to
**True** the following parameter is required in addition to the above:

- :ref:`multiple_scattering_hrpd_isis-powder-diffraction-ref`

The following parameter may optionally be passed:

- :ref:`mode_hrpd_isis-powder-diffraction-ref`
- :ref:`do_solid_angle_corrections_hrpd_isis-powder-diffraction-ref`

Example
=======

.. code-block:: python

  cal_mapping_file = r"C:\path\to\cal_mapping.yaml"

  hrpd_example.create_vanadium(calibration_mapping_file=cal_mapping_file,
                               first_cycle_run_no=66058, window="10-110",
			       do_absorb_correction=True,
			       multiple_scattering=False)
  
.. _focus_hrpd_isis-powder-diffraction-ref:

focus
^^^^^

The *focus* method processes the user-specified run(s). It aligns,
focuses and optionally applies corrections if the user has requested
them.

On HRPD the following parameters are required when executing *focus*

- :ref:`calibration_mapping_file_hrpd_isis-powder-diffraction-ref`
- :ref:`do_absorb_corrections_hrpd_isis-powder-diffraction-ref`
- :ref:`run_number_hrpd_isis-powder-diffraction-ref`
- :ref:`vanadium_normalisation_hrpd_isis-powder-diffraction-ref`
- :ref:`window_hrpd_isis-powder-diffraction-ref`

The following parameters may optionally be passed:

- :ref:`mode_hrpd_isis-powder-diffraction-ref`
- :ref:`sample_empty_hrpd_isis-powder-diffraction-ref`
- :ref:`suffix_hrpd_isis-powder-diffraction-ref`
- :ref:`subtract_empty_instrument_hrpd_isis-powder-diffraction-ref`
- :ref:`do_solid_angle_corrections_hrpd_isis-powder-diffraction-ref`
If :ref:`do_absorb_corrections_hrpd_isis-powder-diffraction-ref` is set to
**True** the following parameter is required in addition to the above:

- :ref:`multiple_scattering_hrpd_isis-powder-diffraction-ref`

If :ref:`sample_empty_hrpd_isis-powder-diffraction-ref` is set then
the following parameter is required in addition to the above:

- :ref:`sample_empty_scale_hrpd_isis-powder-diffraction-ref`

Example
=======

.. code-block:: python

  cal_mapping_file = r"C:\path\to\cal_mapping.yaml"

  hrpd_example.focus(run_number=66845, calibration_mapping_file=cal_mapping_file,
                     vanadium_normalisation=True, do_absorb_corrections=True,
		     sample_empty=66829, sample_empty_scale=1,
		     multiple_scattering=False, window="10-110")
  
.. _set_sample_hrpd_isis-powder-diffraction-ref:

set_sample
^^^^^^^^^^
The *set_sample* method allows a user to specify a SampleDetails
object which contains the sample properties used when
:ref:`do_absorb_corrections_hrpd_isis-powder-diffraction-ref` is
**True** in :ref:`focus_hrpd_isis-powder-diffraction-ref`.

For more details on the SampleDetails object and how to set it see:
:ref:`isis-powder-diffraction-sampleDetails-ref`.

The following parameter is required when calling *set_sample*.

- *sample* - This must be a SampleDetails object with the material set
  already.

Example
=======

..  code-block:: python

  sample_obj = SampleDetails(...)
  sample_obj.set_material(...)

  hrpd_example.set_sample(sample=sample_obj)


.. _calibration_mapping_hrpd_isis-powder-diffraction-ref: 
  
Calibration Mapping File
------------------------
The calibration mapping file holds the mapping between run numbers,
current label, offset filename and the empty and vanadium numbers.

For more details on the calibration mapping file see:
:ref:`cycle_mapping_files_isis-powder-diffraction-ref`

The layout on HRPD should look as follows for each block, substituting
the example values for appropriate ones.

.. code-block:: yaml
  :linenos:

  1-100:
    "coupled":
      "10-110":
        vanadium_run_numbers: "1"
	empty_run_numbers: "2"
    "decoupled":
      "100-200":
        vanadium_run_numbers: "3"
	empty_run_numbers: "4"
    label: "1_1"
    offset_file_name "offset_file.cal"

Lines 4 and 5 in this example set the vanadium and empty run numbers
for a time-of-flight window of 10-110 in a coupled run. Lines 7 and 8
set the vanadium & empty for tof window of 100-200 on a decoupled run.

Parameters
----------
The following parameters for HRPD are intended for regular use when
using the ISIS Powder scripts.

.. _calibration_directory_hrpd_isis-powder-diffraction-ref:

calibration_directory
^^^^^^^^^^^^^^^^^^^^^
This parameter should be the full path to the calibration folder.
Within the folder the following should be present:

- Grouping .cal file (see:
  :ref:`grouping_file_name_hrpd_isis-powder-diffraction-ref`)
- Folder(s) with the label name specified in mapping file (e.g. "1_1")
- Inside each folder should be the offset file with name specified in
  mapping file

The script will also save out vanadium splines into the relevant label
folder which are subsequently loaded and used within the
:ref:`focus_hrpd_isis-powder-diffraction-ref` method.

Example Input:

.. code-block:: python

  hrpd_example = HRPD(calibration_directory=r"C:\path\to\calibration_dir", ...)

.. _calibration_mapping_file_hrpd_isis-powder-diffraction-ref:

calibration_mapping_file
^^^^^^^^^^^^^^^^^^^^^^^^
This parameter gives the full path to the YAML file containing the
calibration mapping. For more details on this file see:
:ref:`calibration_mapping_hrpd_isis-powder-diffraction-ref`

*Note: this should be the full path to the file including extension*

Example Input:

..  code-block:: python

  hrpd_example =
  HRPD(calibration_mapping_file=r"C:\path\to\file\calibration_mapping.yaml", ...)

.. _config_file_hrpd_isis-powder-diffraction-ref:

config_file
^^^^^^^^^^^
The full path to the YAML configuration file. This file is described
in detail here:
:ref:`configuration_files_isis-powder-diffraction-ref`.  It is
recommended to set this parameter at object creation instead of when
executing a method as it will warn if any parameters are overridden in
the scripting window.

*Note: This should be the full path to the file including extension*

Example Input:

.. code-block:: python

  hrpd_example = HRPD(config_file=r"C:\path\to\file\configuration.yaml", ...)

.. _do_absorb_corrections_hrpd_isis-powder-diffraction-ref:

do_absorb_corrections
^^^^^^^^^^^^^^^^^^^^^
Indicates whether to perform absorption corrections in
:ref:`create_vanadium_hrpd_isis-powder-diffraction-ref` and
:ref:`focus_hrpd_isis-powder-diffraction-ref`. In
:ref:`focus_hrpd_isis-powder-diffraction-ref` the sample details must
be set first with :ref:`set_sample_hrpd_isis-powder-diffraction-ref`.

Accepted values are **True** or **False**.

*Note: If this is set to 'True'*
:ref:`multiple_scattering_hrpd_isis-powder-diffraction-ref` *must be
set*

Example Input:

..  code-block:: python

  hrpd_example.create_vanadium(do_absorb_corrections=True, ...)
  # Or (this assumes sample details have already been set)
  hrpd_example.focus(do_absorb_corrections=True, ...)
  
.. _first_cycle_run_no_hrpd_isis-powder-diffraction-ref:

first_cycle_run_no
^^^^^^^^^^^^^^^^^^
Indicates a run from the current cycle to use when calling
:ref:`create_vanadium_hrpd_isis-powder-diffraction-ref`. This does not
have to be the first run of the cycle or the run number corresponding
to the vanadium. However it must be in the correct cycle according to
:ref:`calibration_mapping_file_hrpd_isis-powder-diffraction-ref`.

Example Input:

.. code-block:: python

  # In this example assume we mean a cycle with run numbers 100-200
  hrpd_example.create_vanadium(first_cycle_run_no=100, ...)

.. _multiple_scattering_hrpd_isis-powder-diffraction-ref:

multiple_scattering
^^^^^^^^^^^^^^^^^^^
Indicates whether to account for the effects of multiple scattering
when calculating absorption corrections. if
:ref:`do_absorb_corrections_hrpd_isis-powder-diffraction-ref` is set
to **True** then this parameter must be set.

Accepted values are **True** or **False**.

*Note: Calculating multiple scattering effects will add a considerable
amount to the time it takes to run your script*

Example Input:

..  code-block:: python

  hrpd_example.create_vanadium(multiple_scattering=True, ...)
  # Or
  hrpd_example.focus(multiple_scattering=False, ...)

.. _output_directory_hrpd_isis-powder-diffraction-ref:

output_directory
^^^^^^^^^^^^^^^^
Specifies the path to the output directory to save processed files
into. The script will automatically create a folder with the label
determined from the
:ref:`calibration_mapping_file_hrpd_isis-powder-diffraction-ref` and
within that create another folder for the current
:ref:`user_name_polaris_isis-powder-diffraction-ref`. NXS and GSAS
files are saved here automatically.

Example Input:

.. code-block:: python

  hrpd_example = hrpd.HRPD(output_directory=r"C:\path\to\output_dir", ...)

.. _run_number_hrpd_isis-powder-diffraction-ref:

run_number
^^^^^^^^^^
Specifies the run number(s) to process when calling the
:ref:`focus_hrpd_isis-powder-diffraction-ref` method.

This parameter accepts a single value or a range 
of values with the following syntax:

**-** : Indicates a range of runs inclusive 
(e.g. *1-10* would process 1, 2, 3....8, 9, 10)

**,** : Indicates a gap between runs 
(e.g. *1, 3, 5, 7* would process run numbers 1, 3, 5, 7)

These can be combined like so:
*1-3, 5, 8-10* would process run numbers 1, 2, 3, 5, 8, 9, 10.

Example Input:

..  code-block:: python

  # Process run number 1, 3, 5, 6, 7
  hrpd_example.focus(run_number="1, 3, 5-7", ...)
  # Or just a single run
  hrpd_example.focus(run_number=100, ...)

.. _sample_empty_hrpd_isis-powder-diffraction-ref:

sample_empty
^^^^^^^^^^^^
*Optional*

This parameter specifies a/several sample empty run(s) to subtract
from the data when running
:ref:`focus_hrpd_isis-powder-diffraction-ref`. If multiple runs are
specified, they will be summed before being subtracted from the data.

This input uses the same syntax as
:ref:`run_number_hrpd_isis-powder-diffraction-ref`.

*Note: If this is set to anything other than* **False**,
*:ref:`sample_empty_scale_hrpd_isis-powder-diffraction-ref` must also
be specified*
     
Example Input:

..  code-block:: python

  # Our sample empty is a single number
  hrpd_example.focus(sample_empty=100, ...)
  # Or a range of numbers
  hrpd_example.focus(sample_empty="100-110", ...)

.. _sample_empty_scale_hrpd_isis-powder-diffraction-ref:

sample_empty_scale
^^^^^^^^^^^^^^^^^^
Required if :ref:`sample_empty_hrpd_isis-powder-diffraction-ref` is set to
anything other than **False**.

Sets a factor to scale the sample empty run(s) by before
subtracting. This value is multiplied after summing the empty runs and
before subtracting the empty from the data set. For more details see
:ref:`Scale <algm-Scale-v1>`.

Example Input:

..  code-block:: python

  # Scale sample empty to 90% of original
  hrpd_example.focus(sample_empty_scale=0.9, ...)

.. _suffix_hrpd_isis-powder-diffraction-ref:

suffix
^^^^^^
*Optional*

This parameter specifies a suffix to append the names of output files
during a focus.

Example Input:

.. code-block:: python

  hrpd_example.focus(suffix="-corr", ...)

.. _subtract_empty_instrument_hrpd_isis-powder-diffraction-ref:

subtract_empty_instrument
^^^^^^^^^^^^^^^^^^^^^^^^^
*Optional*

Indicates whether or not to subtract an empty run decided by the
- :ref:`calibration_mapping_file_hrpd_isis-powder-diffraction-ref`, defaults to false.

Example Input

..code-block:: python

  hrpd_example.focus(subtract_empty_instrument=True, ...)

.. _do_solid_angle_corrections_hrpd_isis-powder-diffraction-ref:

do_solid_angle_corrections
^^^^^^^^^^^^^^^^^^^^^^^^^^
*optional*

Indicates whether or not to run solid angle corrections,
on a vanadium this creaes the correction file as well as dividing by it,
on focus this attempts to load in the correction file and then divides by it.

Example Input

..code-block:: python

  hrpd_example.create_vanadium(do_solid_angle_corrections=True, ...)  
  hrpd_example.focus(do_solid_angle_corrections=True, ...)  

.. _user_name_hrpd_isis-powder-diffraction-ref:

user_name
^^^^^^^^^
Specifies the name of the current user when creating a new HRPD
object. This is only used when saving data to sort data into
respective user folders.
See :ref:`output_directory_hrpd_isis-powder-diffraction-ref` for more
details.

Example Input:

..  code-block:: python

  hrpd_example = HRPD(user_name="Mantid", ...)

.. _vanadium_normalisation_hrpd_isis-powder-diffraction-ref:

vanadium_normalisation
^^^^^^^^^^^^^^^^^^^^^^
Indicates whether to divide the focused workspace within
:ref:`focus_hrpd_isis-powder-diffraction-ref` method.

This requires a vanadium to have been previously created using
:ref:`create_vanadium_hrpd_isis-powder-diffraction-ref`.

Accepted value are **True** or **False**.

Example Input:

..  code-block:: python

  hrpd_example.focus(do_van_normalisation=True, ...)
  
.. _window_hrpd_isis-powder-diffraction-ref:

window
^^^^^^
The time-of-flight window to use in the
:ref:`create_vanadium_hrpd_isis-powder-diffraction-ref` and
:ref:`focus_hrpd_isis-powder-diffraction-ref` methods. This determines
which vanadium and empty run numbers to use while processing.

Accepted values are **10-110**, **30-130** or **100-200**.

Example Input:

.. code-block:: python

  hrpd_example.create_vanadium(window="100-200", ...)
  # Or
  hrpd_example.focus(window="10-110", ...)

Advanced Parameters
-------------------
.. warning:: These values are not intended to be changed and should
             reflect optimal defaults for the instrument. For more
             details please read:
             :ref:`instrument_advanced_properties_isis-powder-diffraction-ref`

             This section is mainly intended to act as reference for
             the current settings distributed with Mantid

Changing any values in the advanced configuration file will require
the user to restart Mantid in order for the new values to take effect.
Please read
:ref:`instrument_advanced_properties_isis-powder-diffraction-ref`
before changing values in the advanced configuration file.

focused_bin_widths
^^^^^^^^^^^^^^^^^^
The dt-upon-t binning for the focused data.

On HRPD this is set to the following:

.. code-block:: python

  focused_bin_widths = [
        -0.0003,  # Bank 1
        -0.0007,  # Bank 2
        -0.0012   # Bank 3
  ]
  
focused_cropping_values
^^^^^^^^^^^^^^^^^^^^^^^

Cropping windows for the three banks once data has been focused.

On HRPD this is set to the following:

.. code-block:: python

  # window = "10-50"
  focused_cropping_values = [
        (1.2e4, 4.99e4),  # Bank 1
        (1.2e4, 4.99e4),  # Bank 2
        (1.2e4, 4.99e4),  # Bank 3
  ]

  # window = "10-110"
  focused_cropping_values = [
        (1e4, 1.1e5),    # Bank 1
        (1e4, 1.2e5),    # Bank 2
        (1.1e4, 1.15e5)  # Bank 3
  ]
  
  # window = "30-130"
  focused_cropping_values = [
        (3e4, 1.3e5),      # Bank 1
        (2.84e4, 1.42e5),  # Bank 2
        (3e4, 1.37e5)      # Bank 3
  ]
  
  # window = "100-200"
  focused_cropping_values = [
        (1e5, 2.02e5),    # Bank 1
        (9.6e4, 2.18e5),  # Bank 2
        (1e5, 2.11e5)     # Bank 3
  ]

  # window = "180-280"
  focused_cropping_values = [
        (1.86e5, 2.8e5),   # Bank 1
        (1.8e5, 2.798e5),  # Bank 2
        (1.9e5, 2.795e5),  # Bank 3
  ]

.. _grouping_file_name_hrpd_isis-powder-diffraction-ref:
  
grouping_file_name
^^^^^^^^^^^^^^^^^^
The name of the grouping calibration file which is located within the
top level of the
:ref:`calibration_directory_hrpd_isis-powder-diffraction-ref`.

The grouping file determines the mapping from detector ID to bank, and
is used when focusing the spectra into banks.

On HRPD this is set to the following:

..  code-block:: python
		 
  grouping_file_name = "hrpd_new_072_01_corr.cal"

.. _mode_hrpd_isis-powder-diffraction-ref:

mode
^^^^
Indicates the coupling mode of the runs in
:ref:`create_vanadium_hrpd_isis-powder-diffraction-ref` and
:ref:`focus_hrpd_isis-powder-diffraction-ref`.

Accepted values are **coupled** and **decoupled**.

On HRPD this is set to the following:

.. code-block:: python

  mode = "coupled"

spline_coefficient
^^^^^^^^^^^^^^^^^^
The spline coefficient to use after processing the vanadium in
:ref:`create_vanadium_hrpd_isis-powder-diffraction-ref` method. For
more details see: :ref:`SplineBackground <algm-SplineBackground>`

*Note that if this value is changed 'create_vanadium' will need to be
called again.*

On HRPD this is set to the following:

..  code-block:: python

  spline_coefficient = 70

  
vanadium_tof_cropping
^^^^^^^^^^^^^^^^^^^^^

The cropping window for the Vanadium sample.

On HRPD this is set to the following:

.. code-block:: python

  # window = "10-50"
  vanadium_tof_cropping = (1.1e4, 5e4)

  # window = "10-110"
  vanadium_tof_cropping = (1e4, 1.2e5)

  # window = "30-130"
  vanadium_tof_cropping = (3e4, 1.4e5)

  # window = "100-200"
  vanadium_tof_cropping = (1e5, 2.15e5)

  # window = "180-280"
  vanadium_tof_cropping = (1.8e5, 2.8e5)

Vanadium Sample details
^^^^^^^^^^^^^^^^^^^^^^^

chemical_formula
================

The chemical formula for the Vanadium rod.

On HRPD this is, predictably, set to the following:

.. code-block:: python

  chemical_formula = "V"

cylinder_position
=================

The position of the Vanadium rod in [x, y, z]

On HRPD this is set to the following:

.. code-block:: python

  cylinder_position = [0.0, 0.0, 0.0]


cylinder_sample_height
======================

The height of the Vanadium rod.

On HRPD this is set to the following:

.. code-block:: python

  cylinder_sample_height = 2.0


cylinder_sample_radius
======================

The radius of the Vanadium rod.

On HRPD this is set to the following:

.. code-block:: python

  cylinder_sample_radius = 2.0

.. categories:: Techniques
