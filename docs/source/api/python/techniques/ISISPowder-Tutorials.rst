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
will see **polaris_cycle_map_example** and need to remember the 
**.yaml** after later*

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

We now have an object for the instrument we specified, in the
case of the tutorial a Polaris object. These objects have methods
we can access using their '.' operator. We will use this to
create a vanadium run on Polaris:

.. code-block:: python

    from isis_powder import Polaris

    a_pol_obj = Polaris(...)
