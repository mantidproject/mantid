.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Load incident spectrum and detector efficiency correction file
containing spectra for each detector. The spectra are created by
"TOPAZ\_spectrum.py" from files of vanadium or TiZr and background.

Usage
-----

.. testcode:: LoadIsawSpectrum

    #Write a ISAW Spectrum file 
    import mantid    
    filename=mantid.config.getString("defaultsave.directory")+"loadIsawSpectrumTest.dat"  
    f=open(filename,'w') 
    f.write("# Column  Unit    Quantity\n") 
    f.write("# ------  ------  --------\n") 
    f.write("#      1  us      time-of-flight\n") 
    f.write("#      2  counts  counts per us corrected for vanadium rod absorption\n") 
    f.write("#      3  A       wavelength\n") 
    f.write("#      4  counts  counts per us uncorrected for absorption\n") 
    f.write("#      5          transmission\n") 
    f.write("#\n") 
    f.write("Bank 1     DetNum 17\n") 
    f.write("      400.000      -23.878       0.0856      -22.409       0.9385\n") 
    f.write("      401.600      -23.878       0.0859      -22.409       0.9385\n") 
    f.write("      403.200      -23.879       0.0863      -22.409       0.9385\n") 
    f.write("      404.800      -23.879       0.0866      -22.409       0.9385\n") 
    f.write("      406.400      -23.879       0.0870      -22.409       0.9384\n") 
    f.write("      408.000      -21.966       0.0873      -20.614       0.9384\n") 
    f.close() 
         
         
    ow = LoadIsawSpectrum(SpectraFile=filename,InstrumentName="TOPAZ")
         
        #check the results 
    print "x=",ow.readX(0) 
    print "y=",ow.readY(0) 
    
.. testcleanup:: LoadIsawSpectrum

    import os,mantid   
    filename=mantid.config.getString("defaultsave.directory")+"loadIsawSpectrumTest.dat"
    os.remove(filename)

Output:

.. testoutput:: LoadIsawSpectrum

    x= [ 399.2  400.8  402.4  404.   405.6  407.2  408.8]
    y= [-0.00471167 -0.00471167 -0.00471187 -0.00471187 -0.00471187 -0.00433439]

.. categories::
