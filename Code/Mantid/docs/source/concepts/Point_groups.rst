.. _Point groups:

Point groups
============

This document explains how crystallographic point groups are used in Mantid.

Introduction
------------

Point groups can be used to describe the symmetry of an object or a lattice, as commonly used in the context of diffraction techniques. According to the definition given in the International Tables for Crystallography A, a "point group is a group of symmetry operations all of which leave at least one point unmoved" [ITA6]_. This means that only symmetry operations without a translational component are allowed, which leaves only rotations :math:`1`, :math:`2`, :math:`3`, :math:`4`, :math:`6` and roto-inversions :math:`\bar{1}`, :math:`\bar{3}`, :math:`\bar{4}`, :math:`\bar{6}` and mirror planes :math:`m`.

In three dimensions there are 32 crystallographic point groups and in 11 of these an inversion center (:math:`\bar{1}`) is present. These so called Laue classes are important for diffraction experiments because Friedel's law defines that diffraction patterns always show a center of symmetry if anomalous dispersion is not taken into account.

Through the presence of certain symmetry operations in certain directions, the Laue classes can be categorized into seven crystal systems (see table). This information is included in the Hermann-Mauguin symbol, which describes symmetry along different directions, depending on the crystal system.

.. table:: The seven crystal systems and how they relate to the 11 Laue classes.

    +----------------+-------------------------------------+
    | Crystal system | Laue classes                        |
    +================+=====================================+
    | Cubic          | :math:`m\bar{3}`, :math:`m\bar{3}m` |
    +----------------+-------------------------------------+
    | Hexagonal      | :math:`6/m`, :math:`6/mmm`          |
    +----------------+-------------------------------------+
    | Trigonal       | :math:`\bar{3}`, :math:`\bar{3}m`   |
    +----------------+-------------------------------------+
    | Tetragonal     | :math:`4/m`, :math:`4/mmm`          |
    +----------------+-------------------------------------+
    | Orthorhombic   | :math:`mmm`                         |
    +----------------+-------------------------------------+
    | Monoclinic     | :math:`2/m`                         |
    +----------------+-------------------------------------+
    | Triclinic      | :math:`\bar{1}`                     |
    +----------------+-------------------------------------+
    
