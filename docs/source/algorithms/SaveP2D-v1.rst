.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Input
#####

This algorithm can be used to create a powder pattern 2d (".p2d") output file as useable for
multidimensional Rietveld refinements.
The input for this algorithm needs to be a 2D workspace containing information about dSpacing and
dSpacingPerpendicular. A 2D workspace can be created using the :ref:`Bin2DPowderDiffraction
<algm-Bin2DPowderDiffraction>` algorithm.
The input values ``removeNaN`` and ``removeNegatives`` control whether intensity values that are negative
or NaN, respectively, are automatically removed from the dataset.
``RemoveNegatives`` also removes intensities equal to zero.
Turning ``cutDdata`` on, allows to cut the measuring data to the specified ranges of
theta, lambda, dSpacing and dSpacingPerpendicular.

Output
######

The output file contains a short comment header giving the title, the instrument parameter file,
the binning parameters of the workspace and the used instrument/detector bank.
Thereafter the measuring data is written into 5 columns, namely, 2theta, lambda, dSpacing,
dSpacingPerpendicular and intensity.

Usage
-----

**Example: Create a ".p2d" file from a 2D Workspace. Remember to change the Filepath for the OutputFile!**

.. testcode:: SaveP2D

	# create a 2D Workspace
    # repeat this block for each spectrum
    xData = [1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0,9.0]		     # d values for one spectrum (one dPerpendicular value)
    yData = ['1','2','3','4']					             # dPerpendicular binedges
    zData = [1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0]		     # intensity values
    eData = [1,1,1,1,1,1,1,1,1]                              # error values

    # used to join all spectra
    xDataTotal = []					                         # d Values for all spectra
    zDataTotal = []					                         # intensity values for all spectra
    eDataTotal = []                                          # error values for all spectra
    nSpec = len(yData)-1                                     # number of spectra

    # Create d and intensity lists for workspace
    for i in range(0,nSpec):
        xDataTotal.extend(xData)	   # extends the list of x values in accordance to the number of spectra used
        zDataTotal.extend(zData)	   # extends the list of intensity values in accordance to the number of spectra used
        eDataTotal.extend(eData)       # extends the list of error values in accordance to the number of spectra used

    # Create a 2D Workspace containing d and dPerpendicular values with intensities
    CreateWorkspace(OutputWorkspace = 'Usage_Example', DataX = xDataTotal, DataY = zDataTotal, DataE = eDataTotal, WorkspaceTitle = 'test', NSpec = nSpec, UnitX = 'dSpacing', VerticalAxisUnit = 'dSpacingPerpendicular', VerticalAxisValues = yData)

    # Save to the users home directory
    file_name = "Usage_Example"
    path = os.path.join(os.path.expanduser("~"), file_name)

    # Create a .p2d file containing the testdata
    SaveP2D(Workspace = "Usage_Example", OutputFile = path, RemoveNaN = False, RemoveNegatives = False, CutData = False)

    # Does the file exist? If it exists, print it!
    path = os.path.join(os.path.expanduser("~"), file_name + '.p2d')
    if os.path.isfile(path):
        with open(path, 'r') as of:
            data = of.readlines()
        for entry in data:
            print(entry[:-1]) # leave out the last character to remove unnecessary newlines

.. testcleanup:: SaveP2D

    os.remove(path)

Output:
The resulting output file (Usage_Example.p2d) looks like this(2theta and lambda get calculated in the algorithm):

.. testoutput:: SaveP2D
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

    Exporting: ...

       0%
      33%
      67%


    Exported: ...
    #Title: test
    #Inst: .prm
    #Binning: ddperp   0.8888889    1.0000000
    #Bank: 1
    #2theta   lambda   d-value   dp-value   counts
      81.3046911      1.3029352      1.0000000      1.5000000      1.0000000
      42.5730378      1.4521280      2.0000000      1.5000000      1.0000000
      28.5401669      1.4789581      3.0000000      1.5000000      1.0000000
      21.4420009      1.4882141      4.0000000      1.5000000      1.0000000
      17.1666094      1.4924723      5.0000000      1.5000000      1.0000000
      14.3112545      1.4947782      6.0000000      1.5000000      1.0000000
      12.2697184      1.4961662      7.0000000      1.5000000      1.0000000
      10.7376523      1.4970660      8.0000000      1.5000000      1.0000000
       9.5455787      1.4976825      9.0000000      1.5000000      1.0000000
     147.7039064      1.9210925      1.0000000      2.5000000      1.0000000
      74.0366222      2.4082809      2.0000000      2.5000000      1.0000000
      48.4687709      2.4628222      3.0000000      2.5000000      1.0000000
      36.1141714      2.4797153      4.0000000      2.5000000      1.0000000
      28.8035116      2.4871957      5.0000000      2.5000000      1.0000000
      23.9632304      2.4911738      6.0000000      2.5000000      1.0000000
      20.5194188      2.4935442      7.0000000      2.5000000      1.0000000
      17.9428625      2.4950714      8.0000000      2.5000000      1.0000000
      15.9421282      2.4961135      9.0000000      2.5000000      1.0000000
     178.1486860      1.9997390      1.0000000      3.5000000      1.0000000
     112.5838945      3.3275045      2.0000000      3.5000000      1.0000000
      70.0240404      3.4424897      3.0000000      3.5000000      1.0000000
      51.4130764      3.4700953      4.0000000      3.5000000      1.0000000
      40.7483187      3.4814929      5.0000000      3.5000000      1.0000000
      33.7894761      3.4873718      6.0000000      3.5000000      1.0000000
      28.8774112      3.4908181      7.0000000      3.5000000      1.0000000
      25.2199975      3.4930167      8.0000000      3.5000000      1.0000000
      22.3888960      3.4945073      9.0000000      3.5000000      1.0000000




.. categories::

.. sourcelink::
