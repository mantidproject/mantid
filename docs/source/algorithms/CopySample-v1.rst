.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm copies some/all the sample information from one workspace
to another. For MD input workspaces, if no input sample number is specified, or
not found, it will copy the first sample. For MD output workspaces, if no
output sample number is specified (or negative), it will copy to all
samples. The following information can be copied:

-  Name
-  Material
-  Sample environment
-  Shape
-  Oriented lattice

One can copy the orientation matrix only. To do this, select both
CopyLattice and CopyOrientationOnly. If only CopyOrientationOnly is
true, the algorithm will throw an error.

A common use for this algorithm is for single crystal measurements.
Finding the :ref:`UB matrix <Lattice>` occurs on a :ref:`PeaksWorkspace <PeaksWorkspace>`.
In order to convertthe data to HKL space, one needs to copy the oriented lattice
to the individual data workspaces.

Usage
-----

.. testcode:: CopySample

    #create some workspaces
    peaks=CreateWorkspace(DataX='1,2',DataY='1',WorkspaceTitle='Workspace a')
    SetUB(peaks,a=2,b=3,c=4,alpha=90,beta=90,gamma=90,u="1,0,0",v="0,0,1")
    data=CreateWorkspace(DataX='1,2',DataY='1',WorkspaceTitle='Workspace b')
    SetUB(data,a=1,b=1,c=1,alpha=90,beta=90,gamma=90,u="1,0,0",v="0,1,1")

    #apply algorithm
    CopySample(InputWorkspace=peaks,OutputWorkspace=data,CopyName=0,CopyMaterial=0,CopyEnvironment=0,CopyShape=0,CopyLattice=1)

    #do a quick check
    ol=data.sample().getOrientedLattice()
    print("Data lattice parameters are: {} {} {} {} {} {}".format(
	   ol.a(), ol.b(), ol.c(), ol.alpha(), ol.beta(), ol.gamma()))

.. testcleanup:: CopySample

    DeleteWorkspace('data')
    DeleteWorkspace('peaks')


Output:

.. testoutput:: CopySample

    Data lattice parameters are: 2.0 3.0 4.0 90.0 90.0 90.0


.. categories::

.. sourcelink::
