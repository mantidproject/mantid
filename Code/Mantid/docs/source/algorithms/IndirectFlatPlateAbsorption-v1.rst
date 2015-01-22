.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Calculates and applies corrections for scattering abs absorption in a flat plate
sample for a run on an indirect inelastic instrument, optionally also performing
a simple can subtraction is a container workspace is provided.

The corrections workspace (:math:`A_{s,s}`) is the standard Paalman and Pings
attenuation factor for absorption and scattering in the sample.

Usage
-----

**Example - Sample corrections for IRIS:**

.. testcode:: SampleCorrectionsOnly

  red_ws = LoadNexusProcessed(Filename='irs26176_graphite002_red.nxs')

  corrected, ass = IndirectFlatPlateAbsorption(SampleWorkspace=red_ws,
                                               ChemicalFormula='H2-O',
                                               SampleHeight=1,
                                               SampleWidth=1,
                                               SampleThickness=1,
                                               ElementSize=1)

  print ('Corrected workspace is intensity against %s'
        % (corrected.getAxis(0).getUnit().caption()))

  print ('Corrections workspace is %s against %s'
        % (ass.YUnitLabel(), ass.getAxis(0).getUnit().caption()))


.. testcleanup:: SampleCorrectionsOnly

   DeleteWorkspace(red_ws)
   DeleteWorkspace(corrected)
   DeleteWorkspace(ass)

**Output:**


.. testoutput:: SampleCorrectionsOnly

  Corrected workspace is intensity against Energy transfer
  Corrections workspace is Attenuation factor against Wavelength

.. categories::
