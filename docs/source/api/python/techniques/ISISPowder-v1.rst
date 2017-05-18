.. _isis-powder-diffraction-ref:

================================
ISIS Powder Diffraction Scripts
================================

.. contents:: Table of Contents
    :local:

.. _script_param_overview_isis-powder-diffraction-ref:

General Information
---------------------
These scripts provide Mantid users a way to process data captured
on various diffraction instruments. It aligns the various detectors
in d-Spacing, focuses the data into banks and applies any relevant
corrections in the process. The scripts proceed to save the focused
data into several output files for later use in data analysis packages.

This documentation assumes the user is familiar with the concepts
behind diffraction reduction and understand how cycles at ISIS work.
It also assumes the user has a basic knowledge of Python.

This documentation also avoids discussing the implementation or design of 
the scripts as it is beyond the scope of this documentation.

.. _prerequisites_isis-powder-diffraction-ref:

Prerequisites 
---------------
Users must setup their input directories so Mantid can find the input files. Instructions
on completing this are located `here <http://www.mantidproject.org/ManageUserDirectories>`_.
*Note: Mantid will not search folders recursively each folder must be added*

Additionally *Search Data Archive* can be ticked if the device is located on the ISIS
network to automatically handle finding the files whilst it is on the network.

.. _tutorial_links_isis-powder-diffraction-ref:

Tutorials
----------
The links below will take you to the relevant part of a tutorial.
These will help you setup your scripts and get you familiar with 
their usage.

:ref:`isis-powder-diffraction-Tutorials-ref`

.. _instrument_doc_links_isis-powder-diffraction-ref:

Instrument Reference
---------------------------------
- :ref:`isis-powder-diffraction-gem-ref`
- :ref:`isis-powder-diffraction-pearl-ref`
- :ref:`isis-powder-diffraction-polaris-ref`

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
This avoids having to re-enter paths or settings which rarely change such as the location of the calibration folder.
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
requirements and is documented as part of the :ref:`instrument_doc_links_isis-powder-diffraction-ref`

- The first line in all examples holds the run numbers.
- This is the range of runs inclusively for example *123-130*
- If the ending number is not known the range can be left unbounded for example
  *123-* this would match any runs with a run number greater or equal to 123


There is several sanity checks in place that ensure there is not multiple
unbounded entries and that all other runs specified are not within the unbounded range.
