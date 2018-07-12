.. _isis-powder-diffraction-Tutorials-ref:

==============================
ISIS Powder - Tutorials
==============================

.. contents:: Table of Contents
    :local:

Introduction
-------------
These tutorials assume you have read, understood and
completed the steps detailed at the following links:


- :ref:`script_param_overview_isis-powder-diffraction-ref`
- :ref:`prerequisites_isis-powder-diffraction-ref`

For these examples we will be using an arbitrary instrument,
and any differences between other instruments will be
clearly documented.

.. _obtaining_example_data_isis-powder-diffraction-ref:

Obtaining tutorial data
^^^^^^^^^^^^^^^^^^^^^^^
These tutorials will use the data found in the 
ISIS example data sets. The data sets can be downloaded
from the `Mantid download page <https://download.mantidproject.org/>`_,
under *Sample datasets* - *ISIS*. Users may also use their own
data instead however results will differ. Some additional
configuration may also be required which is explained in later tutorials.

.. _setup_tutorials_isis-powder-diffraction-ref:

Setting up
------------
This tutorial will help you setup the ISIS Powder
diffraction scripts for your instrument. It assumes
no prior knowledge or usage of the scripts and a fresh install of Mantid.

.. _copying_example_files_isis-powder-diffraction-ref:

Copying instrument example files
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Open your Mantid install location, by default this
will be `C:\\MantidInstall` on Windows and `/opt/Mantid` on Linux.
Open *scripts* > *Diffraction* > *isis_powder*.
In these tutorials we will be using Polaris examples and data
however you may set-up a different instrument. 

Open *polaris_routines* (or *'instName'_routines*), there will
be a folder called *Examples*. Copy the contents (all files and folders)
within the *Examples* folder to a known location.

Importing the instrument
^^^^^^^^^^^^^^^^^^^^^^^^^
Open up Mantid, then go to the scripting window by either pressing
**F3** or going to *View* - *Script Window*

First we need to import the instrument to be able to use it. If
you are using a different instrument substitute Polaris for your
instrument name:

.. code-block:: python

    # Note this IS case sensitive
    from isis_powder import Polaris

.. _intro_to_objects-isis-powder-diffraction-ref:

A quick introduction to objects
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
*This section is for script writers who are not familiar or comfortable
with object orientation. If you feel comfortable with the concept of
objects holding state you may skip onto the next section.*

The ISIS Powder scripts use objects to hold a state, an object is 
best illustrated with the below example:

.. code-block:: python

   blue_duck = Duck(type="Blue")
   red_duck = Duck(type="Rubber duck")

On lines 1 and 2 we create a new duck object. Each
object has a name we choose (in this case ``blue_duck`` and 
``red_duck``) and a separate state, but the actions we
can perform on each are the same. For example

.. code-block:: python

    blue_duck.feed()

We now have fed ``blue_duck`` so its state will have changed so it is no longer
hungry. However the ``red_duck`` has not changed at all so its state
is still hungry in this example.

Because objects have their own state you can create multiple objects
in your script to perform different actions, such as processing half
your data with one set of options and the other half of the data 
with another set of options.

.. _paths_to_setup_files_isis-powder-diffraction-ref:

Paths to the required files
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Navigate back to the files copied from this section of the 
tutorial :ref:`copying_example_files_isis-powder-diffraction-ref`.
There should be two files and a folder. If you are using the
ISIS example data set 
(see :ref:`obtaining_example_data_isis-powder-diffraction-ref`)
you will not need to modify anything at this point.

If you are not using the ISIS example data set you will need to
modify your calibration directory and cycle mapping as detailed
here: :ref:`cycle_mapping_files_isis-powder-diffraction-ref`

Take notes of the following paths as we will need them later:

- The path to the folder you are currently in
- The name of the 'calibration' folder
- The name of the cycle mapping file 

For example in the POLARIS example folder these filenames will be:

- Name of 'calibration' folder: **Calibration**
- Name of cycle mapping file: **polaris_cycle_map_example.YAML**
  -  *Note*: you may not have file extensions showing, in that case you
  will see 'polaris_cycle_map_example' and need to insert 
  **.YAML** after the filename

.. _creating_inst_object_isis-powder-diffraction-ref:

