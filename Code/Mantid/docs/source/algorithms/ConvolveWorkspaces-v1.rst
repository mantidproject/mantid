.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Convolution of two workspaces using `Convolution <http://www.mantidproject.org/Convolution>`__ from
CurveFitting. Workspaces must have the same number of spectra.

Usage
-----

**Example: A simple spherical sample**

.. testcode:: ExConvolveWorkspaces
          

    ws = CreateSampleWorkspace("Histogram",NumBanks=1,BankPixelWidth=1)
    ws = ConvertUnits(ws,"Wavelength")
    ws = Rebin(ws,Params=[1])
    
    #restrict the number of wavelength points to speed up the example
    wsOut = ConvolveWorkspaces(ws,ws)
    
    print "The created workspace has spectra: ", wsOut.readY(0)

Output:

.. testoutput:: ExConvolveWorkspaces

    The created workspace has spectra:  [  60.78539112    0.           32.3478194   121.57078223  175.00835395
  146.57078223]

.. categories::
