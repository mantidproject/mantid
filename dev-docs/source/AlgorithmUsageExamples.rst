.. _AlgorithmUsageExamples:

========================
Algorithm Usage Examples
========================

.. contents::
  :local:

Introduction
============

A *usage example* is part of the documentation page of an algorithm.

From a user's point of view, the main purposes of usage examples are:

* Getting started using the algorithm as part of a Python script
* Understanding the algorithm
* Showing hints/comments etc. that help understand Mantid Python scripting in general

The usage examples are written in `reStructuredText <http://docutils.sourceforge.net/rst.html>`__, which can be converted to HTML and the code in the usage examples can be tested. The image below demonstrates an example of converting reStructuredText to HTML. 

Guide
=====

The example below show the proposed way to format an usage example in reStructuredText.

.. code-block:: rest

   Usage
   -----

   **Example - simple rebin of a histogram workspace:**  

   .. testcode:: ExHistSimple

      # create histogram workspace
      dataX = [0,1,2,3,4,5,6,7,8,9] # or use dataX=range(0,10) 
      dataY = [1,1,1,1,1,1,1,1,1] # or use dataY=[1]*9 
      ws = CreateWorkspace(dataX, dataY)
      
      # rebin from min to max with size bin = 2
      ws = Rebin(ws, 2)   
      
      print "The rebinned X values are: " + str(ws.readX(0))  
      print "The rebinned Y values are: " + str(ws.readY(0)) 


   Output:

   .. testoutput:: ExHistSimple
      
      The rebinned X values are: [ 0.  2.  4.  6.  8.  9.]
      The rebinned Y values are: [ 2.  2.  2.  2.  1.]

What is required is:

* Short description explaining the example, formatted as shown above, i.e. using ``**`` to embolden the text.
* A ``.. testcode::`` section, with a unique 'namespace' name, here ``ExHistSimple``. This 'namespace' is not shown when converted to HTML, but is used in testing the code. This section contains the actual Python code demonstrating a usage of the algorithm. This code block contains commented code, finishing with one or more Python ``print`` lines as shown
* A ``.. testoutput::`` section. This section must have a matching 'namespace' name to the ``..testcode::`` section. It simply contains a copy of the text that is printed out in the ``..testcode::`` section.
* Include the "Output:" string above the ``..testoutput::`` directive.

What is optional:

* A ``..testcleanup::`` section. This section must have a matching 'namespace' name to the ``..testcode::`` section. Here, add Python code to do any cleanup of files etc. that were created by the tests. See the notes below for things that are cleaned automatically.

Notes:

* The configuration has a global "testcleanup" implemented which calls ``FrameworkManager::clear()`` to clear algorithms, workspaces & instruments so these are dealt with automatically.
* There must be a clear blank line before the ``.. testcode::`` and ``.. testoutput`` directives or the test is ignored. As with unit tests you should write a failing test first to ensure it is running.

What is worth keeping in mind is:

* *Assume the user is new to Python*. Consider giving hints to more advanced Python in comments, or introduce a simple example first.
* Use comments.
* Use Python ``print`` to output results, which, where possible, helps to understand the algorithm.

A Jenkins job tests that the usage examples are not broken, i.e. that they continue to provide a working demonstration against the current build. It is vital to stress that the purpose of usage testing is *not to replace unit testing* (or system testing). The purpose of usage testing (better described as demonstration examples), is to provide some happy-path examples, which, where this is possible, can assist the user understanding of the Python code. This is very different from the purposes of testing in general, see `here <UnitTestGoodPractice.html>`__.

Additional benefits of usage examples:

* Quick look-up for developers on how to use a certain algorithm in Python scripting
* Allow the user to test that scripts return expected output in their installed Mantid versions
* Additional test coverage of Mantid Python API

Using CreateSampleWorkspace and CreateWorkspace
-----------------------------------------------

There are many ways to create sample workspaces. For example :ref:`CreateMDHistoWorkspace <algm-CreateMDHistoWorkspace>`, :ref:`CreateSampleWorkspace <algm-CreateSampleWorkspace>` and :ref:`CreateWorkspace <algm-CreateWorkspace>`. CreateSampleWorkspace creates a fully defined workspace (either event or histogram) but for creating simple histogram workspace CreateWorkspace may be a better option. Above is shown an example where CreateWorkspace is used. Below is a more complex use of CreateSampleWorkspace:

