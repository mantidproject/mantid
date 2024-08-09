.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Calculates bin-by-bin or event correction factors for attenuation due to
absorption and scattering in a **spherical** sample. Sample data should be
divided by these corrections. Algorithm calls
:ref:`algm-AnvredCorrection`.

Usage
-----

**Example: A simple spherical sample**

.. testcode:: ExSphericalAbsorption

    ws = CreateSampleWorkspace("Histogram",NumBanks=1,BankPixelWidth=1)
    ws = ConvertUnits(ws,"Wavelength")
    ws = Rebin(ws,Params=[1])
    SetSampleMaterial(ws,ChemicalFormula="V")

    #restrict the number of wavelength points to speed up the example
    wsOut = SphericalAbsorption(ws,SphericalSampleRadius=0.2)

    print("The created workspace has spectra: {}".format(wsOut.readY(0)))

Output:

.. testoutput:: ExSphericalAbsorption

    The created workspace has spectra: [0.8693233  0.81842536 0.77068608 0.72590446 0.68389228 0.65847929]

.. categories::

.. sourcelink::
