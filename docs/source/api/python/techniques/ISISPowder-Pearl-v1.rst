.. _isis-powder-diffraction-pearl-ref:

=======================================
ISIS Powder Diffraction Scripts - Pearl
=======================================

.. warning:: These scripts and documentation are still undergoing active development.
             They can change in any way during these stages and the validity of all
             data has not been tested.

.. contents:: Table of Contents
    :local:


.. _pearl_cal_folder_isis-powder-diffraction-ref:

Calibration Folder
-------------------
Within the top level of the calibration folder the following files must be present:

- .cal files containing grouping information (for all tt_modes)
- .nxs file with absorption corrections (if using absorption corrections)
- Folder for each cycle label (e.g. 10_2) containing a .cal file with detector offsets
  for that cycle

The names of the .cal grouping files and .nxs absorption file is set in the advanced
configuration file: :ref:`pearl_adv_script_params_isis-powder-diffraction-ref`

The label for the run the user is processing and the appropriate offset filename is
taken from the calibration mapping file: :ref:`pearl_cal_map_isis-powder-diffraction-ref`.

.. _pearl_cal_map_isis-powder-diffraction-ref:

Calibration Configuration File
------------------------------
An example layout is below:

File structure:

.. code-block:: yaml
  :linenos:

  123-130, 135-140:
    label : "10_1"
    vanadium_run_numbers : "123-125"
    empty_run_numbers : "135-137"
    calibration_file : "offsets_example_10_1.cal"

  141-145: ...etc.

Line 1 is documented here: :ref:`calibration_map_isis-powder-diffraction-ref`

Lines 2 - 5 can be placed in any order and specifies various properties common to these files.

- Line 2 specifically holds the label which is used in the calibration and output directories.
- Line 3 is the vanadium run numbers to use when creating a calibration for this label
- Line 4 holds the instrument empty run numbers
- Line 5 is the name of the offsets file which will be used whilst aligning detectors. See
  :ref:`pearl_cal_folder_isis-powder-diffraction-ref`

.. _pearl_focus_mode_isis-powder-diffraction-ref:

Focus Modes
-----------

All
^^^
In `all` mode banks 1-9 (inclusive) are summed into a single spectra then scaled
down by 1/9. The workspace is then attenuated if the correction is on. Workspaces
10-14 are left as separate workspaces with appropriate names.

Groups
^^^^^^
In `groups` mode banks 1+2+3, 4+5+6, 7+8+9 are summed into three (3) separate
workspaces then scaled down by 1/3. The workspaces containing banks 4-9 (inclusive)
are then added into a separate workspace and scaled down by 1/2. Banks 10-14
are left as separate workspaces with appropriate names.

Trans
^^^^^
In `trans` mode banks 1-9 (inclusive) are summed into a single spectra then scaled
down by 1/9. The workspace is then attenuated if the correction is on. The individual
banks 1-9 (inclusive) are also output as individual workspaces with appropriate names.

Mods
^^^^
In `mods` mode each bank is left as an individual workspace with an appropriate
name. No additional processing is performed.

Basic Script Parameters
-----------------------
For background on script parameters and how they are evaluated see:
:ref:`script_param_overview_isis-powder-diffraction-ref`

TODO talk about defaults?

- `attenuation_file_path` - The full path to the attenuation file. This is used with
  the algorithm :ref:`PearlMCAbsorption<algm-PearlMCAbsorption>`

- `calibration_directory` - This folder must contain various files such as
  detector offsets and detector grouping information. Additionally calibrated
  vanadium data will be stored here for later data processing.

- `config_file` - The full path to the YAML configuration file. This is described
  in more detail here: :ref:`yaml_basic_conf_isis-powder-diffraction-ref`

- `calbiration_config_path` - The full path to the calibration configuration file
  a description of the file is here: :ref:`pearl_cal_map_isis-powder-diffraction-ref`

- `do_absorb_corrections` - Used during a vanadium calibration and focusing:
  In a vanadium calibration if set to true the calibration will correct for
  absorption and scattering in a cylindrical sample.

  During focusing if set to true this will load a calibration which
  has had the absorption corrections performed, if false it will use a calibration
  where the absorption corrections have not been performed.

- `focus_mode` - More information found here: :ref:`pearl_focus_mode_isis-powder-diffraction-ref` .
  Acceptable options: `all`, `groups`, `trans` and `mods`.

- `long_mode` - Processes data in 20,000-40,000μs instead of the usual 0-20,000μs window.

- `output_directory` - This folder is where all processed data will be saved.

- `perform_attenuation` - If set to true uses the user specified attenuation file
  (see `attenuation_file_name`) and applies the correction.

- `run_in_cycle` - Only used during vanadium calibration. The run specified
  here is used with the calibration mapping file see:
  :ref:`pearl_cal_map_isis-powder-diffraction-ref` to determine the current cycle
  and the vanadium/empty run numbers for the subsequent processing.

- `run_number` - Used during focusing a single run or range of runs can be specified
  here. This range is inclusive e.g. 10-12 will be runs 10,11,12.
  These runs will be first summed together before any processing is performed
  on them if there are multiple runs specified.

- `tt_mode` - Specifies the detectors to be considered.
  Acceptable options: `tt35`, `tt70`, `tt88`, `all` (when creating vanadium).

- `user_name` - Used to create a folder with that name in the output directory

- `vanadium_normalisation` - If set to true divides the sample by the vanadium
  vanadium calibration during the focusing step.

.. _pearl_adv_script_params_isis-powder-diffraction-ref:

