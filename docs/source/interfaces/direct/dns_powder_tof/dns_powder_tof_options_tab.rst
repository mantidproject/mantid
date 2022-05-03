.. _dns_powder_tof_options_tab-ref:

DNS Powder TOF Options Tab
--------------------------

.. image::  ../../../images/DNS_interface_powder_tof_options_tab.png
   :align: center
   :height: 400px
\

This tab of the powder TOF mode defines the options for the reduction script.
The most useful configuration is preselected so that one just needs to set the
binning and proceed to the next tab if no change to the selected options is
desired.

The **Wavelength** value can be overwritten, for example for the cases when
:math:`\lambda/2` was selected by the velocity selector.

**Detector Efficiency Correction** norms the detector efficiency using standard
vanadium data.

Instrument background can be subtracted from vanadium and sample data,
as well as corrected by scaling factors, by using the **Subtract Instrument
Background from Vanadium** option and the **Factor** field below it.

**Correct Elastic Peak Position to 0 meV** shifts the elastic peak to 0 meV
using a fit of the elastic line of the vanadium data. This is necessary because
the zero time signal is not calibrated at DNS at the moment. Alternatively,
the elastic peak channel can be given manually.

The binning can be set automatically to the values that would enable a user
to display the full dataset by clicking on the **Sugest from Data** botton.
If necessary, the binning can be set manually to the desired values.
