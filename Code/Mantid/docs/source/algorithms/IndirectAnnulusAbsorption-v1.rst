.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Calculates and applies corrections for scattering abs absorption in a annular
sample for a run on an indirect inelastic instrument, optionally also performing
a simple can subtraction is a container workspace is provided.

The corrections workspace (:math:`A_{s,s}`) is the standard Paalman and Pings
attenuation factor for absorption and scattering in the sample.

Usage
-----

.. include:: ../usagedata-note.txt

.. **Example - Sample corrections for IRIS:**

.. .. testcode:: SampleCorrectionsWithCanSubtraction

..   red_ws = LoadNexusProcessed(Filename='irs26176_graphite002_red.nxs')
..   can_ws = LoadNexusProcessed(Filename='irs26173_graphite002_red.nxs')

..   corrected, ass = IndirectAnnulusAbsorption(SampleWorkspace=red_ws,
..                                              CanWorkspace=can_ws,
..                                              CanScaleFactor=0.8,
..                                              ChemicalFormula='H2-O',
..                                              CanInnerRadius=0.2,
..                                              SampleInnerRadius=0.15,
..                                              SampleOuterRadius=0.16,
..                                              CanOuterRadius=0.22,
..                                              Events=200)

..   print ('Corrected workspace is intensity against %s'
..         % (corrected.getAxis(0).getUnit().caption()))

..   print ('Corrections workspace is %s against %s'
..         % (ass.YUnitLabel(), ass.getAxis(0).getUnit().caption()))


.. .. testcleanup:: SampleCorrectionsWithCanSubtraction

..    DeleteWorkspace(red_ws)
..    DeleteWorkspace(can_ws)
..    DeleteWorkspace(corrected)
..    DeleteWorkspace(ass)

.. **Output:**


.. .. testoutput:: SampleCorrectionsWithCanSubtraction

..   Corrected workspace is intensity against Energy transfer
..   Corrections workspace is Attenuation factor against Wavelength

.. categories::