Creating the instrument object
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Having introduced objects in: 
:ref:`intro_to_objects-isis-powder-diffraction-ref` we can now
go ahead and create an instrument object. 

.. code-block:: python

    from isis_powder import Polaris

    # This name is arbitrary
    a_pol_obj = Polaris()

If you try to run this code the script will complain whenever it
comes across a parameter it requires but has not been set.
The following parameters must be set for all instruments:

- ``user_name``
- ``calibration_directory``
- ``output_directory``

There will also be additional instrument specific parameters required,
a list of these can be found in the relevant instrument reference:
:ref:`instrument_doc_links_isis-powder-diffraction-ref` for example
all instruments require a cycle mapping file. On HRPD, GEM and POLARIS
this is called the ``calibration_mapping_file``, on PEARL this is the
``calibration_config_path``. 

Using the above information we can start to populate the required
parameters (see :ref:`paths_to_setup_files_isis-powder-diffraction-ref`
for where these paths came from):

.. code-block:: python

    from isis_powder import Polaris

    a_pol_obj = Polaris(user_name="Your name here", 
                        calibration_directory=*Path to calibration directory*,
                        calibration_config_path=*Path to folder*\\*cycle mapping name.YAML*,
                        ....etc.)

Each time we execute the code it will inform us if a parameter is 
required at that point and we have forgotten to enter it. When you see
``Script execution finished`` it means we have enough information to
create the instrument object. 

In the next tutorial we will focus a vanadium run and use that to 
focus a standard sample.

Focusing first data set
------------------------
This tutorial assumes you have followed the steps in the previous
tutorial :ref:`setup_tutorials_isis-powder-diffraction-ref` and
have created an instrument object successfully.

We now have an object for the instrument we specified, if you followed
the previous tutorial this will be a Polaris object. 
These objects have methods we can access using their ``.`` operator. 
We will use this to create a vanadium run on Polaris:

.. code-block:: python
  :linenos:

    from isis_powder import Polaris

    a_pol_obj = Polaris(...)
    a_pol_obj.create_vanadium(...)

On line 4 we call the ``create_vanadium`` method on the Polaris object.
All instruments will have this method however the parameters they
accept and require are bespoke. Parameters can be
found for each individual instrument in the reference document:
:ref:`instrument_doc_links_isis-powder-diffraction-ref`

.. _how_objects_hold_state_isis-powder-diffraction-ref:

How objects hold state in ISIS Powder
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. warning:: This is NOT relevant for PEARL. PEARL scientists should
	     refer to :ref:`state_for_pearl_isis-powder-diffraction-ref`

Additionally as the objects hold state we can set a parameter
anywhere. For example on Polaris the ``mode`` parameter indicates
the chopper state for this/these run(s). This can either be set 
when we create the object like this:

.. code-block:: python

    from isis_powder import Polaris

    a_pol_obj = Polaris(mode="PDF", ....)
    a_pol_obj.create_vanadium(...)

Or set whilst calling a method like this:

.. code-block:: python

    from isis_powder import Polaris

    a_pol_obj = Polaris(...)
    a_pol_obj.create_vanadium(mode="PDF", ...)

Both of the above are equivalent. Additionally if we change the value
the scripts will warn us. This can be demonstrated with the following
example:

.. code-block:: python

    from isis_powder import Polaris

    a_pol_obj = Polaris(mode="PDF", ...)

    # The following line will warn us we changed the chopper
    # status from PDF to Rietveld. It will also remain 
    # in Rietveld mode from now on till we change it again
    a_pol_obj.create_vanadium(mode="Rietveld", ...)
    
    # Mode is still Rietveld on the following line
    a_pol_obj.create_vanadium(...) 

For these reasons it is recommended to create multiple objects
when you need to switch between different settings within a script:

.. code-block:: python

    from isis_powder import Polaris

    pol_PDF = Polaris(mode="PDF", ...)
    pol_Rietveld = Polaris(mode="Rietveld", ...)

    # Runs with the chopper set to PDF mode:
    pol_PDF.create_vanadium(...)
    # Runs with the chopper set to Rietveld mode:
    pol_Rietveld.create_vanadium(...) 

.. _creating_first_vanadium_run_isis-powder-diffraction-ref:

Creating the vanadium run
^^^^^^^^^^^^^^^^^^^^^^^^^^
Because of the way objects hold state in ISIS Powder 
(see: :ref:`how_objects_hold_state_isis-powder-diffraction-ref`)
it is up to the reader of this tutorial where they set different 
parameters. 

