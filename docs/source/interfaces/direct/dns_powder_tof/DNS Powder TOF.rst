.. _dns_powder_tof-ref:

.. toctree::


DNS Powder TOF
==============

Interface Overview
------------------
.. image::  ../../../images/DNS_interface_powder_tof_overview.png
   :align: center
   :height: 400px

\

This interface can be used to perform the data reduction of DNS Powder TOF data at MLZ. The interface can be accessed from the main menu of MantidWorkbench by clicking on "Interfaces" → "Direct" and selecting "DNS Reduction". It can also be initialized standalone by running the python file ``DNS_Reduction.py`` in ``qt/python/mantidqtinterfaces/mantidqtinterfaces``.

The interface performs data reduction for inelastic time of flight data processing for powders. At the moment it only works for data collected by the polarization analysis detector bank, but not those collected by the position sensitive detector.

Below the interface menu, there are various tabs for data processing. These tabs should be called from left to right during the data processing.

Tabs Usage and Description
--------------------------

* :ref:`Paths <dns_powder_tof_paths_tab-ref>`
* :ref:`Data <dns_powder_tof_data_tab-ref>`
* :ref:`Options <dns_powder_tof_options_tab-ref>`
* :ref:`Script Generator <dns_powder_tof_script_generator_tab-ref>`
* :ref:`Plotting <dns_powder_tof_plotting_tab-ref>`

Feedback & Comments
-------------------

If you have any questions or comments about this interface or this help page,
please contact DNS instrument scientists
`DNS instrument webpage <https://mlz-garching.de/dns>`__.

.. categories:: Interfaces Direct
