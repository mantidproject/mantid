.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Loads the UB matrix into a workspace from an ISAW-style UB matrix ASCII
file.

You can use the :ref:`algm-SaveIsawUB` algorithm to save to this
format.

The matrix in the file is the transpose of the UB Matrix. The UB matrix
maps the column vector (h,k,l ) to the column vector (q'x,q'y,q'z).
\|Q'\|=1/dspacing and its coordinates are a right-hand coordinate system
where x is the beam direction and z is vertically upward. (IPNS
convention)

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
    print "a=",ol.a()
    print "b=",ol.b()    
    print "c=",ol.c()   
    print "alpha=",ol.alpha()
    print "beta=",ol.beta()
    print "gamma=",ol.gamma() 
    print "The following vectors are in the horizontal plane: ", ol.getuVector(),ol.getvVector()
    
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
