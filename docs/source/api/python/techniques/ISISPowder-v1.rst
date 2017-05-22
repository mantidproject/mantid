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