As mentioned before, point groups can describe the symmetry of a lattice, including the reciprocal lattice. When working with diffraction data, which are often described in terms of reciprocal lattice vectors with their Miller indices :math:`hkl` (since it's a vector it can be written shortly as :math:`\mathbf{h}`), this is particularly useful. Each symmetry element of the point group transforms a vector :math:`\mathbf{h}` into a new vector :math:`\mathbf{h}'`:

.. math::
    \mathbf{h}' = \mathbf{S}_i \cdot \mathbf{h}
    
In three dimensions :math:`\mathbf{h}` has three elements, so :math:`\mathbf{S}` is a :math:`3\times3`-matrix and the symmetry operation is applied to the vector by matrix-multiplication. Thus, for the purposes currently aimed at in Mantid, each point group can be represented by a collection of :math:`3\times3`-matrices. The number of matrices present in this collection is the so called order :math:`N` of the corresponding point group. Applying all symmetry operations of a point group to a given vector :math:`\mathbf{h}` results in :math:`N` new vectors :math:`\mathbf{h}'`, some of which may be identical (this depends on the symmetry and also on the vectors, e.g. if one or more index is 0). This means that the symmetry operations of a point group generate a set of :math:`N'` (where :math:`N' < N`) non-identical vectors :math:`\mathbf{h}'` for a given vector :math:`\mathbf{h}` - these vectors are called symmetry equivalents.

Using symmetry operations
-------------------------

As described in the introduction, point groups are represented as a collection of symmetry operations, which in turn are represented by :math:`3\times3`-matrices. In order to provide more context to the code using symmetry operations, there is an interface that wraps the :math:`3\times3`-matrix, it's called ``SymmetryOperation``. Besides the matrix this class contains an identifier for the operation, based on the definition in [ITA4]_. It's a symbol that consists of three parts:
    
    1. Symmetry operation in terms of Hermann-Mauguin notation, where :math:`\bar{n}` is denoted as ``-n``.
    2. Direction of the symmetry axis (in case of a mirror plane, the axis perpendicular to the plane) in square brackets, components separated by spaces ``[u v w]``. For :math:`1` and :math:`\bar{1}` the direction must not be present.
    3. ``h`` if the direction refers to a hexagonal coordinate system.
    
Valid examples are ``2 [1 0 0]`` or ``6 [0 0 1]h``. Furthermore, each symmetry operation also has an "order" - it defines how often the operation has to be applied to a vector to generate an identical vector. For a six-fold rotation axis this is 6, for a mirror plane 2.

Using these identifiers, ``SymmetryOperation``-objects can be created through a factory and then used to transform vectors. The following code sample shows how to do that in Python:

.. testcode :: ExSymmetryOperation

    from mantid.geometry import SymmetryOperation, SymmetryOperationFactoryImpl
    
    symOp = SymmetryOperationFactoryImpl.Instance().createSymOp("m [001]")
    
    hkl = [1, -1, 3]
    hklPrime = symOp.apply(hkl)
    
    print "Mirrored hkl:", hklPrime
    
The above code will print the mirrored index:

.. testoutput :: ExSymmetryOperation

    Mirrored hkl: [1,-1,-3]

The corresponding code in C++ looks very similar and usage examples can be found in the code base, mainly in the implementation of ``PointGroup``, which will be the next topic.

Using point groups
------------------

Point groups are represented in Mantid by the ``PointGroup``-interface, which is then implemented for each actual point group. The interface consists of two parts, one for providing information about the point group and one for working with :math:`hkl`-indices. Just as in the case of ``SymmetryOperation``, ``PointGroup``-objects are created using a factory, this time by supplying the short Hermann-Mauguin symbol [#f1]_ :

.. testcode :: ExInformation

    from mantid.geometry import PointGroup, PointGroupFactoryImpl
    
    pg = PointGroupFactoryImpl.Instance().createPointGroup("-1")
    
    print "Name:", pg.getName()
    print "Hermann-Mauguin symbol:", pg.getSymbol()
    print "Crystal system:", pg.crystalSystem()
    
When this code is executed, some information about the point group is printed:
    
.. testoutput :: ExInformation

    Name: -1 (Triclinic)
    Hermann-Mauguin symbol: -1
    Crystal system: Triclinic
    
It's possible to query the factory about available point groups. One option returns a list of all available groups, while another possibility is to get only groups from a certain crystal system:

.. testcode :: ExQueryPointGroups

    from mantid.geometry import PointGroup, PointGroupFactoryImpl
    
    print "All point groups:", PointGroupFactoryImpl.Instance().getAllPointGroupSymbols()
    print "Cubic point groups:", PointGroupFactoryImpl.Instance().getPointGroupSymbols(PointGroup.CrystalSystem.Cubic)
    print "Tetragonal point groups:", PointGroupFactoryImpl.Instance().getPointGroupSymbols(PointGroup.CrystalSystem.Tetragonal)
    
Which results in the following output:

.. testoutput :: ExQueryPointGroups

    All point groups: ['-1','-3','-31m','-3m1','112/m','2/m','4/m','4/mmm','6/m','6/mmm','m-3','m-3m','mmm']
    Cubic point groups: ['m-3','m-3m']
    Tetragonal point groups: ['4/m','4/mmm']

After having obtained a ``PointGroup``-object, it can be used for working with reflection data, more specifically :math:`hkl`-indices. It's possible to check whether two reflections are equivalent in a certain point group:

.. testcode :: ExIsEquivalent

    from mantid.geometry import PointGroup, PointGroupFactoryImpl

    pg = PointGroupFactoryImpl.Instance().createPointGroup("m-3m")

    hkl1 = [2, 0, 0]
    hkl2 = [0, 0, -2]
    hkl3 = [0, 1, 2]

    print "Are [2,0,0] and [0,0,-2] equivalent?", pg.isEquivalent(hkl1, hkl2)
    print "Are [2,0,0] and [0,1,2] equivalent?", pg.isEquivalent(hkl1, hkl3)
    
.. testoutput :: ExIsEquivalent

    Are [2,0,0] and [0,0,-2] equivalent? True
    Are [2,0,0] and [0,1,2] equivalent? False
    
Another common task is to find all symmetry equivalents of a reflection, for example to determine its multiplicity. ``PointGroup`` has a method for this purpose which returns the set of non-identical symmetry equivalents for a given :math:`hkl` (including :math:`hkl` itself):

.. testcode :: ExGetEquivalents

    from mantid.geometry import PointGroup, PointGroupFactoryImpl

    pg = PointGroupFactoryImpl.Instance().createPointGroup("m-3m")

    hkl1 = [2, 0, 0]
    equivalents1 = pg.getEquivalents(hkl1)

    print "Number of reflections equivalent to [2,0,0]:", len(equivalents1)
    print "Equivalents:", equivalents1
    print

    hkl2 = [1, 1, 1]
    equivalents2 = pg.getEquivalents(hkl2)

    print "Number of reflections equivalent to [1,1,1]:", len(equivalents2)
    print "Equivalents:", equivalents2
    
Executing this code results in the following output:
    
.. testoutput :: ExGetEquivalents

    Number of reflections equivalent to [2,0,0]: 6
    Equivalents: [[2,0,0], [0,2,0], [0,0,2], [0,0,-2], [0,-2,0], [-2,0,0]]
    
    Number of reflections equivalent to [1,1,1]: 8
    Equivalents: [[1,1,1], [1,1,-1], [1,-1,1], [1,-1,-1], [-1,1,1], [-1,1,-1], [-1,-1,1], [-1,-1,-1]]
    
Sometimes, a list of reflections needs to be reduced to a set of symmetry independent reflections only. That means it should not contain any two reflections that are symmetry equivalents according to the point group symmetry. To achieve this, ``PointGroup`` offers a method that returns the same :math:`hkl'` for all symmetry equivalents.

.. testcode :: ExIndependentReflections

    from mantid.geometry import PointGroup, PointGroupFactoryImpl

    pg = PointGroupFactoryImpl.Instance().createPointGroup("m-3m")

    hklList = [[1, 0, 0], [0, 1, 0], [-1, 0, 0],    # Equivalent to [1,0,0]
               [1, 1, 1], [-1, 1, 1],               # Equivalent to [1,1,1]
               [-3, 1, 1], [1, -3, 1], [-1, 1, 3]]  # Equivalent to [3,1,1]
		 
    independent = set()

    for hkl in hklList:
    	independent.add(pg.getReflectionFamily(hkl)) # getReflectionFamily returns the same hkl for all symmetry equivalents
	
    print "Number of independent reflections:", len(independent)
    print "Reflections:", list(independent)
    
This example code produces the output below upon execution:

.. testoutput:: ExIndependentReflections

    Number of independent reflections: 3
    Reflections: [[1,1,1], [1,0,0], [3,1,1]]

Again, as in the case of ``SymmetryOperation``, the usage of ``PointGroup`` and the corresponding factory is very similar in C++.

.. [ITA6] International Tables for Crystallography (2006). Vol. A, ch. 10.1, p. 762
.. [ITA4] International Tables for Crystallography, Vol. A, Fourth edition, pp 797-798.

.. [#f1] In the case of the monoclinic Laue class :math:`2/m` it's a bit more complicated, because there are two conventions regarding the unique axis. According to current crystallographic standards, the :math:`b`-axis is used, but in some cases one may find the :math:`c`-axis for this purpose. To resolve this, both options are offered in Mantid. When using the symbol ``2/m``, the :math:`b`-axis convention is used, for :math:`c` one has to explicitly provide the symbol as ``112/m``.

.. categories:: Concepts