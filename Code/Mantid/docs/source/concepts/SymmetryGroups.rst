.. _Symmetry groups:

Symmetry groups
===============

This is an introduction to group theoretical concepts for symmetry in Mantid. It is the basis for further descriptions covering point groups and space groups.

Introduction
------------

Symmetry in space of any dimensionality can be described by symmetry groups. This and the following guides cover symmetry in three dimensions, as it is most commonly used in crystallography and how it is integrated into Mantid. All code samples are given as Python code and can be executed directly in the program.

Groups
------

A group :math:`(G, \cdot)` is defined as a set of elements :math:`G` with a binary operation :math:`\cdot` which combines two elements of :math:`G`. :math:`(G, \cdot)` is a group if the four group axioms are satisfied [Groups]_:

  1. **Closure**: For all :math:`a, b` in :math:`G` the result of :math:`a \cdot b` is also in :math:`G`.
  2. **Associativity**: For all :math:`a, b` in :math:`G`, :math:`(a \cdot b) \cdot c` is equal to :math:`a \cdot (b \cdot c)`.
  3. **Identity element**: There exists an element :math:`e` in :math:`G` such that :math:`e \cdot a = a`.
  4. **Inverse element**: For each :math:`a` in :math:`G` there exists an element :math:`b` in :math:`G` such that :math:`a \cdot b = e`.
   
There are many examples for groups, for example the group defined by the set of signed integer numbers and the addition operation. Any two integers added result again in an integer, integer addition is associative, there is an identity element (the number 0) and for each integer there is an inverse element (the number with the same magnitude, but opposite sign). For describing symmetry, groups are required that consist of a set of symmetry operations and a binary operation that combines them. The next section describes how symmetry operations can be represented and how to work with them in Mantid.

Symmetry operations
-------------------

Theory
~~~~~~

As symmetry operations are used for the definition of space groups, the mathematics behind this are covered in depth in the International Tables for Crystallography A (ITA), namely part 11 [ITASymmetry]_. A symmetry operation describes the transformation of an object :math:`O` into its image :math:`O'`:

.. math::
    O' = S \cdot O
    
For a coordinate :math:`\mathbf{x}` in three dimensions, any transformation can be described using a transformation matrix :math:`\mathbf{W}` and a translation vector :math:`\mathbf{w}`. This leads to the following equation:

.. math::
    \mathbf{x}' = \mathbf{W} \cdot \mathbf{x} + \mathbf{w}
    
:math:`\mathbf{W}` is a :math:`3\times3`-matrix and for affine transformations, the determinant of the matrix is always equal to 1 or -1 (except for hexagonal coordinates, which are handled transparently in Mantid so this is not covered here).

Coordinates :math:`x` are assumed to be fractional in terms of a unit cell, which defines the smallest unit of an infinite lattice that is repeated over and over. For this case, coordinates :math:`x` and :math:`x + 1`, :math:`x + 2`, :math:`-x`, :math:`-x - 1` and so forth are equivalent. That means that translation vectors can be limited to have it's elements on the interval :math:`[0, 1)` - this will be important later on.

Of course it is also possible to transform vectors (such as reciprocal vectors :math:`\mathbf{h}` described by Miller indices h, k and l), in which case the translation does not apply and the matrix needs to be inverted and transposed:

.. math::
    \mathbf{h}' = {\mathbf{W}_i^{-1}}^T \cdot \mathbf{h}

For the definition of the group :math:`(G, \cdot)` this means that :math:`G` is a set of matrix and vector pairs :math:`(\mathbf{W}_i, \mathbf{w}_i) = S_i`. All that's missing for completing the description of a symmetry group is the definition of the binary operation :math:`\cdot`:

.. math::
    S_3 = S_1 \cdot S_2 = \left(\mathbf{W}_1 \cdot \mathbf{W}_2, \left[\mathbf{W}_1 \cdot \mathbf{w}_2\right] + \mathbf{w}_1\right)
    
While matrix and vector pairs are very well suited for modelling symmetry operaitions in a computer language, they are not very convenient for human language. A very common notation is the "Jones-faithful" system, which is for example used in the symmetry operations section in the space group descriptions in ITA. It uses :math:`(x,y,z)` triplets to describe the rows of the matrix (:math:`x` meaning :math:`1, 0, 0` and so on) and the translation vector (for example :math:`x,y,z+1/2` for a translation of half a unit cell vector in :math:`c`-direction). The following table shows some more examples:

.. table:: Examples for symmetry operations in Jones-faithful notation.

    =============== ===================
    Symbol          Symmetry operation
    =============== ===================
    ``x,y,z``       Identity
    ``-x,-y,-z``    Inversion
    ``-x,-y,z``     2-fold rotation around :math:`z`
    ``x,y,-z``      Mirror plane perpendicular to :math:`z`
    ``-x,-y,z+1/2`` :math:`2_1` screw axis along :math:`z`
    =============== ===================
    
There are several advantages to this notation. First of all it's very concise and secondly it directly shows how a point :math:`x` behaves under the symmetry transformation. As such this notation was chosen for generation of symmetry operations in Mantid.

