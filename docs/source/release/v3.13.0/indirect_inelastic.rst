==========================
Indirect Inelastic Changes
==========================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

:ref:`Release 3.13.0 <v3.13.0>`

Data Reduction Interfaces
-------------------------

- Added 'Sum Files' checkbox to ISIS Calibration, to sum a specified range of input files on load.

Algorithms
----------

New
###

- :ref:`algm-QENSFitSequential` can be used to perform a general QENS sequential fit, in addition providing the
  functionality to fit across multiple datasets.
- :ref:`algm-QENSFitSimultaneous` can be used to perform a general QENS simultaneous fit, including across multiple
  datasets.
- :ref:`algm-ConvolutionFitSimultaneous` can be used to perform a QENS simultaneous fit over a convoluted model.
- :ref:`algm-IqtFitSimultaneous` can be used to perform a QENS simultaneous fit over I(Q,t) data.


Improved
########

- :ref:`algm-ConvolutionFitSequential` and :ref:`algm-IqtFitSequential` can now accept multiple datasets as input, in
  the same format as that of :ref:`algm-PlotPeakByLogValue`.


Data Analysis Interfaces
------------------------

Bugfixes
########

- The MSDFit algorithm now uses the fully specified model given in the interface; previously MSDFit only used the
  model specified in the 'Fit Type' drop-down menu.
