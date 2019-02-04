.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Finds spectrum numbers corresponding to reflected and transmission lines
in a line detector Reflectometry dataset.

Expects two or one, reflectometry peaks, will fail if there are more or
less than this number of peaks. The first peak is taken to be from the
reflected line, the second is taken to be from the transmission line.
This algorithm outputs a TableWorkspace containing the spectrum number
of interest.

Usage
-----

**Example:**

.. testcode:: ExFindReflectometryLines

    # a simple workspace
    dataX = [0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1] 
    dataY = [0.1, 1, 0.1, 0.2, 0.1, 2, 0.1] # Real peaks with values 1, 2, false peak with value 0.2
    dataE = [0, 0, 0, 0, 0, 0, 0] 
    nSpec = 7
    ws = CreateWorkspace(DataX=dataX, DataY=dataY, DataE=dataE, NSpec=nSpec, UnitX="Wavelength", 
        VerticalAxisUnit="SpectraNumber")
            
    wsOut = FindReflectometryLines(ws, Version=1)

    for i in range(wsOut.columnCount()):
        print("{} {}".format(wsOut.getColumnNames()[i], wsOut.column(i)[0]))
    
Output:

.. testoutput:: ExFindReflectometryLines

    Reflected Spectrum Number 6
    Transmission Spectrum Number 2

.. categories::

.. sourcelink::
