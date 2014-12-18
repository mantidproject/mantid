
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Calculates and applies the lorentz correction weights to a workspace. The Lorentz correction *L* is calculated according to:

.. math:: 
   L = \sin(\theta)^{2}/\lambda^{4}
   
Where :math:`\theta` is the scattering angle.

The calculations performed in this Algorithm are a subset of those performed by the :ref:`algm-AnvredCorrection`

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
   print y[1:5]
   # print first corrected evalues
   print e[1:5]

Output:

.. testoutput:: LorentzCorrectionExample

   [ 0.84604876  0.4213364   1.67862035  0.        ]
   [ 0.59824681  0.4213364   0.83931018  0.        ]

.. categories::

