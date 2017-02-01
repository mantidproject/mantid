.. _isis-powder-diffraction-ref:

================================
ISIS Powder Diffraction Scripts
================================

.. warning:: These scripts and documentation are still undergoing active development.
             They can change in any way during these stages and the validity of all
             data has not been tested.

.. contents:: Table of Contents
    :local:


Overview and General Information
--------------------------------
These objective of these scripts are to combine the work-flows of several powder
diffraction instruments into a single collection of scripts whilst catering to
their individual requirements. At the most fundamental level it provides the
functionality to calculate and apply vanadium calibrations and subsequently
apply these corrections to experimental data.

Data Files Setup
^^^^^^^^^^^^^^^^^
Users must setup their input directories so Mantid can find the input files. Instructions
on completing this are located `here <http://www.mantidproject.org/ManageUserDirectories>`_.
*Note: Mantid will not search folders recursively each folder must be added*

Additionally *Search Data Archive* can be ticked if the device is located on the ISIS
network to automatically handle finding the files whilst it is on the network.

.. _script_param_overview_isis-powder-diffraction-ref:

Script Parameters
^^^^^^^^^^^^^^^^^
Script parameters can be set at any point (e.g. during configuration or
just before calling a method). If there was previously a value set a notice will
appear in the output window informing the user of the old and new values. The
script will use the value most recently set.

In alternative words  terms it means you ask for
an instrument object and give it a name. Any parameters
you set with that name stay with that name and do not affect other objects
with different names.

The parameters are read in order from the following locations:

- Advanced configuration file
- Basic configuration (if specified see :ref:`yaml_basic_conf_isis-powder-diffraction-ref`)
- Script parameters

For a list of valid keys (also known as parameters), their values and purpose
refer to the respective instrument documentation.

.. _yaml_basic_conf_isis-powder-diffraction-ref:

YAML Configuration Files
^^^^^^^^^^^^^^^^^^^^^^^^
The basic configuration files are written in YAML then can be passed as a parameter whilst configuring the scripts.
This avoids having to respecify paths or settings which rarely change such as the location of the calibration folder.
These use a simple format of key : value pair in YAML syntax. For example:

.. code-block:: yaml

  # Comments are indicated by a '#' at the start of the comment
  # By using ' ' characters around values they do not need to be escaped
  user_name : 'Mantid_Documentation'
  output_directory : '<Path\to\your\output\folder>'

Would set the key `user_name` to the value `Mantid_Documentation` when using the basic configuration file.
The value can be overridden at the command line and a warning will be emitted so the user is aware of the new
value the key is set to.

.. warning:: Within these files the most recent value also takes precedence.
             So if `user_name` appeared twice within a script the value closest
             to the bottom will be used. This is implementation specific and
             should not be relied on. Users should strive to ensure each key - value
             pair appears once to avoid confusion.

.. _calibration_map_isis-powder-diffraction-ref:

Calibration Configuration File
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The calibration mapping file allows users to specify ranges of runs and their
common properties to all of them. These include the vanadium and empty run numbers,
offset file and label for those runs. Each format is bespoke to the instrument's
requirements and is documented as part of the instrument documentation.

The first line in all examples holds the run numbers so is documented here.
The range of run numbers that this block (a block is the lines starting
with consistent number of spaces throughout) holds details for. In this case it specifies
runs 123-130 (inclusive) and runs 135-140 (inclusive) should use the following details.
Additionally a single range of runs can be unbounded such as `200-` which would match
runs >= 200. There is several sanity checks in place that ensure there is not multiple
unbounded entries and that all other runs specified are not within the unbounded range.

PEARL
-----

.. _pearl_cal_folder_isis-powder-diffraction-ref:

Calibration Folder
^^^^^^^^^^^^^^^^^^
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
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
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
^^^^^^^^^^^

All
~~~
In `all` mode banks 1-9 (inclusive) are summed into a single spectra then scaled
down by 1/9. The workspace is then attenuated if the correction is on. Workspaces
10-14 are left as separate workspaces with appropriate names.

Groups
~~~~~~
In `groups` mode banks 1+2+3, 4+5+6, 7+8+9 are summed into three (3) separate
workspaces then scaled down by 1/3. The workspaces containing banks 4-9 (inclusive)
are then added into a separate workspace and scaled down by 1/2. Banks 10-14
are left as separate workspaces with appropriate names.

Trans
~~~~~
In `trans` mode banks 1-9 (inclusive) are summed into a single spectra then scaled
down by 1/9. The workspace is then attenuated if the correction is on. The individual
banks 1-9 (inclusive) are also output as individual workspaces with appropriate names.

Mods
~~~~
In `mods` mode each bank is left as an individual workspace with an appropriate
name. No additional processing is performed.

Basic Script Parameters
^^^^^^^^^^^^^^^^^^^^^^^
For background on script parameters and how they are evaluated see:
:ref:`script_param_overview_isis-powder-diffraction-ref`

TODO talk about defaults?

