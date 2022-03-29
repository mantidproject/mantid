.. _DNS_Reduction-ref:

.. toctree::
   :hidden:
   :glob:
   :maxdepth: 1

   *

DNS Reduction
=============
Interface Overview
------------------
.. image::  ../../../images/DNS_interface_overview.png
   :align: center
   :height: 400px
\

This interface is used to analyse elastic and inelastic data of the DNS
instrument at MLZ. The interface can be accessed from the main menu of
MantidWorkbench, in *Interfaces → Direct → DNS Reduction*.
It can also be run standalone by running python **DNSReduction.py** in
*qt/python/mantidqtinterfaces/mantidqtinterfaces*.

The interface has separate modi for different kinds of measurements at DNS,
they can be accessed over the interface menu for example
**Tools → Change Mode -> Powder TOF** changes to the mode for
inelastic time of flight data processing for powders.
At the moment it only works for data of the polarization analysis detector bank,
not for the position sensitive detector.

Below the interface menu are the different tabs for data processing,
which should be called from left to right during data processing.
They can be common for multiple modi or specific for one modus.

Common Tabs
----

* :ref:`Paths Tab <dns_paths_tab-ref>`
* :ref:`Data Tab <dns_data_tab-ref>`
* :ref:`Script Generator Tab <dns_script_generator_tab-ref>`

Specific Tabs
----

* :ref:`Powder TOF Options Tab <dns_powder_tof_options_tab-ref>`
* :ref:`Powder TOF Plotting Tab <dns_powder_tof_plotting_tab-ref>`

Feedback & Comments
-------------------

If you have any questions or comments about this interface or this help page,
please contact DNS instrument scientists
`DNS instrument webpage <https://mlz-garching.de/dns>`__.

.. categories:: Interfaces Direct
