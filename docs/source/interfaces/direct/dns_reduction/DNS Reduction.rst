.. _dns_reduction-ref:

=============
DNS Reduction
=============

This interface can be used to perform reduction of data collected at the
DNS instrument at MLZ. Three different types of data can be loaded with
the help of this interface. These data types correspond to the following
operational modes of the instrument: "Powder TOF", "Powder Elastic", and
"Single Crystal Elastic". Additionally, the interface provides a possibility
of simulating DNS data using the "Simulation" mode.

At the moment, the interface only works for data collected by the
polarization analysis detector bank, but not those collected by the
position sensitive detector.

The interface can be accessed from the main menu of MantidWorkbench by
clicking on "Interfaces" → "Direct" and selecting "DNS Reduction".
It can also be initialized standalone by running the python file
``DNS_Reduction.py`` in ``qt/python/mantidqtinterfaces/mantidqtinterfaces``.

Once the interface is selected, the user should select one of the four
available operational modes described below.

.. toctree::
   :hidden:
   :glob:
   :maxdepth: 1

Interface Usage and Description
-------------------------------

* :ref:`DNS Powder TOF <dns_powder_tof-ref>`
* :ref:`DNS Powder Elastic <dns_powder_elastic-ref>`
* :ref:`DNS Single Crystal Elastic <dns_single_crystal_elastic-ref>`
* DNS Simulation

Feedback & Comments
-------------------

If you have any questions or comments about this interface or this help page,
please contact DNS instrument scientists
`DNS instrument webpage <https://mlz-garching.de/dns>`__.

.. categories:: Interfaces Direct
