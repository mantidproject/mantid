.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Loads the :ref:`UB matrix <Lattice>` into a workspace from an
ISAW-style :ref:`UB matrix <Lattice>` ASCII file.

You can use the :ref:`algm-SaveIsawUB` algorithm to save to this
format.

The matrix in the file is the transpose of the :ref:`UB matrix
<Lattice>`. The :ref:`UB matrix <Lattice>` maps the column vector
(h,k,l ) to the column vector (q'x,q'y,q'z).  \|Q'\|=1/dspacing and
its coordinates are a right-hand coordinate system where x is the beam
direction and z is vertically upward. (IPNS convention)

Note: for an MDEventWorkspace, all experimentInfo objects will contain
the oriented lattice loaded from the IsawUB file

Usage
-----

.. testcode:: LoadIsawUB

    #Write a ISAW UB file
    import mantid
    filename=mantid.config.getString("defaultsave.directory")+"loadIsawUBTest.mat"
    f=open(filename,'w')
    f.write("0.0  0.5  0.0  \n")
    f.write("0.0  0.0  0.25  \n")
    f.write("0.2  0.0  0.0  \n")
    f.write("2.0  4.0  5.0  90  90  90  40  \n")
    f.write("0.0  0.0  0.0   0   0   0   0  \n")
    f.write("\n\nsome text about IPNS convention")
    f.close()


    w=CreateSingleValuedWorkspace()
    LoadIsawUB(w,filename)

    #check the results
    ol=w.sample().getOrientedLattice()
    print("a= {}".format(ol.a()))
    print("b= {}".format(ol.b()))
    print("c= {}".format(ol.c()))
    print("alpha= {}".format(ol.alpha()))
    print("beta= {}".format(ol.beta()))
    print("gamma= {}".format(ol.gamma() ))
    print("The following vectors are in the horizontal plane:  {} {}".format(ol.getuVector(), ol.getvVector()))

.. testcleanup:: LoadIsawUB

   DeleteWorkspace('w')
   import os,mantid
   filename=mantid.config.getString("defaultsave.directory")+"loadIsawUBTest.mat"
   os.remove(filename)

Output:

.. testoutput:: LoadIsawUB

    a= 2.0
    b= 4.0
    c= 5.0
    alpha= 90.0
    beta= 90.0
    gamma= 90.0
    The following vectors are in the horizontal plane:  [0,0,5] [2,0,0]

.. categories::

.. sourcelink::
