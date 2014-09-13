
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Performs a lorentz correction on a white beam workspace in units of wavelength.

 L = sin\theta^{2}/\lam^{4}


Usage
-----
..  Try not to use files in your examples,
    but if you cannot avoid it then the (small) files must be added to
    autotestdata\UsageData and the following tag unindented
    .. include:: ../usagedata-note.txt

**Example - LorentzCorrection**

.. testcode:: LorentzCorrectionExample

   ws = CreateSampleWorkspace("Histogram",NumBanks=1,BankPixelWidth=1)
   wavelength = ConvertUnits(ws, Target="Wavelength")
   
   corrected = LorentzCorrection(InputWorkspace=wavelength)

   # Print some of the weights. Constant theta angle.
   print "The output workspace has %i spectra" % wsOut.getNumberHistograms()

Output:

.. testoutput:: LorentzCorrectionExample

  The output workspace has ?? spectra

.. categories::