- `attenuation_file_name` - The attenuation file name, this file must be located in
  the top level directory of the calibration directory. More information
  here: :ref:`pearl_cal_folder_isis-powder-diffraction-ref`

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

- `run_in_range` - Only used during vanadium calibration. The run specified
  here is used with the calibration mapping file see:
  :ref:`pearl_cal_map_isis-powder-diffraction-ref` to determine the current cycle
  and the vanadium/empty run numbers for the subsequent processing.

- `run_number` - Used during focusing a single run or range of runs can be specified
  here. This range is inclusive e.g. 10-12 will be runs 10,11,12.
  These runs will be first summed together before any processing is performed
  on them if there are multiple runs specified.

- `tt_mode` - Specifies the detectors to be considered.
  Acceptable options: `tt35`, `tt70`, `tt88`.

- `user_name` - Used to create a folder with that name in the output directory

- `vanadium_normalisation` - If set to true divides the sample by the vanadium
  vanadium calibration during the focusing step.

.. _pearl_adv_script_params_isis-powder-diffraction-ref:

Advanced Script Parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^
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
^^^^^^^^^^^^^^^^^^^^^^^^
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
^^^^^^^^^^^^^^^^^^^^^
Following on from the examples configuring the scripts (see:
:ref:`pearl_config_scripts_isis-powder-diffraction-ref`) we can run a vanadium
calibration with the `create_calibration_vanadium` method.

TODO the following parameters are needed...

.. code-block:: python

 # Lets use the "pearl_object_override" which stores in "My custom location"
 # from the previous examples
 pearl_object_override.create_calibration_vanadium(run_in_range=12345,
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
^^^^^^^^^^
Using the examples from the configured scripts (see: :ref:`pearl_config_scripts_isis-powder-diffraction-ref`)
we can run focusing with the `focus` method:

TODO list the parameters which are mandatory

.. code-block:: python

  # Using pearl_object_config_file which was using a configuration file
  # We will focus runs 10000-10010 which sums up the runs inclusively
  pearl_object_config_file.focus(run_number="10000-10010")


POLARIS
-------

.. _polaris_calibration_folder-powder-diffraction-ref:

Calibration Folder
^^^^^^^^^^^^^^^^^^^
Within the top level of the calibration folder for POLARIS the following files
must be present:

- .cal file containing the detector grouping information
- File containing masking data for Vanadium peaks
- Folder for each cycle label (e.g. 10_2) containing a .cal file with detector
  offsets for that cycle

The names of the .cal grouping file and masking file are set in the advanced
configuration file. See: TODO link

The label for the run being processed and the appropriate offset filename is
read from the calibration mapping file: TODO link

.. _polaris_calibration_map-powder-diffraction-ref:

Calibration Configuration File
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
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
^^^^^^^^^^^^^^^^^^^^^^^
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

  If set to true during focusing the vanadium with these corrections is loaded
  and used, if false it will load a vanadium sample where these corrections have
  not been applied.

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

Advanced Script Parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^

- `grouping_file_name` - The name of the .cal file containing grouping information
  for the detectors. This file must be located at the top of the calibration
  directory as noted here :ref:`polaris_calibration_folder-powder-diffraction-ref`

- `masking_file_name` - The name of the file containing Vanadium masking information.
  This file must be located at the top of the calibration directory as noted here:
  :ref:`polaris_calibration_folder-powder-diffraction-ref`

- `raw_data_cropping_values` - The window in TOF which the data should be cropped
  down to before any processing. This should be stored as a tuple of minimum and
  maximum TOF values. The window should be larger than `vanadium_cropping_values`.

- `spline_coefficient` - The coefficient to use whilst taking a b-spline of the
  Vanadium workspace during calibration

- `tof_cropping_ranges` - Stores the TOF window to crop down to on a bank-by-bank
  basis. This is one of the final steps applied to a focused workspace. The values
  are stored as a list of tuples, with one tuple per bank and each containing
  the minimum and maximum values in TOF. The window specified must be less than
  both `vanadium_cropping_values` and `raw_data_tof_cropping`

- `vanadium_cropping_values` - Stores the TOF window the vanadium workspace is
  cropped down to after focusing. This value is stored as a tuple of the minimum
  and maximum values. The TOF window should be smaller than `raw_data_cropping_values`
  but larger than `tof_cropping_ranges`

Configuring the Scripts
^^^^^^^^^^^^^^^^^^^^^^^
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
^^^^^^^^^^^^^^^^^^^^
Within the objects now configured we can run the vanadium calibrations. This
is done with the `create_calibration_vanadium` method.

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
  polaris_manually_specified.create_calibration_vanadium(run_in_range="123", ...)

Focusing
^^^^^^^^
Using the examples for the configured scripts we can now run the focusing method:

TODO required parameters

.. code-block:: python

  # We will use the object which has the output_directory overridden to
  # "My custom location"
  polaris_overriden.focus(run_number="140-150", input_mode="Individual"...)