As previously mentioned each instrument has bespoke parameters
and can be found in the individual instrument reference document:
:ref:`instrument_doc_links_isis-powder-diffraction-ref`

Additionally as noted previously this tutorial assumes the user
is using the example ISIS data set (
see: :ref:`obtaining_example_data_isis-powder-diffraction-ref`).
If they are not they will need to setup their cycle mapping to their 
data detailed here: :ref:`cycle_mapping_files_isis-powder-diffraction-ref`

For Polaris we require the following parameters in addition to the
parameters discussed to create the object (see
:ref:`creating_inst_object_isis-powder-diffraction-ref`):

- ``do_absorb_corrections`` - Indicates whether to account for absorption when processing
  the vanadium data. It is recommended to have this set to ``True``
- ``first_cycle_run_no`` - Used to determine which cycle to create a vanadium for.
  For example on a cycle with runs 100-120 this value can be any value from 100-120 
  (e.g. 111)
- ``mode`` - Indicates what the chopper state was for this run
- ``multiple_scattering`` - Indicates whether to account for the effects of
  multiple scattering. For the tutorial it is highly recommended to set this to ``False``
  as it will increase the script run time from seconds to 10-30 minutes.

*Note: Due to the complexity of the Polaris instrument definition it will take 
Mantid up to 10 minutes to load your first data set for this instrument.*

As we will be later focusing run number 98533 we can use that to ensure
the correct cycle is selected for the ``first_cycle_run_no`` input.

.. code-block:: python

    from isis_powder import Polaris

    # This should be set from the previous tutorial. 
    a_pol_obj = Polaris(....)
    a_pol_obj.create_vanadium(first_cycle_run_no=98533,
                              do_absorb_corrections=True,
                              mode="Rietveld",
                              multiple_scattering=False)

Executing the above should now successfully process the vanadium run,
you should have two resulting workspaces for the vanadium run in 
dSpacing and TOF. Additionally there will be another workspace containing
the splines which will be used when focusing future data.

.. _focusing_data_isis-powder-diffraction-ref:

Focusing a data set
^^^^^^^^^^^^^^^^^^^^
Having successfully processed a vanadium run (see: 
:ref:`creating_first_vanadium_run_isis-powder-diffraction-ref`)
we are now able to focus a data set. For this tutorial we will
be focusing a sample of Silicon.

*It is highly recommended to create a separate script file for
focusing data, this ensures the vanadium is not reprocessed
every time data is focused.*

To focus data we can call the ``focus`` method present on all 
instruments. As previously mentioned each instrument has 
bespoke parameters, these can be found in the individual 
instrument reference document: 
:ref:`instrument_doc_links_isis-powder-diffraction-ref`

.. code-block:: python

    from isis_powder import Polaris
    # This should be set from the previous tutorial. 
    a_pol_obj = Polaris(....)

    a_pol_obj.focus(...)

To focus the Si sample included in the ISIS data set we 
require the following parameters:

- ``do_absorb_corrections`` - This will be covered in a later tutorial.
  It determines whether to perform sample absorption corrections on
  instruments which support this correction. For this tutorial please
  ensure it is set to ``False``
- ``do_van_normalisation`` - Determines whether to divide the data
  set by the processed vanadium splines. This should be set to 
  ``True``.
- ``input_mode`` - Some instruments will not have this 
  (in which case the data will always be summed). Acceptable values
  are ``"Individual"`` or ``"Summed"``. When set to individual each run
  will be loaded and processed separately, in summed all runs specified
  will be summed.
- ``mode`` - Indicates what the chopper state was for this run (eg
  ``"Rietveld"``)
- ``run_number`` - The run number or range of run numbers. This can
  either be a string or integer (plain number). For example 
  ``"100-105, 107, 109-111"`` will process 
  100, 101, 102..., 105, 107, 109, 110, 111.


For this tutorial the run number will be 98533, and ``input_mode``
will not affect the result as it is a single run. Additionally in
the example data you could focus 98534 (YAG sample) too.

.. code-block:: python

    from isis_powder import Polaris

    # This should be set from the previous tutorial. 
    a_pol_obj = Polaris(....)
    a_pol_obj.focus(input_mode="Individual", run_number=98533,
                    mode="Rietveld",
                    do_absorb_corrections=False,
                    do_van_normalisation=True)

