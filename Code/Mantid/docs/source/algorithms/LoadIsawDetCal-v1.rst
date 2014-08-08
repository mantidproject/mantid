.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Moves the detectors in an instrument using the origin and 2 vectors of
the rotated plane from an ISAW DetCal file.

Usage
-----
 
.. testcode:: LoadIsawDetCal
    
    #Write a ISAW DetCal file 
    import mantid    
    filename=mantid.config.getString("defaultsave.directory")+"loadIsawDetCalTest.DetCal"
    f=open(filename,'w') 
    f.write("5      1    256    256 50.1000 49.9000  0.2000  55.33   50.0000   16.7548  -16.7548  0.00011 -0.00002  1.00000  0.00000  1.00000  0.00000\n")
    f.close() 
             
    iw = LoadEmptyInstrument(Filename="IDFs_for_UNIT_TESTING/MINITOPAZ_Definition.xml",)
    LoadIsawDetCal(InputWorkspace=iw,FileName=filename)
    bank = iw.getInstrument().getComponentByName("bank1")
    print "Position after LoadDetCal :",bank.getPos()
    
.. testcleanup:: LoadIsawDetCal
    
    import os,mantid   
    filename=mantid.config.getString("defaultsave.directory")+"loadIsawDetCalTest.DetCal"
    os.remove(filename)
 
Output:
 
.. testoutput:: LoadIsawDetCal
    
    Position after LoadDetCal : [0.5,0.167548,-0.167548]

.. categories::
