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

This interface can be used to perform the data reduction of the DNS Powder TOF data at MLZ. The interface can be accessed from the main menu of MantidWorkbench, in *Interfaces → Direct → DNS Reduction*. It can also be run standalone by running python **DNSReduction.py** in *qt/python/mantidqtinterfaces/mantidqtinterfaces*.

The interface performs data reduction for inelastic time of flight data processing for powders. At the moment it only works for data of the polarization analysis detector bank, not for the position sensitive detector.

Below the interface menu are the different tabs for data processing, which should be called from left to right during data processing.

Tabs Description
----------------

* :ref:`Paths <dns_paths_tab-ref>`
* :ref:`Data <dns_data_tab-ref>`
* :ref:`Script Generator <dns_script_generator_tab-ref>`
* :ref:`Powder TOF Options <dns_powder_tof_options_tab-ref>`
* :ref:`Powder TOF Plotting <dns_powder_tof_plotting_tab-ref>`

Feedback & Comments
-------------------

If you have any questions or comments about this interface or this help page,
please contact DNS instrument scientists
`DNS instrument webpage <https://mlz-garching.de/dns>`__.

.. categories:: Interfaces Direct
