.. _dns_powder_tof-ref:

.. toctree::


DNS Powder TOF
==============

Interface Overview
------------------
.. image::  ../../../images/DNS_interface_overview.png
   :align: center
   :height: 400px
\

This interface can be used to perform the data reduction of DNS Powder TOF data at MLZ. The interface can be accessed from the main menu of MantidWorkbench, in *Interfaces → Direct → DNS Reduction*. It can also be run standalone by running python **DNS_Reduction.py** in *qt/python/mantidqtinterfaces/mantidqtinterfaces*.

The interface performs data reduction for inelastic time of flight data processing for powders. At the moment it only works for data collected by the polarization analysis detector bank, but not those collected by the position sensitive detector.

Below the interface menu, there are different tabs for data processing. These tabs should be called from left to right during the data processing.

Tabs Usage and Description
--------------------------

* :ref:`Paths <dns_paths_tab-ref>` Tab 
* :ref:`Data <dns_data_tab-ref>` Tab 
* :ref:`Script Generator <dns_script_generator_tab-ref>` Tab
* :ref:`Options <dns_powder_tof_options_tab-ref>` Tab
* :ref:`Plotting <dns_powder_tof_plotting_tab-ref>` Tab

Feedback & Comments
-------------------

If you have any questions or comments about this interface or this help page,
please contact DNS instrument scientists
`DNS instrument webpage <https://mlz-garching.de/dns>`__.

.. categories:: Interfaces Direct
