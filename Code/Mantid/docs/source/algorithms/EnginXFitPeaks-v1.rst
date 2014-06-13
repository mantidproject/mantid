.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

.. warning::

   This algorithm is being developed for a specific instrument. It might get changed or even 
   removed without a notification, should instrument scientists decide to do so.

The pattern is specified by providing a list of dSpacing values where Bragg peaks are expected. The
algorithm then fits peaks in those areas, emitting a dSpacing to TOF conversion coefficients for 
every peak. Those dSpacing to TOF relationships are then fitted to a linear function.

Usage
-----

**Example - Fitting two peaks:**

.. testcode:: ExTwoPeaks

   # Two B2B peaks
   peak1 = "name=BackToBackExponential,I=4000,A=1,B=0.5,X0=12000,S=350"
   peak2 = "name=BackToBackExponential,I=5000,A=1,B=0.7,X0=35000,S=300"

   # Create workpsace with the above peaks and a single detector pixel
   ws = CreateSampleWorkspace(Function="User Defined",
                              UserDefinedFunction=peak1 + ";" + peak2,
                              NumBanks=1,
                              BankPixelWidth=1,
                              XMin=6000,
                              XMax=45000,
                              BinWidth=10)

   # Update instrument geometry to something that would allow converting to some sane dSpacing values
   EditInstrumentGeometry(Workspace = ws, L2 = [1.5], Polar = [90], PrimaryFlightPath = 50)

   # Run the algorithm
   difc, zero = EnginXFitPeaks(ws, 0, [0.6, 1.9])

   # Print the results
   print "Difc:", difc
   print "Zero:", zero

Output:

.. testoutput:: ExTwoPeaks

   Difc: 49019.4561189
   Zero: -58131.0062365

.. categories::