This will now process the data and produce two workspace groups
for the results in dSpacing and TOF in addition to another group
containing the spline(s) used whilst processing the data.

Congratulations you have now focused a data set using ISIS Powder.

.. _output_folder_isis-powder-diffraction-ref:

Output files
^^^^^^^^^^^^^
After focusing the data it is saved in a variety of formats which
suits the instrument. These can be found in the user specified 
output directory. The scripts will automatically create the
label for the current cycle (covered in additional detail later
:ref:`cycle_mapping_files_isis-powder-diffraction-ref`).

Within the label folder a new folder will be created or used
matching the ``user_name`` specified. Within that folder will
be the output data in the various formats that is used on 
that instrument to perform data analysis.

.. _configuration_files_isis-powder-diffraction-ref:

Using configuration files
---------------------------
This tutorial assumes you have successfully created an instrument
object as described here: :ref:`creating_inst_object_isis-powder-diffraction-ref`.

You have probably noticed that a lot of the parameters set do not 
change whenever you create an instrument object and a warning 
is emitted stating you are not using a configuration file.

The rational behind a configuration file is to move settings which
rarely change but are machine specific to a separate file you can
load in instead. For example the output directory or calibration
directory do not change often. 

Creating a configuration file
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Navigate back to the files copied from the example folder (see:
:ref:`copying_example_files_isis-powder-diffraction-ref`). There is
a file we have not been using which will be named along the lines of
*'inst'_config_example.YAML*.

This will come pre-configured with some examples of how parameters are
set in the files. The names always match parameter names which
can be found in the instrument reference documentation:
:ref:`instrument_doc_links_isis-powder-diffraction-ref`

For example if we currently have the output directory as follows:

.. code-block:: python

    from isis_powder import Polaris

    # Note the r before " avoids us having to put \\
    a_pol_obj = Polaris(output_directory=r"C:\path\to\your\output_dir", ....)

We can instead move it to the YAML file so it reads as follows:

.. code-block:: yaml
    
    # YAML FILE:
    # Note the single quotes on a path in a YAML file
    output_directory: 'C:\path\to\your\output_dir'

Additionally we can move parameters which should be defaults into
the same file too:

.. code-block:: yaml

    #YAML FILE:
    output_directory: 'C:\path\to\your\output_dir'
    do_van_normalisation: True

.. warning:: Within the YAML files the most recent value also takes precedence.
             So if ``user_name`` appeared twice the value closest
             to the bottom will be used. This is implementation specific and
             should not be relied on. Users should strive to ensure each key - value
             pair appears once to avoid confusion.

Using the configuration file
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You will need to make a note of the full path to the configuration
file. Note that the filename entered must end with .YAML (even if it
is not shown when browsing the files on your OS).

Setting the configuration file from the previous example we 
now have a default output directory and perform vanadium normalisation
by default too. 

.. code-block:: python

    from isis_powder import Polaris

    config_file_path = r"C:\path\to\your\config_file.YAML"
    a_pol_obj = Polaris(config_file=config_file_path, ...)
    # Will now divide by the vanadium run by default as this was
    # set in the configuration file
    a_pol_obj.focus(...)

Any property set in the configuration file can be overridden. So
if you require a different output directory for a specific script
you can still use the original configuration file.

.. code-block:: python

    from isis_powder import Polaris

    config_file_path = r"C:\path\to\your\config_file.YAML"

    # Output directory changed to our own output directory, 
    # and warning emitted informing us this has happened
    a_pol_obj = Polaris(config_file=config_file_path,
                        output_dir=r"C:\path\to\new\output_dir", ...)

    # As the object has a state it will still be set to our custom
    # output directory here (instead of configuration one) without
    # restating it
    a_pol_obj.focus(...)

It is recommended instrument scientists move optimal defaults 
(such as performing vanadium normalisation) into a configuration
file which the scripts use.

.. _cycle_mapping_files_isis-powder-diffraction-ref:

Cycle mapping files
--------------------
The cycle mapping file is used to hold various details about the current
and past cycles. These details include the empty and vanadium run number(s),
current label and offset filename.

