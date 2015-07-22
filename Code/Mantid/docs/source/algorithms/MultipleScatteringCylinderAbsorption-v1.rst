.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is a port to C++ of a multiple scattering absorption
correction, used to correct the vanadium spectrum for the GPPD
instrument at the IPNS. The correction calculation was originally worked
out by Jack Carpenter and Asfia Huq and implemented in Java by Alok
Chatterjee. The java code was translated to C++ in Mantid by Dennis
Mikkelson.

Usage
-----

**Example: A simple cylindrical sample**

.. testcode:: ExMultipleScatteringCylinderAbsorption
       
    ws = CreateSampleWorkspace("Histogram",NumBanks=1,BankPixelWidth=1)
    ws = ConvertUnits(ws,"Wavelength")
    ws = Rebin(ws,Params=[1])
    SetSampleMaterial(ws,ChemicalFormula="V")

    #restrict the number of wavelength points to speed up the example
    wsOut = MultipleScatteringCylinderAbsorption(ws,CylinderSampleRadius=0.2)
    
    print "Output: ", wsOut.readY(0)

Output:

.. testoutput:: ExMultipleScatteringCylinderAbsorption

    Output:  [  5.90548152   5.90559329  16.28933615   5.90581685   5.90592863
       1.62244888]


References
----------

The source for how this calculation is done is

#. D.F.R. Mildner, J.M. Carpenter, and C.A. Pelizzari *Generalized Attenuation Correction Factor for Scattering from Cylindrical Targets.* Review of Scientific Instruments **45.4** (1974): 572. `doi: 10.1063/1.1686687 <http://dx.doi.org/10.1063/1.1686687>`_

.. categories::

.. sourcelink::
