.. _dns_powder_tof_options_tab-ref:

DNS Powder TOF Options Tab
--------------------------

.. image::  ../../../images/DNS_interface_powder_tof_options_tab.png
   :align: center
   :height: 400px
\

The **Options tab** of the powder TOF mode defines the options for the
reduction script, the most useful configuration is preselected and the whole tab
can be skipped if no change to the options is desired.

The **wavelength** can be overwritten, for example for cases where lambda/2 was
selected by the velocity selector.

**Detector efficiency correction** norms the detector efficiency using Vanadium
standard data.

The **instrument background** can be subtracted from Vanadium and sample data,
and corrected by scaling factors.

**Correct elastic peak position to 0 meV** shifts the elastic peak to 0 meV
using a fit of the elastic line of the vanadium data which is necessary since
atm. at DNS the zero time signal is not calibrated.
Alternatively the elastic peak channel can be given manually.

The **Binning** is automatically chosen to show the full dataset if nothing is
selected but can be overwritten.

:ref:`DNS Reduction <dns_reduction-ref>`