Last but not least, each symmetry operation has a so called order :math:`k`, which describes how many times the operation has to be chained together to arrive at identity:

.. math::
    I = S^k = S \cdot S \cdot \dots \cdot S
    
The same is true for transforming coordinates as well. Applying a symmetry operation of order :math:`k` exactly :math:`k` times to a point :math:`x` will result in :math:`x` again, which is the definition of identity.

Symmetry operations in Mantid
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Symmetry operations are implemented in Mantid following the scheme described above with matrix and vector pairs, it is written in C++ and resides in the `Geometry` library. How to use this code is described in the doxygen documentation and shall not be covered at this point. Instead this section describes how symmetry operations can be used in the Python environment of Mantid.

As described above, the operations are represented using the Jones-faithful notation, so they can be created by a factory that parses these strings. It returns a `SymmetryOperation`-object which can be queried for certain characteristics. The most obvious one is the identifier string. It may differ from the one used to create the operation, because the strings are normalized internally so that a specific matrix and vector pair always has the same identifier.

.. testcode :: ExSymmetryOperationFactory

    from mantid.geometry import SymmetryOperationFactory
    
    # This time the identifier is already normalized
    symOp = SymmetryOperationFactory.createSymOp("x,y,-z")
    print symOp.getIdentifier()
    
    # This is an example for an identifier that changes
    symOp = SymmetryOperationFactory.createSymOp("1/2+x,y,z")
    print symOp.getIdentifier()
    
Executing the above code yields the following output, which shows how the operation identifier is modified in some cases:

.. testoutput :: ExSymmetryOperationFactory

    x,y,-z
    x+1/2,y,z
    
For ease of use with multiple symmetry operations it's also possible to create multiple operations at once, using a semi-colon separated string of identifiers:

.. testcode :: ExSymmetryOperationFactoryMultiple

    from mantid.geometry import SymmetryOperationFactory

    # Create a list of symmetry operations
    symOps = SymmetryOperationFactory.createSymOps("x,y,-z; -x,-y,-z; z,x,y")
    
    print "Number of operations:", len(symOps)
    print "Operations:"
    
    for op in symOps:
	print op.getIdentifier()
	
This prints each identifier on a new line:

.. testoutput :: ExSymmetryOperationFactoryMultiple

    Number of operations: 3
    Operations:
    x,y,-z
    -x,-y,-z
    z,x,y
    
Symmetry operation objects can be used to transform coordinates or Miller indices, which are handled differently as detailed in the theory section above, so different methods exists for each of the two tasks.

.. testcode :: ExSymmetryOperationPoint

    from mantid.geometry import SymmetryOperationFactory
    
    symOp = SymmetryOperationFactory.createSymOp("x-y,x,z")
    
    coordinates = [0.3, 0.4, 0.5]
    coordinatesPrime = symOp.transformCoordinates(coordinates)
    
    print "Transformed coordinates:", coordinatesPrime
    
This script generates a symmetry operation that is used in hexagonal coordinate systems and uses it to transform the given coordinates:

.. testoutput :: ExSymmetryOperationPoint

    Transformed coordinates: [-0.1,0.3,0.5]

As transforming HKLs requires slightly different math, there is a special method for this as well:

.. testcode :: ExSymmetryOperationHKL

    from mantid.geometry import SymmetryOperationFactory
    
    symOp = SymmetryOperationFactory.createSymOp("x,y,-z")
    
    hkl = [1, -1, 3]
    hklPrime = symOp.transformHKL(hkl)
    
    print "Transformed hkl:", hklPrime
    
The above code will print the transformed Miller index triplet:

.. testoutput :: ExSymmetryOperationHKL

    Transformed hkl: [1,-1,-3]
    
It's also possible to query the order of a symmetry operation. The next example generates a fourfold rotation around the :math:`z`-axis and prints some information about it.

.. testcode :: ExSymmetryOperationOrder

    from mantid.geometry import SymmetryOperationFactory

    symOp = SymmetryOperationFactory.createSymOp("-y,x,z")
    
    k = symOp.getOrder()
    print "Order of the symmetry operation:", k

    x = [0.3, 0.4, 0.5]
    print "Original point:",x
    for i in range(k):
        x = symOp.transformCoordinates(x)
        print "After", i + 1, "application(s):", x
      
.. testoutput :: ExSymmetryOperationOrder

    Order of the symmetry operation: 4
    Original point: [0.3, 0.4, 0.5]
    After 1 application(s): [-0.4,0.3,0.5]
    After 2 application(s): [-0.3,-0.4,0.5]
    After 3 application(s): [0.4,-0.3,0.5]
    After 4 application(s): [0.3,0.4,0.5]
    
Symmetry elements
~~~~~~~~~~~~~~~~~

Sometimes it's easier to describe symmetry in terms of the symmetry element that is associated to an operation. Several notation systems exist for these elements, but Hermann-Mauguin symbols are most commonly used in crystallography. Information on how to read these symbols can be found in ITA. Except identity, inversions and translations, all symmetry elements have a characteristic axis. In case of mirror and glide planes, this axis is perpendicular to the plane.

