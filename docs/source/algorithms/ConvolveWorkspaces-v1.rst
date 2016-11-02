.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Convolution of two workspaces using :ref:`Convolution <func-Convolution>` from
CurveFitting. Workspaces must have the same number of spectra.

Usage
-----

**Example: Convolve sample workspaces**

.. testcode:: ExConvolveWorkspaces
          

    ws = CreateSampleWorkspace("Histogram",NumBanks=1,BankPixelWidth=1)
    ws = ConvertUnits(ws,"Wavelength")
    ws = Rebin(ws,Params=[1])
    
    #restrict the number of wavelength points to speed up the example
    wsOut = ConvolveWorkspaces(ws,ws)
    
    print "Output: ", wsOut.readY(0)

Output:

.. testoutput:: ExConvolveWorkspaces

    Output:  [  6.07853911e+01   5.68434189e-14   3.23478194e+01   1.21570782e+02
       1.75008354e+02   1.46570782e+02]

.. categories::

.. sourcelink::
