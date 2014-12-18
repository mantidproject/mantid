.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm uses a numerical integration method to calculate
attenuation factors resulting from absorption and single scattering in a
flat plate (slab) sample with the dimensions and material properties
given. Factors are calculated for each spectrum (i.e. detector position)
and wavelength point, as defined by the input workspace. The sample is
divided up into cuboids having sides of as close to the size given in
the ElementSize property as the sample dimensions will allow. Thus the
calculation speed depends linearly on the total number of bins in the
workspace and goes as :math:`\rm{ElementSize}^{-3}`.

Path lengths through the sample are then calculated for the centre-point
of each element and a numerical integration is carried out using these
path lengths over the volume elements.

Restrictions on the input workspace
###################################

The input workspace must have units of wavelength. The
:ref:`instrument <instrument>` associated with the workspace must be fully
defined because detector, source & sample position are needed.


Usage
-----

**Example:**

.. testcode:: ExSimpleSpere
    
    ws = CreateSampleWorkspace("Histogram",NumBanks=1,BankPixelWidth=2)
    ws = ConvertUnits(ws,"Wavelength")
    SetSampleMaterial(ws,ChemicalFormula="V")

    wsOut = FlatPlateAbsorption(ws, SampleHeight=1, SampleWidth=0.5, SampleThickness=0.5)

    print "The created workspace has one entry for each spectra: %i" % wsOut.getNumberHistograms()

Output:

.. testoutput:: ExSimpleSpere

    The created workspace has one entry for each spectra: 4


.. categories::
