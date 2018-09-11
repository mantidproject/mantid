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

    The created workspace has spectra: [ 0.8451289   0.79101809  0.74254761  0.69867599  0.65861079  0.63477521]

.. categories::

.. sourcelink::
