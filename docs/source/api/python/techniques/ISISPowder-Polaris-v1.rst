.. _isis-powder-diffraction-polaris-ref:

=========================================
ISIS Powder Diffraction Scripts - Polaris
=========================================

.. warning:: These scripts and documentation are still undergoing active development.
             They can change in any way during these stages and the validity of all
             data has not been tested.

.. contents:: Table of Contents
    :local:



.. _polaris_calibration_folder-powder-diffraction-ref:

Calibration Folder
------------------
Within the top level of the calibration folder for POLARIS the following files
must be present:

- .cal file containing the detector grouping information
- File containing masking data for Vanadium peaks
- Folder for each cycle label (e.g. 10_2) containing a .cal file with detector
  offsets for that cycle

The names of the .cal grouping file and masking file are set in the advanced
configuration file. See: :ref:`polaris_adv_config-powder-diffraction-ref`

The label for the run being processed and the appropriate offset filename is
read from the calibration mapping file: :ref:`polaris_calibration_map-powder-diffraction-ref`

.. _polaris_calibration_map-powder-diffraction-ref:

Calibration Configuration File
------------------------------
An example of the file layout is below:

.. code-block:: yaml
  :linenos:

  123-130, 135-140:
    label : "10_1"
    offset_file_name : "offsets_example_10_1.cal"

    chopper_on :
      vanadium_run_numbers : "123-125"
      empty_run_numbers : "126-130"

    chopper_off :
      vanadium_run_numbers : "135-137"
      empty_run_numbers : "138-140"

  141-145: ...etc.

Line 1 is documented here: :ref:`calibration_map_isis-powder-diffraction-ref`

The subsequent lines can be placed in any order provided that blocks (which are
marked by the indentation of the line) remain together. This is further explained
below.

- Line 2 sets the label that is associated with any runs specified in line 1
  and is used for the calibration and output directories
- Line 3 sets the name of the offset file to use. See TODO link (calibration folder)
- Lines 4 and 8 are whitespace - they are there to visually separate the blocks
  and will be ignored by the parser
- Line 5 indicates the next block (which is marked by the indentation) will
  be runs for when the chopper was on
- Line 6 the vanadium run numbers for when the chopper is on
- Line 7 the empty run numbers for when the chopper was on
- Line 8 - See line 4
- Line 9 indicates the next block, notice the indentation is back to the original
  level. This says the subsequent lines at deeper indentation are for chopper off
- Line 10 the vanadium run numbers for when the chopper is off
- Line 11 the empty run numbers for when the chopper is off

Basic Script Parameters
-----------------------
For background on script parameters and how they are evaluated see:
:ref:`script_param_overview_isis-powder-diffraction-ref`

- `calibration_directory` - The location of the calibration folder. The structure
  of the folder is described here: :ref:`polaris_calibration_folder-powder-diffraction-ref`
  Additionally calibrated vanadium data will be stored here for later
  use whilst focusing.

- `calibration_mapping_file` - The full path to the YAML mapping of run numbers,
  label and vanadium/empty runs. This is described in more detail here:
  :ref:`polaris_calibration_map-powder-diffraction-ref`

- `chopper_on` - This flag which can be set to True or False indicates whether the
  chopper was on for this set of runs. As noted (:ref:`script_param_overview_isis-powder-diffraction-ref`)
  the scripts will use the most recent value set on that object.

- `config_file` - The full path to the YAML configuration file. The full description
  of this file is here: :ref:`calibration_map_isis-powder-diffraction-ref`

- `do_absorb_corrections` - Used during a vanadium calibration and subsequent focusing
  if set to True the calibration routine will correct for absorption and scattering
  in a cylindrical sample as defined in the advanced configuration file. It then applies
  these calibrations to the vanadium sample.

- `do_van_normalisation` - If set to True divides the sample by the calculated vanadium
  spline during the normalisation step.

- `input_mode` - Specifies how the runs are processed. Accepted values `Individual`,
  `Summed` - TODO write section on input modes for overview

- `multiple_scattering` - If set to True with `do_absorb_corrections` the calculation
  will factor in the effects of multiple scattering and apply the correct corrections.

- `run_in_range` - Only used during vanadium calibration. The run specified here
  is used with to determine the current label and the correct runs to use whilst
  calculating the calibration. See :ref:`polaris_calibration_map-powder-diffraction-ref`

