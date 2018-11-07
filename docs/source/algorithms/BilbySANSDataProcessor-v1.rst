.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The main equation Bilby is using for the :math:`I(Q)` calculation is the following:

.. math:: I(Q)=\frac{1}{d_{sam}}.\frac{\sum_{R, \lambda \subset Q}C_{sam,corr}(R,\lambda)}{M \sum_{R,\lambda \subset Q}T_{corr}(R, \lambda)\frac{I_{empty\_beam}(\lambda)}{M_{empty\_beam}att_{empty\_beam}}\Omega(R)Det_{flood}(R)}

Where :math:`C_{sam, corr}` is the measured counts per pixel per wavelength,
:math:`I_{empty\_beam}` is the intensity of the empty beam collected for :math:`M_{empty\_beam}` time,
M is a time measure for the data collection, :math:`att_{empty\_beam}` is attenuation factor,
:math:`\Omega(R)` is solid angle, :math:`Det_{flood}` is a detector response function,
:math:`T_{corr}` is the sample transmission, and :math:`d_{sam}` is the sample thickness.

Details are described in the paper
[Sokolova, A., Christoforidis, J., Eltobaji, A., Barnes, J., Darmann, F., Whitten, A. E. & de Campo, L. (2016). Neutron News 27, 9-13]
The core algorithms the BilbySANSDataProcessor is utilising are :ref:`Q1D <algm-Q1D>` and :ref:`TOFSANSResolutionByPixel <algm-TOFSANSResolutionByPixel>`.
Please refer to those pages for details of the input parameters.
The unit of the output workspace is 1/cm. Absolute scale calibration done relatively to the empty beam transmission measurements.

Usage
-----

See https://github.com/hortica/Mantid_Bilby page to download a set of the test input data.

Please note, currently we do not have a User Interface, so we are working with csv lists.

To make the Bilby data reduction working, follow the steps listed below:

* Two csv files, similar to (input_csv_example.csv and mantid_reduction_settings_example.csv) shall be created during the experiment; the names can be different, the format (especially the top line in each) must stay the same
* Download Mantid, install it; don't open it before the next step will be completed;
   + For the Mac users, note to Click on the Mantid.app with the control key and select package contents; after that it will be come obvious where the *.py files should go
* Download scripts from reduction_scripts_const
* Two following scripts can be saved everywhere, but important thing is that the folder shall be on the path (added to “Mantid User Directories”)
   * BilbyCustomFunctions_Reduction.py
   * shift_assembled.csv
* Add the folder with your *.tar files AND two csv files (input_csv_example.csv and mantid_reduction_settings_example.csv) on the path (added to “Mantid User Directories”)
   * To keep it simple, one can put tar and csv files in one folder, but you can add as many folders on the paths as you want
* Open the Script menu in Mantid: press “View”, and then “Script window”; alternalively just press F3
* Open the reducer script reducer_example.py (in the “Python Window” open file, just “Ctrl-O” or File -> Open), update lines 15-27 accordingly
* Run the reducer, Execute -> Execute All in the Menu
   * All output 1D files will be saved in the folder you define in the mantid_reduction_settings_example.csv

References
----------

Sokolova, A., Christoforidis, J., Eltobaji, A., Barnes, J., Darmann, F., Whitten, A. E. & de Campo, L. (2016). Neutron News 27, 9-13

.. sourcelink::

.. categories::