The *label* is used to separate output data into its various cycle numbers,
Mantid will correctly handle the cycle on input data. The goal of the label
is to ensure runs end up in the output folder the user wants them in, 
regardless of which cycle ISIS is on.

Examples
^^^^^^^^^
These examples explain the layout and concept of YAML files. For
instrument specific examples please look at the individual
instrument reference document:
:ref:`instrument_doc_links_isis-powder-diffraction-ref` for
an example specific to your instrument.

The simplest example of the calibration file is used on Pearl as the
empty, label and vanadium are the same regardless of mode.

.. code-block:: yaml
 
  # This is the layout used on PEARL
  # NB this example is not representative of actual run numbers
  123-200:
    # Notice how the indentation changes to indicate it belongs
    # to this section
    label : "1_2"
    vanadium_run_numbers : "150"
    empty_run_numbers : "160"
    offset_file_name : "pearl_offset_1_2.cal"  

On GEM the two chopper modes ``"PDF"`` and ``""Rietveld""`` affect the
empty and vanadium run numbers used. In this case the additional
indentation underneath the respective mode is used.

Fields can be left blank until a later date
if runs in different modes have not been collected yet. 

.. code-block:: yaml

    # This is the layout used on GEM
    # NB this example is not representative of actual run numbers
    123-200:
        label: "1_2"
        offset_file_name: "offsets.cal"
        PDF:
            # Blank entries are allowed provided we do not try to run in PDF mode
            vanadium_run_numbers: ""
            empty_run_numbers: ""
        # Notice it is not case sensitive
        rietveld:
            # The indentation indicates these are for Rietveld mode
            vanadium_run_numbers: "130"
            empty_run_numbers: "131"

Run numbers
^^^^^^^^^^^^^
The run numbers for a cycle use the same syntax as the run number field.
You can specify ranges of runs, have gaps or individual runs. For example
``"100-103, 105"`` will specify runs 100, 101, 102, 103 and 105.

The mapping also allows unbounded runs, this is useful for a cycle that
is in progress as the final run number of a cycle is unknown

.. code-block:: yaml
 
  1-122:
    label : "1_1"
    ...

  123-:
    label : "1_2"
    ...

All runs from 1-122 inclusive will go use the details associated with label
``1_1``, whilst any runs after 123 will use label ``1_2``. These values also
have validation to ensure that there is only one unbounded range and no values
come after the starting interval. For example in the above example adding a section
for runs ``200-`` or ``200-210`` would fail validation. 

Relation to calibration directory
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The user specified calibration directory directly relates to a cycle mapping
file. After writing or adapting a cycle mapping file for your instrument 
you must update the calibration directory. Using the cycle mapping from Peal:

.. code-block:: yaml
 
  # NB this example is not representative of actual run numbers
  123-200:
    label : "1_2"
    vanadium_run_numbers : "150"
    empty_run_numbers : "160"
    offset_file_name : "pearl_offset_1_2.cal"  

The relevant fields from the cycle mapping are the ``label`` and 
``offset_file_name``. Within the calibration directory a folder
with the ``label`` name must exist. ``offset_file_name`` must either
be the name of a cal file within that folder, or the full path to a
cal file elsewhere.

In this example we need a folder within the calibration 
directory called *1_2* which holds a
cal file called *pearl_offset_1_2.cal*.

Changing mid-cycle
^^^^^^^^^^^^^^^^^^^
The splines of the processed vanadium uses the run number
and offset file name as a fingerprint to uniquely identify
it. Because of this we can have two sets of details corresponding
to the same cycle.

.. code-block:: yaml
 
  # NB this example is not representative of actual run numbers
  123-150:
    label : "1_2"
    vanadium_run_numbers : "150"
    empty_run_numbers : "152"
    offset_file_name : "pearl_offset_1_2.cal"  

  151-200:
    label : "1_2"
    # Notice the changed details for runs 151 onwards
    vanadium_run_numbers : "170"
    empty_run_numbers : "160"
    offset_file_name : "pearl_offset_1_2-second.cal"  

Processing intermediate files
------------------------------
The scripts also support processing intermediate files. This
tutorial assumes you have successfully focused data
previously as detailed here: :ref:`focusing_data_isis-powder-diffraction-ref`.

To process intermediate runs for example *.s01* or *.s02* files
you must ensure the user directories are setup to 
include the folder where these files are located. 

