.. _isis-powder-diffraction-ref

================================
ISIS Powder Diffraction Scripts
================================

.. warning:: These scripts and documentation are still undergoing active development. 
             They can change in any way during these stages and the validity of all
             data has not been tested.
             
.. contents:: Table of Contents
    :local:

Overview
--------
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

.. _yaml_isis-powder-diffraction-ref:

YAML Configuration Files
^^^^^^^^^^^^^^^^^^^^^^^^



PEARL
-----

.. _pearl_cal_folder_isis-powder-diffraction-ref:

Calibration Folder
^^^^^^^^^^^^^^^^^^
TODO talk about structure of calibration folder/required folders and what is put in
...etc.

.. _pearl_cal_map_isis-powder-diffraction-ref:

Calibration Configuration File
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
TODO talk about the calibration mapping file

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


Script configuration parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The following parameters must be included in the object construction step. 
They be either manually specified or set in the configuration file:
 
 - `calibration_directory` - This folder must contain various files such as 
   detector offsets and detector grouping information. Additionally calibrated
   vanadium data will be stored here for later data processing. 
   
 - `user_name` - Used to create a folder with that name in the output directory

 - `output_directory` - This folder is where all processed data will be saved. 
 
Basic Script Parameters
^^^^^^^^^^^^^^^^^^^^^^^
The following parameters can be set at any point (e.g. during configuration or
just before calling a method). If there was previously a value set a notice will appear
in the output window informing the user of the old and new values. 

TODO talk about defaults?

 - `attenuation_file_name` - The attenuation file name, this file must be located in
   the top level directory of the calibration directory. More information 
   here: :ref:`pearl_cal_folder_isis-powder-diffraction-ref`
 
 - `config_file` - The full path to the YAML configuration file. This is described
   in more detail here: :ref:`yaml_isis-powder-diffraction-ref`

 - `calbiration_config_path` - The full path to the calibration configuration file 
   a description of the file is here: :ref:`pearl_cal_map_isis-powder-diffraction-ref`
   
 - `do_absorb_corrections` - Used during a vanadium calibration if set to true the 
   calibration will correct for absorption and scattering in a cylindrical sample
   
 - `focus_mode` - More information found here: :ref:`pearl_focus_mode_isis-powder-diffraction-ref` .
   Acceptable options: `all`, `groups`, `trans` and `mods`.
 
 - `long_mode` - Processes data in 20,000-40,000μs instead of the usual 0-20,000μs window.
 
 - `perform_attenuation` - If set to true uses the user specified attenuation file 
   (see `attenuation_file_name`) and applies the correction.
   
 - `tt_mode` - Specifies the detectors to be considered whilst focussing.
   Acceptable options: `tt35`, `tt70`, `tt88`.
 
 - `vanadium_normalisation` - If set to true divides the sample by the vanadium
   vanadium calibration during the focussing step.
   
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
   
 - `tof_cropping_values` - Stores per bank the TOF which the focussed data should
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
   file to after focussing. This must be less than `raw_data_tof_cropping` and
   larger than `tof_cropping_values`. The cropping is applied before a spline is
   taken of the vanadium sample. 
   
 
Configuring the scripts
^^^^^^^^^^^^^^^^^^^^^^^^
Code example with comments:
::

 # First import the relevant scripts for PEARL
 from isis_powder.pearl import Pearl  
 
The scripts can be setup in 3 ways:

1.  Explicitly setting parameters for example :- user_name, calibration_directory 
and output_directory...etc.:
::

 pearl_manually_specified = Pearl(user_name="Mantid", 
                                  calibration_directory="<Path to calibration folder>",
                                  output_directory="<Path to output folder>", ...etc.)

2. Using user configuration files. This eliminates having to specify several parameters
::
 
 config_file_path = <path to your configuration file>
 pearl_object_config_file = Pearl(user_name="Mantid2", config_file=config_file_path)
 
3. Using a combination of both, any parameter can be overridden from the 
configuration file without changing it:
::

 # This will use "My custom location" instead of the location set in the configuration file
 pearl_object_override = Pearl(user_name="Mantid3", config_file=config_file_path,
                               output_directory="My custom location")

Each object remembers its own properties - changing properties on another 
object will not affect others: In the above examples `pearl_object_override`
will save in *"My custom location"* whilst `pearl_manually_specified` will have user
name *"Mantid"* and save in *<Path to output folder>*. 

Vanadium Calibration
^^^^^^^^^^^^^^^^^^^^^
TODO 

Focussing
^^^^^^^^^^
TODO