.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Normalises all spectra of workspace to a specified wavelength. Following
A.J.Schultz's anvred, scales the vanadium spectra.



Usage
-----

.. testcode:: NormaliseVanadium

    
    inst = LoadEmptyInstrument(Filename='IDFs_for_UNIT_TESTING/MINITOPAZ_Definition.xml')
    vanadium = CreateWorkspace(DataX='0,0.5,1,1.5,2,2.5,3,3.5,4,4.5,5', DataY='10.574151,10.873,11.07348,11.22903,11.42286,11.47365,11.37375,11.112,10.512181,10.653397', UnitX='wavelength', ParentWorkspace=inst)
    norm_van = NormaliseVanadium(InputWorkspace=vanadium)
    print "Wavelength = ", norm_van.readX(0)[2], " Y = ", norm_van.readY(0)[2]
    
Output:

.. testoutput:: NormaliseVanadium

    Wavelength =  1.0  Y =  1.00913495012


.. categories::