Section 11.2 [ITASymmetry]_ in the same book describes how to derive symmetry elements from matrix and vector pairs. The algorithms from that text are implemented in Mantid as well, so after a symmetry operation has been created using the factory, another factory can be used to generate the symmetry element corresponding to the operation. The resulting object can be queried for its Hermann-Mauguin symbol and its axis. For identity, inversion and translation this returns ``[0, 0, 0]``.

.. testcode :: ExSymmetryElement

    from mantid.geometry import SymmetryOperationFactory, SymmetryElementFactory

    symOp = SymmetryOperationFactory.createSymOp("x,y,-z")
    element = SymmetryElementFactory.createSymElement(symOp)

    print "The element corresponding to 'x,y,-z' has the following symbol:", element.getHMSymbol()
    print "The mirror plane is perpendicular to:", element.getAxis()
    
In this case, it's a mirror plane perpendicular to the :math:`z`-axis:
    
.. testoutput:: ExSymmetryElement

    The element corresponding to 'x,y,-z' has the following symbol: m
    The mirror plane is perpendicular to: [0,0,1]
    
Symmetry groups
---------------

In the previous section, symmetry operations and a binary operation combining them were introduced, which is finally sufficient to define symmetry groups. The most simple group possible contains only one element, the identity:

.. math::
    G = \left\{1\right\}
    
This group fulfills all four group axioms. The identity matrix multiplied with itself is again identity, so the group is closed. Associativity holds as well, since it does not matter in which order multiple identical operations are performed. Since the only element of the group is the identity, the third axiom is fulfilled as well. So is the fourth, since the inverse of the identity is again identity. This group exists as the point group :math:`1` and describes objects that do not show any symmetry except identity.

As more operations are added to a group, it can be useful to display the group in terms of a group table, which makes it easy to check the group axioms. The following example group contains the symmetry operations :math:`1`, :math:`\bar{1}`, :math:`2` and :math:`m`, the latter two being characterized by the same axis. The cells of the group tables show the result of the binary operation combining the symmetry operations in the header row and header column:

.. list-table:: Group table example
    :header-rows: 1
    :stub-columns: 1
    
    * - 
      - :math:`1`
      - :math:`\bar{1}`
      - :math:`2`
      - :math:`m`
    * - :math:`1`
      - :math:`1`
      - :math:`\bar{1}`
      - :math:`2`
      - :math:`m`
    * - :math:`\bar{1}`
      - :math:`\bar{1}`
      - :math:`1`
      - :math:`m`
      - :math:`2`
    * - :math:`2`
      - :math:`2`
      - :math:`m`
      - :math:`1`
      - :math:`\bar{1}`
    * - :math:`m`
      - :math:`m`
      - :math:`2`
      - :math:`\bar{1}`
      - :math:`1`
      
Combining the symmetry operations does not result into any new operations, so the group is closed. Each element has an inverse (:math:`\bar{1}` occurs in each row) and an identity element exists (all elements in the first row are the same as in the header row).

Some groups are so called cyclic groups, all elements of the group can be expressed as powers of one symmetry operation (which are explained above) from 0 to :math:`k-1`, where `k` is the order of the operation. The group with elements :math:`1` and :math:`2` is an example for such a cyclic group, it can be expressed as :math:`2^0 = 1` and :math:`2^1 = 2`.

Just like in the case of symmetry operations, it's also possible to define a binary operation that combines two groups. For this, each symmetry operation of the first group is multiplied with each symmetry operation of the second group. If the resulting new set of operations fulfills the group axioms, the product of the two groups is again a group.

These general group concepts are available in the C++ library of Mantid and are described in the API documentation (`Mantid::Geometry::SymmetryOperation <http://doxygen.mantidproject.org/nightly/d4/d82/classMantid_1_1Geometry_1_1SymmetryOperation.html#details>`_, `Mantid::Geometry::SymmetryElement <http://doxygen.mantidproject.org/nightly/df/d22/classMantid_1_1Geometry_1_1SymmetryElement.html>`_, `Mantid::Geometry::Group <http://doxygen.mantidproject.org/nightly/d3/d80/classMantid_1_1Geometry_1_1Group.html>`_). The most important specializations of symmetry groups implemented in Mantid are point- and space groups. They are explained in an additional :ref:`document <PointAndSpaceGroups>`.

.. [Groups] `Wikipedia article on groups <http://en.wikipedia.org/wiki/Group_%28mathematics%29#Definition>`_. Can be found in different formulations in various places such as `Wolfram MathWorld <http://mathworld.wolfram.com/Group.html>`_.

.. [ITASymmetry] International Tables for Crystallography (2006). Vol. A, part 11, p. 810 (chapters `11.1 <http://it.iucr.org/Ab/ch11o1v0001/>`_ and `11.2 <http://it.iucr.org/Ab/ch11o2v0001/>`_).

.. categories:: Concepts
