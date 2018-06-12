.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::


Usage
-----

.. testcode:: Min
    
    #Create a workspace
    CreateWorkspace(OutputWorkspace='w2',
        DataX='1,2,3,4,5,1,2,3,4,5',
        DataY='1,0,5,3,3,2,3,1',  
        DataE='1,2,2,1,1,1,1,1',NSpec='2')
    
    #Find minima
    minim=Min(InputWorkspace='w2')
    print("Minima for spectrum 0 is Y =  {}  and it occurs at X between  {}  and  {}".
           format(minim.dataY(0)[0], minim.dataX(0)[0], minim.dataX(0)[1]))
    print("Minima for spectrum 1 is Y =  {}  and it occurs at X between  {}  and  {}".
          format(minim.dataY(1)[0], minim.dataX(1)[0], minim.dataX(1)[1]))
    
    #Find minima with extra parameters
    minim=Min(InputWorkspace='w2',RangeLower=0,RangeUpper=3,StartWorkspaceIndex =1,EndWorkspaceIndex=1)
    print("The new output workspace has  {}  histogram, with the minimum Y =  {}  " \
          "and it occurs at X between  {}  and  {}".
          format(minim.getNumberHistograms(), minim.dataY(0)[0], minim.dataX(0)[0], minim.dataX(0)[1]))
    
.. testcleanup:: Min

   DeleteWorkspace("w2")
   DeleteWorkspace("minim")


Output:

.. testoutput:: Min
   
    Minima for spectrum 0 is Y =  0.0  and it occurs at X between  2.0  and  3.0
    Minima for spectrum 1 is Y =  1.0  and it occurs at X between  4.0  and  5.0
    The new output workspace has  1  histogram, with the minimum Y =  2.0  and it occurs at X between  2.0  and  3.0

.. categories::

.. sourcelink::