.. code-block:: rest

   Usage
   -----

   **Example - Fit a Gaussian to a peak in a spectrum:**

   .. testcode:: ExFitPeak

      # create a workspace with a gaussian peak sitting on top of a linear (here flat) background
      ws = CreateSampleWorkspace(Function="User Defined", UserDefinedFunction="name=LinearBackground, \
         A0=0.3;name=Gaussian, PeakCentre=5, Height=10, Sigma=0.7", NumBanks=1, BankPixelWidth=1, XMin=0, XMax=10, BinWidth=0.1)

      # Setup the data to fit:
      workspaceIndex = 0  # the spectrum with which WorkspaceIndex to fit
      startX = 1      # specify fitting region 
      endX = 9      #
      
      # Setup the model, here a Gaussian, to fit to data
      tryCentre = '4'   # A start guess on peak centre
      sigma = '1'          # A start guess on peak width
      height = '8'         # A start guess on peak height
      myFunc = 'name=Gaussian, Height='+height+', PeakCentre='+tryCentre+', Sigma='+sigma
      # here purposely haven't included a linear background which mean fit will not be spot on
      # to include a linear background uncomment the line below
      #myFunc = 'name=LinearBackground, A0=0.3;name=Gaussian, Height='+height+', PeakCentre='+tryCentre+', Sigma='+sigma

      # Do the fitting
      fitStatus, chiSq, covarianceTable, paramTable, fitWorkspace = Fit(InputWorkspace='ws', \ 
         WorkspaceIndex=0, StartX = startX, EndX=endX, Output='fit', Function=myFunc)

      print "The fit was: " + fitStatus  
      print("chi-squared of fit is: %.2f" % chiSq)
      print("Fitted Height value is: %.2f" % paramTable.column(1)[0])
      print("Fitted centre value is: %.2f" % paramTable.column(1)[1]) 
      print("Fitted sigma value is: %.2f" % paramTable.column(1)[2])
      # fitWorkspace contains the data, the calculated and the difference patterns 
      print "Number of spectra in fitWorkspace is: " +  str(fitWorkspace.getNumberHistograms())
      print("The 20th y-value of the calculated pattern: %.4f" % fitWorkspace.readY(1)[19])    

   .. testcleanup:: ExFitPeak

      DeleteWorkspace(ws)

   Output:

   .. testoutput:: ExFitPeak

      The fit was: success
      chi-squared of fit is: 0.14
      Fitted Height value is: 9.79
      Fitted centre value is: 5.05
      Fitted sigma value is: 0.77
      Number of spectra in fitWorkspace is: 3
      The 20th y-value of the calculated pattern: 0.2361

For a more simple use of CreateSampleWorkspace see example below (note if no arguments are given then a histogram workspace is created):

.. code-block:: rest

   Usage
   -----

   **Example - use option PreserveEvents:**

   .. testcode:: ExEventRebin

      # create some event workspace
      ws = CreateSampleWorkspace(WorkspaceType="Event")

      print "What type is the workspace before 1st rebin: " + str(type(ws)) 
      # rebin from min to max with size bin = 2 preserving event workspace (default behaviour)
      ws = Rebin(ws, 2)   
      print "What type is the workspace after 1st rebin: " + str(type(ws)) 
      ws = Rebin(ws, 2, PreserveEvents=False)   
      print "What type is the workspace after 2nd rebin: " + str(type(ws)) 
      # note you can also check the type of a workspace using: print isinstance(ws, IEventWorkspace)   

   .. testcleanup:: ExEventRebin

      DeleteWorkspace(ws)

   Output:

   .. testoutput:: ExEventRebin

      What type is the workspace before 1st rebin: <class 'mantid.api._api.IEventWorkspace'>
      What type is the workspace after 1st rebin: <class 'mantid.api._api.IEventWorkspace'>
      What type is the workspace after 2nd rebin: <class 'mantid.api._api.MatrixWorkspace'>

When needing to load a data file
--------------------------------

Instructions to add a new data file to the repository are available `here <DataFilesForTesting.html>`__. Files from the repository will be bundled up into a .zip file, and this .zip made available for download from the Mantid download page.

If you use files you must add the line

.. code-block:: rest

   .. include:: ../usagedata-note.txt

as shown in the example below. This will generate a note to the user explaining how to download the UsageData.

.. code-block:: rest

   Usage
   -----

   .. include:: ../usagedata-note.txt

   **Example - Load ISIS histogram Nexus file:**
   (see :ref:`LoadISISNexus <algm-LoadISISNexus>` for more options)   

   .. testcode:: ExLoadISISnexusHist

      # Load ISIS LOQ histogram dataset
      ws = Load('LOQ49886.nxs') 
      
      print "The 1st x-value of the first spectrum is: " + str(ws.readX(0)[0])      

   .. testcleanup:: ExLoadISISnexusHist

      DeleteWorkspace(ws)
      
   Output:

   .. testoutput:: ExLoadISISnexusHist
      
      The 1st x-value of the first spectrum is: 5.0

Running the Tests
=================

See `here <DocumentationGuideForDevs.html>`__ for how to run and test the usage examples locally.
