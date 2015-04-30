.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Calculates and applies corrections for scattering abs absorption in a annular
sample for a run on an indirect inelastic instrument, optionally allowing for
the subtraction or corrections of the container.

The correction factor workspace is a workspace group containing the correction
factors in the Paalman and Pings format, note that only :math:`{A_{s,s}}` and
:math:`A_{c,c}` factors are calculated by thsi algorithm.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Sample corrections for IRIS:**

.. testcode:: SampleCorrectionsWithCanSubtraction

  red_ws = LoadNexusProcessed(Filename='irs26176_graphite002_red.nxs')
  can_ws = LoadNexusProcessed(Filename='irs26173_graphite002_red.nxs')

  corrected, fact = IndirectAnnulusAbsorption(SampleWorkspace=red_ws,
                                              SampleChemicalFormula='H2-O',
                                              CanWorkspace=can_ws,
                                              CanScaleFactor=0.8,
                                              CanInnerRadius=0.19,
                                              SampleInnerRadius=0.2,
                                              SampleOuterRadius=0.25,
                                              CanOuterRadius=0.26,
                                              Events=200)

  ass = fact[0]

  print ('Corrected workspace is intensity against %s'
        % (corrected.getAxis(0).getUnit().caption()))

  print ('Ass workspace is %s against %s'
        % (ass.YUnitLabel(), ass.getAxis(0).getUnit().caption()))

.. testcleanup:: SampleCorrectionsWithCanSubtraction

   DeleteWorkspace(red_ws)
   DeleteWorkspace(can_ws)
   DeleteWorkspace(corrected)
   DeleteWorkspace(fact)

**Output:**

.. testoutput:: SampleCorrectionsWithCanSubtraction

  Corrected workspace is intensity against Energy transfer
  Ass workspace is Attenuation factor against Wavelength

**Example - Sample corrections for IRIS:**

.. testcode:: SampleAndCanCorrections

  red_ws = LoadNexusProcessed(Filename='irs26176_graphite002_red.nxs')
  can_ws = LoadNexusProcessed(Filename='irs26173_graphite002_red.nxs')

  corrected, fact = IndirectAnnulusAbsorption(SampleWorkspace=red_ws,
                                              SampleChemicalFormula='H2-O',
                                              CanWorkspace=can_ws,
                                              CanChemicalFormula='H2-O',
                                              CanInnerRadius=0.19,
                                              SampleInnerRadius=0.2,
                                              SampleOuterRadius=0.25,
                                              CanOuterRadius=0.26,
                                              Events=200,
                                              UseCanCorrections=True)

  ass = fact[0]
  acc = fact[1]

  print ('Corrected workspace is intensity against %s'
        % (corrected.getAxis(0).getUnit().caption()))

  print ('Ass workspace is %s against %s'
        % (ass.YUnitLabel(), ass.getAxis(0).getUnit().caption()))

  print ('Acc workspace is %s against %s'
        % (acc.YUnitLabel(), acc.getAxis(0).getUnit().caption()))

.. testcleanup:: SampleAndCanCorrections

   DeleteWorkspace(red_ws)
   DeleteWorkspace(can_ws)
   DeleteWorkspace(corrected)
   DeleteWorkspace(fact)

**Output:**

.. testoutput:: SampleAndCanCorrections

  Corrected workspace is intensity against Energy transfer
  Ass workspace is Attenuation factor against Wavelength
  Acc workspace is Attenuation factor against Wavelength

.. categories::