The instructions for this can be found here: 
:ref:`prerequisites_isis-powder-diffraction-ref`.
*Note: The 'Search Data Archive' option will not locate
intermediate runs as only completed runs are published to the data archive.*

To indicate the extension to process the ``file_ext`` can be specified
like so:

.. code-block:: python

    from isis_powder import Polaris

    a_pol_obj = Polaris(....)

    a_pol_obj.focus(file_ext="s01", ...)
    # Or
    a_pol_obj.focus(file_ext=".s01", ...)

This will locate a .s01 file for that run number and focus
it like a normal run. The output filename will also reflect that
this is a partial file. For run number 123 and file extension s01 
the output filename will be *s01<InstrumentName>123.nxs*.
This allows users to easily distinguish between full runs and 
partial runs in the output folder. (For more details about the 
output folder see :ref:`output_folder_isis-powder-diffraction-ref`)

Absorption corrections on sample
----------------------------------
This tutorial assumes you have successfully focused data
previously as detailed here: :ref:`focusing_data_isis-powder-diffraction-ref`.

To perform absorption corrections on a sample we must first specify
the chemical properties of the sample by creating a sample properties
object. (See :ref:`intro_to_objects-isis-powder-diffraction-ref`.)

*Note*: Not all instruments support sample absorption corrections.
Please check the instrument reference: 
:ref:`instrument_doc_links_isis-powder-diffraction-ref`. If the
instrument has a ``set_sample_details`` method it supports sample 
absorption corrections

.. _create_sampleDetails_isis-powder-diffraction-ref:

Create SampleDetails object
^^^^^^^^^^^^^^^^^^^^^^^^^^^^
First we need to import the sample details object from ISIS Powder. 
The properties required when creating a SampleDetails
object is the geometry of the sample.

**Note: this assumes a cylinder geometry**

- ``height`` - Cylinder height
- ``radius`` - Cylinder radius
- ``center`` - List of x, y, z positions of the cylinder

For more details see :ref:`algm-SetSample-v1`.

.. code-block:: python

    from isis_powder import Polaris, SampleDetails

    # Creates a cylinder of height 3.0, radius 2.0
    # at position 0, 1, 2 (x, y, z)
    position = [0, 1, 2]

    # Create a new sample details object
    my_sample = SampleDetails(height=3.0, radius=2.0, center=position)

.. _set_material_sampleDetails_isis-powder-diffraction-ref:

Setting the material details
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Having set the sample geometry we can now set the chemical 
material and optionally the number density. If the chemical
formula is not a single element the number density must be
entered as it cannot be calculated.

For accepted syntax of chemical formulas see
:ref:`algm-SetSampleMaterial-v1`. Specifically the section
on specifying chemical composition if you are using isotopes.
This will allow Mantid to automatically calculate the properties
except for number density.

*The material must be set before absorption corrections can
be calculated for a sample.*

.. code-block:: python

    ... snip from previous example ...
    my_sample = SampleDetails(height=3.0, radius=2.0, center=position)
    
    my_sample.set_material(chemical_formula="V")
    # OR
    my_sample.set_material(chemical_formula="VNb", number_density=123)


Setting material properties
^^^^^^^^^^^^^^^^^^^^^^^^^^^
Advanced material properties can be optionally set instead of letting 
Mantid calculate them. These properties are:

- ``absorption_cross_section`` - Absorption Cross Section
- ``scattering_cross_section`` - Scattering Cross Section

*Note: This is purely optional and Mantid will calculate these
values based on the chemical formula entered if this is not set*

.. code-block:: python

    ... snip from previous example ...
    my_sample = SampleDetails(height=3.0, radius=2.0, center=position)
    my_sample.set_material(chemical_formula="VNb", number_density=123)
    
    # Setting individual properties:
    my_sample.set_material_properties(absorption_cross_section=123, 
                                      scattering_cross_section=456)

Using the new SampleDetails object
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Having created a new SampleDetails object 
(:ref:`create_sampleDetails_isis-powder-diffraction-ref`) and then
set the chemical material (:ref:`set_material_sampleDetails_isis-powder-diffraction-ref`)
we can instruct the scripts to use these details whilst focusing. 

This is done by calling ``set_sample_details`` on the instrument object,
this will then use those sample details each time absorption corrections
are applied to the sample. (See :ref:`how_objects_hold_state_isis-powder-diffraction-ref`)