Advanced Script Parameters
--------------------------
- `monitor_lambda_crop_range` - The range in dSpacing to crop a monitor workspace
  to whilst calculating the current normalisation. This is should be stored as a tuple
  of both values. This is used with `long_mode` so there is a set of values for
  `long_mode` off and on.

- `monitor_integration_range` - The maximum and minimum contribution a bin can provide
  whilst integrating the monitor spectra. Any values that fall outside of this range
  are not added in. This should be stored as a tuple of both values. This is
  used with `long_mode` so there is a set of values for `long_mode` off and on.

- `monitor_spectrum_number` - The spectrum number of the current monitor.

- `monitor_spline_coefficient` - The number of b-spline coefficients to use whilst
  taking a background spline of the monitor.

- `raw_data_tof_cropping` - Stores the window in TOF which the data should be
  cropped down to before any processing. This is used with `long_mode` so there
  is a set of values for `long_mode` off and on. Each should be a tuple of the minimum
  and maximum time of flight. It should also be greater than `vanadium_tof_cropping`
  and `tof_cropping_values`

- `spline_coefficient` - The number of b-spline coefficients to use whilst taking
  a background spline of the focused vanadium data.

- `tof_cropping_values` - Stores per bank the TOF which the focused data should
  be cropped to. This does not affect the `vanadium_tof_cropping` which must be larger
  than the interval between the smallest and largest cropping values. This is
  stored as a list of tuple pairs with one tuple per bank. This is used with `long_mode`
  so there is a set of values for `long_mode` off and on.

- `tt_88_grouping` - The file name for the `.cal` file with grouping details for
  the instrument in `TT88` mode. This must be located in the top level directory
  of the calibration folder. More information can be found
  here: :ref:`pearl_cal_folder_isis-powder-diffraction-ref`

- `tt_70_grouping` - The file name for the `.cal` file with grouping details for
  the instrument in `TT70` mode. See `tt_88_grouping` for more details.

- `tt_35_grouping` - The file name for the `.cal` file with grouping details for
  the instrument in `TT35` mode. See `tt_88_grouping` for more details.

- `vanadium_absorb_file` - The file name for the vanadium absorption corrections.
  This must be located in the top level directory of the calibration folder.
  More information here: :ref:`pearl_cal_folder_isis-powder-diffraction-ref`

- `vanadium_tof_cropping` - The range in TOF to crop the calibrated vanadium
  file to after focusing. This must be less than `raw_data_tof_cropping` and
  larger than `tof_cropping_values`. The cropping is applied before a spline is
  taken of the vanadium sample.

.. _pearl_config_scripts_isis-powder-diffraction-ref:

Configuring the scripts
------------------------
The scripts are object oriented for more information on this concept see
:ref:`script_param_overview_isis-powder-diffraction-ref`

The following parameters must be included in the object construction step.
They can either be manually specified or set in the configuration file:

- calibration_directory
- output_directory
- user_name

First the relevant scripts must be imported with the instrument specific customisations
as follows:

.. code-block:: python

 # First import the relevant scripts for PEARL
 from isis_powder.pearl import Pearl

The scripts can be setup in 3 ways:

1. Explicitly setting parameters for example :- user_name, calibration_directory
and output_directory...etc.:

.. code-block:: python

 pearl_manually_specified = Pearl(user_name="Mantid",
                                  calibration_directory="<Path to calibration folder>",
                                  output_directory="<Path to output folder>", ...etc.)

2. Using user configuration files see :ref:`yaml_basic_conf_isis-powder-diffraction-ref`.
   This eliminates having to specify several common parameters

.. code-block:: python

 config_file_path = <path to your configuration file>
 pearl_object_config_file = Pearl(user_name="Mantid2", config_file=config_file_path)

3. Using a combination of both, any parameter can be overridden from the
configuration file without changing it:

.. code-block:: python

 # This will use "My custom location" instead of the location set in the configuration file
 pearl_object_override = Pearl(user_name="Mantid3", config_file=config_file_path,
                               output_directory="My custom location")

Each object remembers its own properties - changing properties on another
object will not affect others: In the above examples `pearl_object_override`
will save in *"My custom location"* whilst `pearl_manually_specified` will have user
name *"Mantid"* and save in *<Path to output folder>*.

Vanadium Calibration
---------------------
Following on from the examples configuring the scripts (see:
:ref:`pearl_config_scripts_isis-powder-diffraction-ref`) we can run a vanadium
calibration with the `create_vanadium` method.

TODO the following parameters are needed...

.. code-block:: python

 # Lets use the "pearl_object_override" which stores in "My custom location"
 # from the previous examples
 pearl_object_override.create_vanadium(run_in_range=12345,
                                       do_absorb_corrections=True
                                       long_mode=False, tt_mode=tt88)

This will generate a calibration for the specified vanadium and empty runs
specified in the calibration mapping file (see: :ref:`pearl_cal_map_isis-powder-diffraction-ref`)
and store it in the calibration folder - more details here: :ref:`pearl_cal_folder_isis-powder-diffraction-ref`

*Note: This only needs to be completed once per cycle as the splined vanadium workspace will be
automatically loaded and used for future focusing where that vanadium is used.
This means that it should not be part of your focusing scripts as it will recalculate the same
values every time it is ran.*

Focusing
----------
Using the examples from the configured scripts (see: :ref:`pearl_config_scripts_isis-powder-diffraction-ref`)
we can run focusing with the `focus` method:

TODO list the parameters which are mandatory

.. code-block:: python

  # Using pearl_object_config_file which was using a configuration file
  # We will focus runs 10000-10010 which sums up the runs inclusively
  pearl_object_config_file.focus(run_number="10000-10010")
