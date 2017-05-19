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
clearly highlighted and documented.

.. _obtaining_example_data_isis-powder-diffraction-ref:

Obtaining tutorial data
^^^^^^^^^^^^^^^^^^^^^^^
These tutorials will use the data found in the 
ISIS example data sets. The data sets can be downloaded
from the `Mantid download page <https://download.mantidproject.org/>`_,
under *Sample datasets* - *ISIS*. Users may also use their own
data instead however they results may differ and some additional
configuration may be required which is explained in later tutorial.

.. _setup_tutorials_isis-powder-diffraction-ref:

Setting up
------------
This tutorial will help you setup the ISIS Powder
diffraction scripts for your instrument. It assumes
no prior knowledge or usage and a fresh install of Mantid.

.. _copying_example_files_isis-powder-diffraction-ref:

Copying instrument example files
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Open your Mantid install location, by default on Windows this
will be `C:\\MantidInstall` on Linux `/opt/Mantid`. Open *scripts*
*diffraction* and *isis_powder*. For this tutorial we will be using
Polaris data however you may set-up a different instrument. 

Open *polaris_routines* (or *'instName'_routines*), there will
be a folder called *examples*. Copy the contents (all files and folders)
within the *examples* folder to a known location.

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

Quick introduction to objects
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
*This section is for script writers who are not familiar or comfortable
with object orientation. If you feel comfortable with the concept of
objects holding state you may skip onto the next section.*

The ISIS Powder scripts use objects to hold a state, an object is 
best illustrated with the below example:

.. code-block:: python

   blue_duck = Duck(name="Blue")
   red_duck = Duck(name="Rubber duck")

On lines 1 and 2 we create a new object, specifically a duck. Each
object which can have a name we choose (in this case *blue_duck* and 
*red_duck*). Each object has separate state, but the same actions we
can perform on it. For example

.. code-block:: python

    blue_duck.feed()

We now have fed *blue_duck* so its state will have changed to no longer
being hungry. However the *red_duck* was not changed at all so its state
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
this will be configured for you.

If you are not using the ISIS example data set you will need to
modify your calibration directory and cycle mapping as detailed
here : TODO link to later tutorial section

Take notes of the following paths as we will need them later:
- The path to the folder you are currently in
- The name of the 'calibration' folder
- The name of the cycle mapping file 

For example in the POLARIS example folder these filenames will be:
- Name of 'calibration' folder: **Calibration**
- Name of cycle mapping file: **polaris_cycle_map_example.yaml**
*Note you may not have file extensions showing, in that case you
will see **polaris_cycle_map_example** and need to remember to insert 
**.yaml** after the filename*

.. _creating_inst_object_isis-powder-diffraction-ref:

Creating the instrument object
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
With the understanding of an object described in: 
:ref:`intro_to_objects-isis-powder-diffraction-ref` we can now
create an instrument object. 

.. code-block:: python

    from isis_powder import Polaris

    # This name is arbitrary
    a_pol_obj = Polaris()

If you try to run this code the script will complain whenever it
comes across a parameter is requires but has not been set yet.
The following parameters must be set for all instruments:

- *user_name*
- *calibration_directory*
- *output_directory*

There will also be additional instrument specific parameters required,
a list of these can be found in the relevant instrument reference: 
:ref:`instrument_doc_links_isis-powder-diffraction-ref` for example
we require the cycle mapping file. On GEM and POLARIS this is 
called the *calibration_mapping_file* on PEARL this is the 
*calibration_config_path*. 

Using the above information we can start to populate the required
parameters (see :ref:`paths_to_setup_files_isis-powder-diffraction-ref`
for where these paths came from):

.. code-block:: python

    from isis_powder import Polaris

    a_pol_obj = Polaris(user_name="Your name here", 
                        calibration_directory=*Path to calibration directory*,
                        calibration_config_path=*Path to folder*\\*cycle mapping name.yaml*,
                        ....etc.)

Each time we execute the code it should inform us which parameter is 
required at that point and we have forgotten to enter. When you see
*script execution finished* it means we have enough information to
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
These objects have methods we can access using their '.' operator. 
We will use this to create a vanadium run on Polaris:

.. code-block:: python
  :linenos:

    from isis_powder import Polaris

    a_pol_obj = Polaris(...)
    a_pol_obj.create_vanadium(...)

On line 4 we call the create_vanadium method on the Polaris object,
all instruments will have this method however the parameters they
accept and require are bespoke to each instrument. These can be
found in each individual instrument reference document:
:ref:`instrument_doc_links_isis-powder-diffraction-ref`

.. _how_objects_hold_state_isis-powder-diffraction-ref:

How objects hold state in ISIS Powder
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Additionally as the objects hold state we can set a parameter
anywhere. For example on Polaris that have a *chopper_on* indicates
the chopper state for this/these run(s). This can either be set 
when we create the object like this:

.. code-block:: python

    from isis_powder import Polaris

    a_pol_obj = Polaris(chopper_on=True, ....)
    a_pol_obj.create_vanadium(...)

Or set whilst calling a method like this:

.. code-block:: python

    from isis_powder import Polaris

    a_pol_obj = Polaris(...)
    a_pol_obj.create_vanadium(chopper_on=True, ...)

Both of the above are equivalent. Additionally if we change the value
the scripts will warn us. This can be demonstrated with the following
example:

.. code-block:: python

    from isis_powder import Polaris

    a_pol_obj = Polaris(chopper_on=True, ...)

    # The following line will warn us we changed the chopper
    # status from True to False. It will also remain False
    # from now on
    a_pol_obj.create_vanadium(chopper_on=False, ...)
    
    # Chopper_on is still False on the following line
    a_pol_obj.create_vanadium(...) 