.. code-block:: python

    from isis_powder import Polaris, SampleDetails
    ... snip from previous examples ...
    my_sample = SampleDetails(...)
    my_sample.set_material(...)

    polaris_obj = Polaris(...)
    polaris_obj.set_sample_details(sample=my_sample)

    # Indicate we want to perform sample absorption corrections whilst focusing
    polaris_obj.focus(do_absorb_corrections=True, ...)

Changing sample properties
^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. warning:: This method is not recommended for changing multiple samples. 
             Instead it is recommended you create a new sample details object
             if you need to change properties mid way through a script. 
             See :ref:`create_sampleDetails_isis-powder-diffraction-ref`
             and :ref:`intro_to_objects-isis-powder-diffraction-ref`.

*Note: The geometry of a sample cannot be changed without creating a new 
sample details object*

Once you have set a material by calling ``set_material`` or set 
the properties by calling ``set_material_properties`` you will 
not be able to change (or set) these details without first
resetting the object. This is to enforce the sample properties 
being set only once so that users are guaranteed of the state. 

If you wish to change the chemical material or its advanced properties
without creating a new sample details object you can call 
``reset_sample_material``. This will reset **all** details (i.e
advanced properties and chemical properties)

.. code-block:: python

    from isis_powder import Polaris, SampleDetails

    my_sample = SampleDetails(...)
    my_sample.set_material(...)

    # Next line will throw as it has already been set once
    my_sample.set_material(...)
    # This is still ok as its first time
    my_sample.set_material_properties(...)

    # Reset material
    my_sample.reset_sample_material()
    # Now allowed as object does not have a chemical formula associated
    my_sample.set_material(...)

.. _set_beam_parameters-ref:

Setting beam parameters
-----------------------

The beam width and height can be set for the instrument.
These are then used for total scattering corrections.

.. code-block:: python

 from isis_powder import Polaris
 polaris_obj = Polaris(...)
 polaris.obj.set_beam_parameters(height=1.23, width=4,56)

.. _instrument_advanced_properties_isis-powder-diffraction-ref:

Instrument advanced properties
-------------------------------
.. warning:: This section is intended for instrument scientists.
             The advanced configuration distributed with Mantid
             use optimal values for each instrument and
             should not be changed unless you understand what you
             are doing.

*Note*: Parameters should not be changed in the advanced configuration
for a few runs. If you require a set of values to be changed for a range
of runs (such as the cropping values) please set the value in the scripting
window or configuration file instead
(see: :ref:`configuration_files_isis-powder-diffraction-ref`).

The advanced configuration file provides optimal defaults for 
an instrument and applies to all runs unless otherwise specified. If
this file is modified Mantid will **not** remove it on uninstall or
reinstall, or upgrade. *(Note: This behavior is not guaranteed and
should not be relied on)*

It is highly recommended you read the instrument reference 
found here: :ref:`instrument_doc_links_isis-powder-diffraction-ref`
to understand the purpose of each property and the effect changing
it may have.

**If you change any values in your advanced properties file could
you please forward the new value to the Mantid development team
to ensure this new value is distributed in future versions of Mantid**

For the purposes of testing a parameter can be overridden at
script runtime. The hierarchy of scripts is:
*scripting window* > *config file* > *advanced config*.
In other words a value set in the configuration file will
override one found in the advanced configuration file.
A value set in the scripting window will override one
found in the configuration file.

A warning will always be emitted when a value is overridden
so that the user is fully aware when this is happening.

For example to test a different spline coefficient value

.. code-block:: python

    from isis_powder import Polaris

    a_pol_obj = Polaris(spline_coefficient=80, ...)
    a_pol_obj.create_vanadium(...)

This will create a new vanadium run with the spline coefficient
set to 80. Note that until create_vanadium is run again
in this example any future data will implicitly use the 
splines with a coefficient of 80.

If you wish to change or view the advanced configuration files
these can be found under 
*MantidInstall*/scripts/diffraction/isis_powder/**inst** _routines
and will be called **inst** _advanced_config.py

If you change a value within the advanced config file you will
need to restart Mantid for it to take effect. If you are happy
with the new value please ensure you forward it on to the Mantid
development team to be distributed in future versions.

.. categories:: Techniques
