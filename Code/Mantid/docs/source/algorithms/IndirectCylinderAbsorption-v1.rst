.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Calculates and applies corrections for scattering and absorption in a
cylindrical sample for a run on an indirect inelastic instrument, optionally
allowing for the subtraction or corrections of the container.

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

  corrected, fact = IndirectCylinderAbsorption(SampleWorkspace=red_ws,
                                               SampleChemicalFormula='H2-O',
                                               CanWorkspace=can_ws,
                                               CanScaleFactor=0.8,
                                               SampleRadius=0.2,
                                               UseCanCorrections=False,
                                               Events=100)

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

**Example - Sample and container corrections for IRIS:**

.. testcode:: SampleCorrectionsWithCanCorrections

  red_ws = LoadNexusProcessed(Filename='irs26176_graphite002_red.nxs')
  can_ws = LoadNexusProcessed(Filename='irs26173_graphite002_red.nxs')

  corrected, fact = IndirectCylinderAbsorption(SampleWorkspace=red_ws,
                                               SampleChemicalFormula='H2-O',
                                               SampleRadius=0.2,
                                               CanWorkspace=can_ws,
                                               CanScaleFactor=0.8,
                                               CanChemicalFormula='V',
                                               CanRadius=0.22,
                                               UseCanCorrections=True,
                                               Events=100)

  ass = fact[0]
  acc = fact[1]

  print ('Corrected workspace is intensity against %s'
        % (corrected.getAxis(0).getUnit().caption()))

  print ('Ass workspace is %s against %s'
        % (ass.YUnitLabel(), ass.getAxis(0).getUnit().caption()))

  print ('Acc workspace is %s against %s'
        % (acc.YUnitLabel(), acc.getAxis(0).getUnit().caption()))

.. testcleanup:: SampleCorrectionsWithCanCorrections

   DeleteWorkspace(red_ws)
   DeleteWorkspace(can_ws)
   DeleteWorkspace(corrected)
   DeleteWorkspace(fact)

**Output:**

.. testoutput:: SampleCorrectionsWithCanCorrections

   Corrected workspace is intensity against Energy transfer
   Ass workspace is Attenuation factor against Wavelength
   Acc workspace is Attenuation factor against Wavelength

.. categories::