- `run_number` - used during focusing, a single run or range of runs can be specified here.
  All ranges specified are processed inclusively with the behavior determined by
  `input_mode`. See TODO link

- `output_directory` - The folder where the data is saved. The data is saved
  in a folder with the label appropriate for that/those run(s) and the user name
  specified by the user.

- `user_name` - Specifies the user name to use when saving out focused data.

.. _polaris_adv_config-powder-diffraction-ref:

Advanced Script Parameters
--------------------------

- `grouping_file_name` - The name of the .cal file containing grouping information
  for the detectors. This file must be located at the top of the calibration
  directory as noted here :ref:`polaris_calibration_folder-powder-diffraction-ref`

- `focused_cropping_values` - Stores the TOF window to crop down to on a bank-by-bank
  basis. This is one of the final steps applied to a focused workspace. The values
  are stored as a list of tuples, with one tuple per bank and each containing
  the minimum and maximum values in TOF. The window specified must be less than
  both `vanadium_cropping_values` and `raw_data_tof_cropping`

- `masking_file_name` - The name of the file containing Vanadium masking information.
  This file must be located at the top of the calibration directory as noted here:
  :ref:`polaris_calibration_folder-powder-diffraction-ref`

- `raw_data_cropping_values` - The window in TOF which the data should be cropped
  down to before any processing. This should be stored as a tuple of minimum and
  maximum TOF values. The window should be larger than `vanadium_cropping_values`.

- `spline_coefficient` - The coefficient to use whilst taking a b-spline of the
  Vanadium workspace during calibration

- `vanadium_cropping_values` - Stores the TOF window the vanadium workspace is
  cropped down to after focusing. This value is stored as a tuple of the minimum
  and maximum values. The TOF window should be smaller than `raw_data_cropping_values`
  but larger than `tof_cropping_ranges`

Configuring the Scripts
-----------------------
The scripts are objected oriented - for more information on this concept see
:ref:`script_param_overview_isis-powder-diffraction-ref`

The following parameters must be included in the object construction step.
They can either be manually specified or set in the configuration file:

- calibration_directory
- output_directory
- user_name

The first step is importing the correct scripts for the Polaris instrument:


.. code-block:: python

  # First import the scripts for Polaris
  from isis_powder.polaris import Polaris

The scripts can be setup in 3 ways:

1. Explicitly setting all parameters:

.. code-block:: python

  polaris_manually_specified = Polaris(user_name="Mantid",
                                       calibration_directory="<Path to Calibration folder>",
                                       output_directory="<Path to output folder>")

2. Using user configuration files see :ref:`yaml_basic_conf_isis-powder-diffraction-ref`.
   This eliminates having to specify common parameters:

.. code-block:: python

  config_file_path = "<path_to_your_config_file">
  polaris_using_config_file = Polaris(user_name="Mantid2", config_file=config_file_path)

3. Using a combination of both, a parameter set from the script will override the
   configuration parameter without changing the configuration file.

.. code-block:: python

  # This will use "My custom location" instead of the value set in the configuration file
  polaris_overriden = Polaris(user_name="Mantid3", config_file=config_file_path,
                              output_directory="My custom location")


Vanadium Calibration
--------------------
Within the objects now configured we can run the vanadium calibrations. This
is done with the `create_vanadium` method.

This will generate a calibration for the matching vanadium and empty runs in
the calibration mapping file (see :ref:`polaris_calibration_map-powder-diffraction-ref`)
and store it into the calibration folder under the appropriate label.

*Note: This only needs to be completed once per cycle for each set of options used.
The splined vanadium will automatically be loaded during focusing so the
vanadium calibration step should not be part of your focusing scripts.*

TODO the following parameters are needed.

.. code-block:: python

  # Using the manually specified object where we put in the calibration folder
  # location when configuring the scripts
  polaris_manually_specified.create_vanadium(run_in_range="123", ...)

Focusing
---------
Using the examples for the configured scripts we can now run the focusing method:

TODO required parameters

.. code-block:: python

  # We will use the object which has the output_directory overridden to
  # "My custom location"
  polaris_overriden.focus(run_number="140-150", input_mode="Individual"...)

