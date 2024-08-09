
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Multiply the input workspace by the calculated Lorentz factor.
The Lorentz correction for time-of-flight single crystal diffraction, ``L``, is calculated according to:

.. math::
   L = \frac{\sin^{2}\theta}{\lambda^{4}}

Where :math:`\theta` is the scattering angle. For time-of-flight powder diffraction it is calculated according to

.. math::
   L = \sin{\theta}

The calculations performed in this Algorithm are a subset of those performed by the :ref:`algm-AnvredCorrection` for single crystal measurements

Usage
-----

**Example - LorentzCorrection**

.. testcode:: LorentzCorrectionExample

   tof = Load(Filename='HRP39180.RAW')
   lam = ConvertUnits(InputWorkspace=tof, Target='Wavelength')
   corrected = LorentzCorrection(InputWorkspace=lam)

   y = corrected.readY(2)
   e = corrected.readE(2)
   # print first corrected yvalues
   print(y[1:5])
   # print first corrected evalues
   print(e[1:5])

Output:

.. testoutput:: LorentzCorrectionExample

   [0.84604876 0.4213364  1.67862035 0.        ]
   [0.59824681 0.4213364  0.83931018 0.        ]

.. categories::

.. sourcelink::
