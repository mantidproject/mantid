.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

PEARL high-pressure, high-temperature measurements rely on the use neutron transmission data to determine the sample
temperature. Typically Hf foil is included with the sample, though others can be used, provided they have well
characterised resonances. Transfit is an algorithm which reads high-energy neutron resonances from the downstream
monitor data on the PEARL instrument. It fits a Voigt function to them to determine the sample temperature. It is
broadly similar to the Fortran77 version originally implemented on PEARL through OpenGenie. As currently coded – this is
only appropriate for use on the PEARL instrument.

Full details of how the temperature information is extracted from the peak shape parameters, can be found in the
references below. Essentially, there are two components contributing to the peak shape, the Gaussian and Lorentzian.
First, the calibration run must be performed to fit the instrument contributions to the line-shape, and account for any
other effects to the line-shape at a given sample pressure. After this, only the Gaussian component is fitted to extract
the temperature. Note that a recalibration is required for each given sample pressure.

References
----------

‘Temperature measurement in a Paris-Edinburgh cell by neutron resonance spectroscopy' - Journal Of Applied Physics 98, 064905 (2005)
'Remote determination of sample temperature by neutron resonance spectroscopy' - Nuclear Instruments and Methods in Physics Research A 547 (2005) 601-615

.. categories::

.. sourcelink::