For these reasons it is recommended to create multiple objects
when you needs to have switch between settings within a script:

.. code-block:: python

    from isis_powder import Polaris

    pol_chopper_on = Polaris(chopper_on=True, ...)
    pol_chopper_off = Polaris(chopper_on=False, ...)

    # Runs with chopper on:
    pol_chopper_on.create_vanadium(...)
    # Runs with chopper off:
    pol_chopper_off.create_vanadium(...) 

.. _creating_first_vanadium_run_isis-powder-diffraction-ref:

Creating the vanadium run
^^^^^^^^^^^^^^^^^^^^^^^^^^
Because of the way objects hold state in ISIS Powder 
(discussed here :ref:`how_objects_hold_state_isis-powder-diffraction-ref`)
it is up to the reader of this tutorial where they set different 
parameters. 

As previously mentioned each instrument has bespoke parameters
and can be found in the individual instrument reference document:
:ref:`instrument_doc_links_isis-powder-diffraction-ref`

Additionally as noted previously this tutorial assumes the user
is using the example ISIS data set (:ref:`obtaining_example_data_isis-powder-diffraction-ref`)
if they are not they will need to setup their cycle mapping to their 
data detailed here: TODO

For Polaris we require the following parameters in addition to the
parameters discussed to create the object (see
:ref:`creating_inst_object_isis-powder-diffraction-ref`):

- *chopper_on* - Indicates what the chopper state was for this run
- *first_cycle_run_no* - Used to determine which cycle to create a vanadium for.
  For example on a cycle with runs 100-120 this value can be any value from 100-120 
  (e.g. 111)
- *do_absorb_corrections* - Indicates whether to account for absorption when processing
  the vanadium data. It is recommended to have this set to *True*
- *multiple_scattering* - Indicates whether to account for the effects of
  multiple scattering. For the tutorial it is highly **recommended to set this to False**
  as it will increase the script run time from seconds to 10-30 minutes.

*Note: Due to the complexity of the Polaris instrument definition it will take 
Mantid up to 10 minutes to load your first data set for this instrument.
Subsequent loads will take seconds so please be patient.*

.. code-block:: python

    from isis_powder import Polaris

    # This should be set from the previous tutorial. 
    a_pol_obj = Polaris(....)
    a_pol_obj.create_vanadium(chopper_on=False,
                              first_cycle_run_no=TODO
                              do_absorb_corrections=True
                              multiple_scattering=False)

Executing the above should now successfully process the vanadium run,
you should have two resulting workspaces for the vanadium run in 
dSpacing and TOF. Additionally there will be another workspace containing
the splines which will be used when focusing future data.

Focusing a data set
^^^^^^^^^^^^^^^^^^^^
Having successfully processed a vanadium run (see: 
:ref:`creating_first_vanadium_run_isis-powder-diffraction-ref`)
we are now able to focus a data set. For this tutorial we will
be focusing a standard sample of Silicon.

*It is recommended to create a separate script file for
focusing data, this ensures the vanadium is not reprocessed
every time data is focused.*

To focus data we can call the *focus* method present on all 
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

- *input_mode* - Some instruments will not have this 
  (in which case the data will always be summed). Acceptable values
  are **Individual** or **Summed**. When set to individual each run
  will be loaded and processed separately, in summed all runs specified
  will be summed.
- *run_number* - The run number or range of run numbers. This can
  either be a string or integer (plain number). For example 
  *"100-105, 107, 109-111"* will process 
  100, 101, 102..., 105, 107, 109, 110, 111.
- *do_absorb_corrections* - This will be covered in a later tutorial
  it determines whether to perform sample absorption corrections on
  instruments which support this correction. For this tutorial please
  ensure it is set to *False*
- *do_van_normalisation* - Determines whether to divide the data
  set by the processed vanadium splines. This should be set to 
  *True*.

For this tutorial the run number will be TODO, and *input_mode*
will not affect the result as it is a single run.

.. code-block:: python

    from isis_powder import Polaris

    # This should be set from the previous tutorial. 
    a_pol_obj = Polaris(....)
    a_pol_obj.focus(input_mode="Individual", run_number=TODO,
                    do_absorb_corrections=False,
                    do_van_normalisation=True)

This will now process the data and produce two workspace groups
for the results in dSpacing and TOF in addition to another group
containing the spline(s) used whilst processing the data.

Congratulations you have now focused a data set using ISIS Powder.

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
*'inst'_config_example.yaml*.

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

.. code-block:: YAML

    # Note the single quotes on a path in a YAML file
    output_directory: 'C:\path\to\your\output_dir'

Additionally we can move parameters which should be defaults into
the same file too:

.. code-block:: YAML

    output_directory: 'C:\path\to\your\output_dir'
    do_van_normalisation: True

Using the configuration file
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You will need to make a note of the full path to the configuration
file. Note that the filename entered must end with .yaml (even if it
is not shown when browsing the files on your OS).

Setting the configuration file at instrument object creation
from the previous example we now have a default output directory.
Additionally we will perform vanadium normalisation by default too. 

.. code-block:: python

    from isis_powder import Polaris

    config_file_path = r"C:\path\to\your\config_file.yaml"
    a_pol_obj = Polaris(config_file=config_file_path, ...)
    # Will now divide by the vanadium run by default as this was
    # set in the configuration file
    a_pol_obj.focus(...)

Any property set in the configuration file can be overridden. So
if you require a different output directory for a specific script
you can still use the original configuration file.

.. code-block:: python

    from isis_powder import Polaris

    config_file_path = r"C:\path\to\your\config_file.yaml"

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
file which user scripts use.

