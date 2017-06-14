.. _Point and space groups:

Point and space groups
======================

This document explains how crystallographic point and space groups are used in Mantid. The :ref:`document <Symmetry groups>` about symmetry operations, elements and groups is a prerequisite for this text, so it is recommended to read it beforehand.

Introduction
------------

As explained in the general text covering symmetry groups, groups are used to describe symmetry of objects in space. For crystallography, point and space groups are essential tools to describe crystal structures and aspects of diffraction experiments. According to the definition given in the International Tables for Crystallography A, a "point group is a group of symmetry operations all of which leave at least one point unmoved" [ITAPointGroups]_. This means that only symmetry operations without a translational component are allowed, which leaves only rotations :math:`1`, :math:`2`, :math:`3`, :math:`4`, :math:`6` and roto-inversions :math:`\bar{1}`, :math:`\bar{3}`, :math:`\bar{4}`, :math:`\bar{6}` and mirror planes :math:`m`. In space groups, translational symmetry is present as well (for example in the form of screw axes and glide planes).

Theory
------

In three dimensions there are 32 crystallographic point groups and in 11 of these an inversion center (:math:`\bar{1}`) is present. These so called Laue classes are important for diffraction experiments because Friedel's law defines that diffraction patterns always show a center of symmetry if anomalous dispersion is not taken into account.

Through the presence of certain symmetry operations in certain directions, the Laue classes (and also the point groups) can be categorized into seven crystal systems (see table below). This information is included in the Hermann-Mauguin symbol, which describes symmetry along different directions, depending on the crystal system.

.. table:: The seven crystal systems and how they relate to the 11 Laue classes and the 32 crystallographic point groups

    +----------------+-------------------------------------+------------------------------------------------------------------------------------------------------------------------+
    | Crystal system | Laue classes                        | Point groups                                                                                                           |
    +================+=====================================+========================================================================================================================+
    | Cubic          | :math:`m\bar{3}`, :math:`m\bar{3}m` | :math:`23`, :math:`m\bar{3}`, :math:`432`, :math:`\bar{4}3m`, :math:`m\bar{3}m`                                        |
    +----------------+-------------------------------------+------------------------------------------------------------------------------------------------------------------------+
    | Hexagonal      | :math:`6/m`, :math:`6/mmm`          | :math:`6`, :math:`\bar{6}`, :math:`6/m`, :math:`622`, :math:`6mm`, :math:`\bar{6}2m`, :math:`\bar{6}m2`, :math:`6/mmm` |
    +----------------+-------------------------------------+------------------------------------------------------------------------------------------------------------------------+
    | Trigonal       | :math:`\bar{3}`, :math:`\bar{3}m`   | :math:`3`, :math:`\bar{3}`, :math:`321`, :math:`312`, :math:`3m1`, :math:`31m`, :math:`\bar{3}m1`, :math:`\bar{3}1m`   |
    +----------------+-------------------------------------+------------------------------------------------------------------------------------------------------------------------+
    | Tetragonal     | :math:`4/m`, :math:`4/mmm`          | :math:`4`, :math:`\bar{4}`, :math:`4/m`, :math:`422`, :math:`4mm`, :math:`\bar{4}2m`, :math:`\bar{4}m2`, :math:`4/mmm` |
    +----------------+-------------------------------------+------------------------------------------------------------------------------------------------------------------------+
    | Orthorhombic   | :math:`mmm`                         | :math:`222`, :math:`mm2`, :math:`mmm`                                                                                  |
    +----------------+-------------------------------------+------------------------------------------------------------------------------------------------------------------------+
    | Monoclinic     | :math:`2/m`                         | :math:`2`, :math:`m`, :math:`2/m`                                                                                      |
    +----------------+-------------------------------------+------------------------------------------------------------------------------------------------------------------------+
    | Triclinic      | :math:`\bar{1}`                     | :math:`1`, :math:`\bar{1}`                                                                                             |
    +----------------+-------------------------------------+------------------------------------------------------------------------------------------------------------------------+

Any point group can be generated by using a maximum of three symmetry operations as so-called generators using the principle described in [Shmueli84]_. According to this, any point group can be described either as a cyclic group or as a product of maximum three cyclic groups. The point group :math:`4` is an example for a group that is simply a cyclic group generated from a positive four fold rotation axis along the :math:`z`-axis, which has the order 4.

.. math::
  G = \left\{{4^{+}}^0, {4^{+}}^1, {4^{+}}^2, {4^{+}}^3\right\} = \left\{1, 4^{+}, 2, 4^{-}\right\}


So by specifying one single symmetry operation as generator, all symmetry operations of the point group in question are generated. From this it's convenient to expand the example to generate a point group that can be expressed as the product of two cyclic groups - :math:`4/m`. In this point group, an additional mirror plane is present perpendicular to the four fold axis. The point group can be expressed as product of :math:`4` shown above and a cyclic group generated by the symmetry operation :math:`m` (mirror plane perpendicular to :math:`z`):

.. math::
  G' = G \cdot \left\{m^0, m^1\right\} = \left\{1, 4^{+}, 2, 4^{-}\right\} \cdot \left\{1, m\right\} = \left\{1, m, 4^{+}, \bar{4}^{+}, 2, \bar{1}, 4^{-}, \bar{4}^{-}\right\}


This means that :math:`4/m` contains an inversion center as well as a four fold rotoinversion axis which result from the combination of the operations of the two cyclic groups. It's also possible to use a different cyclic group to achive the same result (:math:`\bar{1}`). As mentioned above, for some point groups it's necessary to use three generators, which follows the same principle and is not shown here.

Space groups can be handled in a very similar way if translations are limited to the interval :math:`[0, 1)` so that screw-axes and glide-planes can also be used to generate cyclic groups. Without this limitation, the translational components would not be the same for :math:`S^k` and :math:`S^0`.

Using point groups in Mantid
----------------------------

Point groups are represented in Mantid by the ``PointGroup``-class, which is constructed for each actual point group using generators. The interface of the class consists of two parts, one for providing information about the point group and one for working with :math:`hkl`-indices. Just as in the case of ``SymmetryOperation``, ``PointGroup``-objects are created using a factory, this time by supplying the short Hermann-Mauguin symbol [#f1]_ :

.. testcode :: ExInformation

    from mantid.geometry import PointGroupFactory

    pg = PointGroupFactory.createPointGroup("-1")

    print "Name:", pg.getName()
    print "Hermann-Mauguin symbol:", pg.getHMSymbol()
    print "Crystal system:", pg.getCrystalSystem()

When this code is executed, some information about the point group is printed:

.. testoutput :: ExInformation

    Name: -1 (Triclinic)
    Hermann-Mauguin symbol: -1
    Crystal system: Triclinic

It's possible to query the factory about available point groups. One option returns a list of all available groups, while another possibility is to get only groups from a certain crystal system:

.. testcode :: ExQueryPointGroups

    from mantid.geometry import PointGroupFactory, PointGroup

    print "All point groups:", PointGroupFactory.getAllPointGroupSymbols()
    print "Cubic point groups:", PointGroupFactory.getPointGroupSymbols(PointGroup.CrystalSystem.Cubic)
    print "Tetragonal point groups:", PointGroupFactory.getPointGroupSymbols(PointGroup.CrystalSystem.Tetragonal)

Which results in the following output:

.. testoutput :: ExQueryPointGroups

    All point groups: ['-1','-3','-3 r','-31m','-3m','-3m r','-3m1','-4','-42m','-43m','-4m2','-6','-62m','-6m2','1','112','112/m','11m','2','2/m','222','23','2mm','3','3 r','312','31m','32','32 r','321','3m','3m r','3m1','4','4/m','4/mmm','422','432','4mm','6','6/m','6/mmm','622','6mm','m','m-3','m-3m','m2m','mm2','mmm']
    Cubic point groups: ['-43m','23','432','m-3','m-3m']
    Tetragonal point groups: ['-4','-42m','-4m2','4','4/m','4/mmm','422','4mm']

The point groups with an extra ``r`` at the end are trigonal point groups with rhombohedral axes. Trigonal point groups without that additional letter use the hexagonal coordinate system. For some of them there are two different axis choices, for example :math:`\bar{3}m`, which can be defined as :math:`\bar{3}m1` or :math:`\bar{3}1m`. Creating it by the symbol ``-3m`` defaults to :math:`\bar{3}m1`.

After having obtained a ``PointGroup``-object, it can be used for working with reflection data, more specifically :math:`hkl`-indices. It's possible to check whether two reflections are equivalent in a certain point group:

.. testcode :: ExIsEquivalent

    from mantid.geometry import PointGroupFactory

    pg = PointGroupFactory.createPointGroup("m-3m")

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

    from mantid.geometry import PointGroupFactory

    pg = PointGroupFactory.createPointGroup("m-3m")

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

    from mantid.geometry import PointGroupFactory

    pg = PointGroupFactory.createPointGroup("m-3m")

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

Symmetry imposes restrictions on the metric of the unit cell. Cubic symmetry for example implies that all cell edges have the same length and all angles are 90 degrees. The ``Group``-class (and thus, by inheritance also ``PointGroup``) provides a method that checks is a metric tensor is compatible with the symmetry operations of the group:

.. testcode:: ExPointGroupIsInvariant

    from mantid.geometry import PointGroupFactory, UnitCell

    cell = UnitCell(3, 3, 5)

    pgCubic = PointGroupFactory.createPointGroup("m-3m")
    print "Is the cell compatible with cubic symmetry?", pgCubic.isInvariant(cell.getG())

    pgTetragonal = PointGroupFactory.createPointGroup("4/mmm")
    print "Is the cell compatible with tetragonal symmetry?", pgTetragonal.isInvariant(cell.getG())

Executing the code above will produce the following output that reveals that the cell is only compatible with tetragonal, but not with cubic symmetry:

.. testoutput:: ExPointGroupIsInvariant

    Is the cell compatible with cubic symmetry? False
    Is the cell compatible with tetragonal symmetry? True

The ``SpaceGroup`` class described below provides a convenience method that takes a unit cell object directly.

This is all that's covered by the Python interface regarding point groups in Mantid at the time of this writing. The use in C++ is very similar and described in detail in the API documentation.

Using space groups in Mantid
----------------------------

Space group representation in Mantid is very similar to the point group representation, there is a ``SpaceGroup``-class that contains some information about the group, but also a method to generate equivalent positions from a coordinate triplet.

Exactly like point groups, space groups are also created using a factory:

.. testcode:: ExSpaceGroupInfo

    from mantid.geometry import SpaceGroupFactory

    sg = SpaceGroupFactory.createSpaceGroup("P -1")
    print "Hermann-Mauguin symbol:", sg.getHMSymbol()
    print "ITA number:", sg.getNumber()

Executing this code shows the Hermann-Mauguin symbol of the space group as well as the number defined in ITA:

.. testoutput:: ExSpaceGroupInfo

    Hermann-Mauguin symbol: P -1
    ITA number: 2

For some space group types there is more than one setting, so the factory supports querying the available symbols for a given number:

.. testcode:: ExSpaceGroupNumber

    from mantid.geometry import SpaceGroupFactory

    print "Space group no. 26:", SpaceGroupFactory.subscribedSpaceGroupSymbols(26)
    print "Total subscribed space group types:", len(SpaceGroupFactory.getAllSpaceGroupNumbers())

This shows all 6 permutations of the orthorhombic space group no. 26, and that there are 230 space group types:

.. testoutput:: ExSpaceGroupNumber

    Space group no. 26: ['P m c 21','P c m 21','P 21 m a','P 21 a m','P b 21 m','P m 21 b']
    Total subscribed space group types: 230

Besides containing some very basic information, the most important functionality is the ability to generate equivalent coordinates. The following example generates all coordinates for the :math:`4h` position in space group :math:`P6/m` (No. 175):

.. testcode:: ExEquivalentCoordinates

    from mantid.geometry import SpaceGroupFactory

    sg = SpaceGroupFactory.createSpaceGroup("P 6/m")

    position = [1./3., 2./3., 0.25]
    equivalents = sg.getEquivalentPositions(position)

    print "There are", len(equivalents), "equivalent coordinates."
    print "Coordinates:"
    for i, pos in enumerate(equivalents):
        print str(i + 1) + ":", pos

Please note that for hexagonal and trigonal space groups, where translations of :math:`1/3`, :math:`2/3`, :math:`1/6` and so on are common, these coordinates must be supplied either as ``1./3.`` or with a precision of 5 digits, e.g. ``0.66667``.

.. testoutput:: ExEquivalentCoordinates

    There are 4 equivalent coordinates.
    Coordinates:
    1: [0.333333,0.666667,0.25]
    2: [0.333333,0.666667,0.75]
    3: [0.666667,0.333333,0.25]
    4: [0.666667,0.333333,0.75]

Closely related to the generation of equivalent coordinates is the site symmetry group, which leaves a point unchanged:

.. testcode:: ExSiteSymmetryGroupInBuilt

    from mantid.geometry import SpaceGroupFactory, SymmetryElementFactory, SymmetryElement

    def getFullElementSymbol(symmetryElement):
    # Dictionary for mapping enum values to short strings
        rotationSenseDict = {
                                SymmetryElement.RotationSense.Positive: '+',
                                SymmetryElement.RotationSense.Negative: '-',
                                SymmetryElement.RotationSense.NoRotation: ''
                            }
        hmSymbol = element.getHMSymbol()
        rotationSense = rotationSenseDict[element.getRotationSense()]
        axis = str(element.getAxis())

        return hmSymbol + rotationSense + ' ' + axis



    sg = SpaceGroupFactory.createSpaceGroup("P 6/m")

    position = [1./3., 2./3., 0.25]
    siteSymmetryGroup = sg.getSiteSymmetryGroup(position)

    print "Order of the site symmetry group:", siteSymmetryGroup.getOrder()
    print "Group elements:"
    for i, op in enumerate(siteSymmetryGroup.getSymmetryOperations()):
        element = SymmetryElementFactory.createSymElement(op)
        print str(i + 1) + ":", op.getIdentifier(), "(" + getFullElementSymbol(element) + ")"

The group contains three symmetry operations:

.. testoutput:: ExSiteSymmetryGroupInBuilt

    Order of the site symmetry group: 3
    Group elements:
    1: -x+y,-x,z (3- [0,0,1])
    2: -y,x-y,z (3+ [0,0,1])
    3: x,y,z (1 [0,0,0])

An extended example below shows an algorithm to derive the site symmetry group.

Furthermore, it is possible to create a PointGroup-object from a SpaceGroup object in order to obtain information about the crystal system and to perform the Miller index operations provided by PointGroup. For this, PointGroupFactory has a special method, but the point group can also be conveniently created directly from the space group object:

.. testcode:: ExPointGroupFromSpaceGroup

    from mantid.geometry import PointGroupFactory, SpaceGroupFactory

    # Create space group Fd-3m (for example silicon or diamond)
    sg_diamond = SpaceGroupFactory.createSpaceGroup("F d -3 m")
    pg_diamond = PointGroupFactory.createPointGroupFromSpaceGroup(sg_diamond)

    print "Space group no.", sg_diamond.getNumber(), "has point group:", pg_diamond.getHMSymbol()

    # Related space group F-43m (sphalerite)
    sg_zincblende = SpaceGroupFactory.createSpaceGroup("F -4 3 m")
    pg_zincblende = sg_zincblende.getPointGroup()

    print "Space group no.", sg_zincblende.getNumber(), "has point group:", pg_zincblende.getHMSymbol()

The script prints the point group of the space group in question:

.. testoutput:: ExPointGroupFromSpaceGroup

    Space group no. 227 has point group: m-3m
    Space group no. 216 has point group: -43m

Sometimes it's useful to reverse the above process - which is not exactly possible, because several space groups may map to the same point group. The space group factory does however provide a way to get all space group symbols that belong to a certain point group:

.. testcode:: ExSpaceGroupFactoryPointGroup

    from mantid.geometry import PointGroupFactory, SpaceGroupFactory

    pg = PointGroupFactory.createPointGroup("m-3")

    print "Space groups with point group m-3:", SpaceGroupFactory.getSpaceGroupsForPointGroup(pg)

The example produces the following output:

.. testoutput:: ExSpaceGroupFactoryPointGroup

    Space groups with point group m-3: ['F d -3','F d -3 :2','F m -3','I a -3','I m -3','P a -3','P m -3','P n -3','P n -3 :2']

While PointGroup offers useful methods to handle reflections, some information can only be obtained from the space group. The presence of translational symmetry causes the contributions from symmetrically equivalent atoms to the structure factor of certain reflections to cancel out completely so that it can not be observed. These systematically absent reflections are characteristic for each space group, a fact that can be used to determine the space group from measured reflection intensities. The following script shows how to check a few reflections:

.. testcode:: ExSpaceGroupReflectionIsAllowed

    from mantid.kernel import V3D
    from mantid.geometry import SpaceGroupFactory

    sg = SpaceGroupFactory.createSpaceGroup("F d d d")

    hkls = [V3D(0, 0, 2), V3D(0, 0, 4), V3D(0, 0, 6), V3D(0, 0, 8)]

    for hkl in hkls:
        print hkl, "is allowed:", sg.isAllowedReflection(hkl)

Because space group :math:`Fddd` contains diamond glide planes, only :math:`00l` reflections with :math:`l=4n` are allowed. The script gives the correct answer for these reflections:

.. testoutput:: ExSpaceGroupReflectionIsAllowed

    [0,0,2] is allowed: False
    [0,0,4] is allowed: True
    [0,0,6] is allowed: False
    [0,0,8] is allowed: True

:ref:`Below <SpaceGroupCheck>` is a more elaborate example which shows one possibility to find a likely candidate space group for a list of reflections. Please note that these reflection conditions only covers the ones listed for the "general position" in ITA. When atoms are located on special positions, there may be additional conditions that need to be fulfilled. A notable example is the :math:`222`-reflection in Silicon. It is forbidden because the silicon atom is located on the :math:`8a` position, which introduces additional reflection conditions.

As mentioned above, ``SpaceGroup`` provides a function that verifies whether the metric of a unit cell is compatible with the space group's symmetry:

.. testcode:: ExSpaceGroupIsAllowedUnitCell

    from mantid.geometry import SpaceGroupFactory, UnitCell

    # An arbitrary cell with hexagonal metric
    cell = UnitCell(4.402, 4.402, 10.0, 90, 90, 120)

    sgR3mRh = SpaceGroupFactory.createSpaceGroup("R -3 m :r")
    print "Is the cell allowed in R-3m (rhombohedral setting)?", sgR3mRh.isAllowedUnitCell(cell)

    sgR3mHex = SpaceGroupFactory.createSpaceGroup("R -3 m")
    print "Is the cell allowed in R-3m (hexagonal setting)?", sgR3mHex.isAllowedUnitCell(cell)

The code above shows that the defined cell is only compatible with space group :math:`R\bar{3}m` in hexagonal setting:

.. testoutput:: ExSpaceGroupIsAllowedUnitCell

    Is the cell allowed in R-3m (rhombohedral setting)? False
    Is the cell allowed in R-3m (hexagonal setting)? True

The method uses a tolerance of :math:`10^{-8}` for comparison of the metric tensor and its transform. For more fine-grained control of the tolerance it is possible to use the ``isInvariant``-method and supply the metric tensor along with the desired tolerance as a second parameter.

Very similar constructions are available in C++ as well, as shown in the API documentation.

Other ways of using groups in Mantid
------------------------------------

Retrieving information about space group symmetry
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The previous two sections demonstrated how to perform common tasks using point and space groups in Mantid. With the available Python tools it is however possible to obtain other information as well. One useful method that both PointGroup and SpaceGroup expose is to query the symmetry operations of the group, although in string format:

.. testcode:: ExGroupSymmetryOperationStrings

    from mantid.geometry import SpaceGroupFactory

    sg = SpaceGroupFactory.createSpaceGroup("P 6/m")
    symOpStrings = sorted(sg.getSymmetryOperationStrings())

    print "There are", len(symOpStrings), "symmetry operations in space group", sg.getHMSymbol() + "."
    print "Symmetry operations:", symOpStrings

Which prints the symmetry operation information:

.. testoutput:: ExGroupSymmetryOperationStrings

    There are 12 symmetry operations in space group P 6/m.
    Symmetry operations: ['-x+y,-x,-z', '-x+y,-x,z', '-x,-y,-z', '-x,-y,z', '-y,x-y,-z', '-y,x-y,z', 'x,y,-z', 'x,y,z', 'x-y,x,-z', 'x-y,x,z', 'y,-x+y,-z', 'y,-x+y,z']

While this can be interesting for informational purposes, it's more useful to obtain the symmetry operations directly as objects to use them for new purposes. This script for example sorts the contained symmetry operations according to their order:

.. testcode:: ExGroupSymmetryOperations

    from mantid.geometry import SpaceGroupFactory

    def getMaximumOrderOperation(spaceGroup):
        return sorted(spaceGroup.getSymmetryOperations(), key=lambda x: x.getOrder())[-1]

    sg1 = SpaceGroupFactory.createSpaceGroup("P 6/m")
    sg2 = SpaceGroupFactory.createSpaceGroup("P 4 3 2")

    # Get the symmetry operation with the highest order
    symOpMax1 = getMaximumOrderOperation(sg1)
    symOpMax2 = getMaximumOrderOperation(sg2)

    print "The symmetry operation with highest order in space group no.", sg1.getNumber(), "is:", symOpMax1.getIdentifier(), "(k=" + str(symOpMax1.getOrder()) + ")"
    print "The symmetry operation with highest order in space group no.", sg2.getNumber(), "is:", symOpMax2.getIdentifier(), "(k=" + str(symOpMax2.getOrder()) + ")"

Which produces the following output:

.. testoutput:: ExGroupSymmetryOperations

    The symmetry operation with highest order in space group no. 175 is: y,-x+y,z (k=6)
    The symmetry operation with highest order in space group no. 207 is: z,y,-x (k=4)

Another way to extract more information about the symmetry in a space group is to obtain the symmetry elements and arrange them by their characteristic axis:

.. testcode:: ExGroupSymmetryElements

    from mantid.kernel import V3D
    from mantid.geometry import PointGroupFactory, SpaceGroupFactory, SymmetryElementFactory

    def getSymmetryElementsFromOperations(symmetryOperations):
        return [SymmetryElementFactory.createSymElement(x) for x in symmetryOperations]

    sg = SpaceGroupFactory.createSpaceGroup("P n m a")
    pg = PointGroupFactory.createPointGroupFromSpaceGroup(sg)

    symElements = getSymmetryElementsFromOperations(sg.getSymmetryOperations())
    symElementsByAxis = {}
    symElementsNoAxis = []

    for symElem in symElements:
        axis = pg.getReflectionFamily(symElem.getAxis())
        #axis = symElem.getAxis()

        # If axis is [0,0,0], put the element into the "no axis" list
        if axis == V3D(0, 0, 0):
            symElementsNoAxis.append(symElem)
        else:
            # Otherwise check if that axis is already in the dictionary with a list...
            if axis in symElementsByAxis.keys():
                symElementsByAxis[axis].append(symElem)
            # ...or create a new list for that axis
            else:
                symElementsByAxis[axis] = [symElem]

    noAxisSymbols = [x.getHMSymbol() for x in symElementsNoAxis]
    print "There are", len(symElementsNoAxis), "element(s) with no characteristic axis."
    print "Are there translations?", "Yes" if 't' in noAxisSymbols else "No"

    axes = symElementsByAxis.keys()
    print "There is a total of", len(axes), "different characteristic axes."
    print "Symmetry in each direction:"

    for axis in sorted(axes):
        print str(axis) + ": ", sorted([x.getHMSymbol() for x in symElementsByAxis[axis]])

This prints the following information:

.. testoutput:: ExGroupSymmetryElements

    There are 2 element(s) with no characteristic axis.
    Are there translations? No
    There is a total of 3 different characteristic axes.
    Symmetry in each direction:
    [0,0,1]:  ['21', 'a']
    [0,1,0]:  ['21', 'm']
    [1,0,0]:  ['21', 'n']

Looking up space group number 62 (:math:`Pnma` from the example) in ITA shows that the full Hermann-Mauguin symbol for that space group is :math:`P 2_1/n 2_1/m 2_1/a`. The short script gives us all of this information, since there are no translations (the primitive lattice translations are implicit) it must be a primitive lattice (:math:`P`) and all directions encoded in the HM-symbol contain a :math:`2_1` screw axis perpendicular to a glide or mirror plane.

Extracting the site symmetry group
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

With the space group information it's also possible to derive information about site symmetry at specific coordinates and construct the site symmetry group, which is the sub-group of the point group that contains the symmetry operations of the space group that leave the point unchanged. In the following script, the site symmetry group of the :math:`6h` position (coordinates :math:`x, 2x, 1/4`) in space group :math:`P6_3/mmc` (no. 194) is determined:

.. testcode:: ExSiteSymmetryGroup

    from mantid.kernel import V3D
    from mantid.geometry import SpaceGroupFactory, Group
    import numpy as np

    # Function that transforms coordinates to the interval [0, 1)
    def getWrappedCoordinates(coordinates):
        tmp = coordinates + V3D(1, 1, 1)
        return V3D(np.fmod(tmp.X(), 1.0), np.fmod(tmp.Y(), 1.0), np.fmod(tmp.Z(), 1.0))

    # Function that construct the site symmetry group
    def getSiteSymmetryGroup(spaceGroup, point):
        symOps = spaceGroup.getSymmetryOperations()

        ops = []
        for op in symOps:
            transformed = getWrappedCoordinates(op.transformCoordinates(point))

            # If the transformed coordinate is equivalent to the original, add it to the group
            if transformed == point:
                ops.append(op)

        # Return group with symmetry operations that leave point unchanged
        return Group(ops)

    # Construct space group object
    sg = SpaceGroupFactory.createSpaceGroup("P 63/m m c")

    # Point on 6h-position, [x, 2x, 1/4]
    point = V3D(0.31, 0.62, 0.25)

    siteSymmGroup = getSiteSymmetryGroup(sg, point)

    print "Site symmetry group fulfills group axioms:", siteSymmGroup.isGroup()
    print "Order of site symmetry group:", siteSymmGroup.getOrder()
    print "Order of space group:", sg.getOrder()
    print "Site multiplicity:", sg.getOrder() / siteSymmGroup.getOrder()

The script produces the following output:

.. testoutput:: ExSiteSymmetryGroup

    Site symmetry group fulfills group axioms: True
    Order of site symmetry group: 4
    Order of space group: 24
    Site multiplicity: 6

There are four symmmetry operations that leave the coordinates :math:`x,2x,1/4` unchanged, they fulfill the group axioms. Dividing the order of the space group by the order of the site symmetry group gives the correct site multiplicity 6.

.. _SpaceGroupCheck:

Checking a list of unique reflections for possible space groups
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Building on the example above which showed how to check whether a reflection is allowed by the symmetry operations contained in the space group, the next script goes a bit further. The starting point is a list of symmetry independent reflections with the information whether it's observed or not. A list like can usually be obtained at some point during data reduction after intensities have been determined, which allows for the derivation of the Laue class (and thus, merging the reflections so only unique reflections are available) and decision if a reflection is observed or not (for example :math:`I/\sigma(I) > 3`). Covering all these steps is beyond the scope of this document, so it's assumed that a list with pairs of HKL and a boolean value (`True` for "observed" and `False` for "not observed") is available. Furthermore it's assumed that the space group belongs to Laue class :math:`m\bar{3}m`.

.. testcode:: ExSpaceGroupCheck

    from mantid.geometry import SpaceGroupFactory, PointGroupFactory

    # Small helper function that distinguishes three cases:
    #   0: The reflection is observed and allowed or not observed and not allowed
    #  -1: The reflection is allowed, but not observed - additional reflection condition is present
    #   1: The reflection is observed, but not allowed - systematic absence violation
    def conditionsMatch(spaceGroup, hkl, isObserved):
        isAllowed = spaceGroup.isAllowedReflection(hkl)

        if isAllowed == isObserved:
            return 0
        elif isAllowed and not isObserved:
            return -1
        else:
            return 1

    # Small helper function that returns the frequency of values in a list. Can be replaced with Counter from collections in Python >= 2.7
    def getValueFrequencies(values):
        frequencyDict = {}

        uniqueValues = set(values)
        for val in uniqueValues:
            frequencyDict[val] = values.count(val)

        return frequencyDict

    # List of reflections with "observation status" from a hypothetical experiment.
    reflections = [([1,0,0], False), ([1,1,0], False), ([1,1,1], True), ([2,0,0], False), ([2,1,0], False), ([2,1,1], False),
                ([2,2,0], True), ([2,2,1], False), ([2,2,2], False), ([3,0,0], False), ([3,1,0], False), ([3,1,1], True),
                ([3,2,0], False), ([3,2,1], False), ([3,2,2], False), ([3,3,0], False), ([3,3,1], True), ([3,3,2], False),
                ([3,3,3], True), ([4,0,0], True), ([4,1,0], False), ([4,1,1], False), ([4,2,0], False), ([4,2,1], False),
                ([4,2,2], True), ([4,3,0], False), ([4,3,1], False), ([4,3,2], False), ([4,3,3], False), ([4,4,0], True),
                ([4,4,1], False), ([4,4,2], False), ([4,4,3], False), ([5,0,0], False), ([5,1,0], False), ([5,1,1], True),
                ([5,2,0], False), ([5,2,1], False), ([5,2,2], False), ([5,3,0], False), ([5,3,1], True), ([5,3,2], False),
                ([5,3,3], True), ([5,4,0], False), ([5,4,1], False), ([5,4,2], False), ([6,0,0], False), ([6,1,0], False),
                ([6,1,1], False), ([6,2,0], True), ([6,2,1], False), ([6,2,2], False), ([6,3,0], False), ([6,3,1], False)]

    reflectionCount = len(reflections)
    print "There are", reflectionCount, "reflections to consider."

    # Check space groups and store results in a list
    spaceGroupMatchList = []

    # As described above, point group m-3m is assumed
    pg = PointGroupFactory.createPointGroup("m-3m")
    possibleSpaceGroups = SpaceGroupFactory.getSpaceGroupsForPointGroup(pg)
    for sgSymbol in possibleSpaceGroups:
        sgObject = SpaceGroupFactory.createSpaceGroup(sgSymbol)

        # For each (hkl, observed) pair obtain whether this matches the space group's conditions
        conditionsMatchList = [conditionsMatch(sgObject, x[0], x[1]) for x in reflections]

        # In this list, each reflection has a dictionary with frequency of the values 0, -1 and 1
        # (see the helper functions defined above).
        spaceGroupMatchList.append((sgSymbol, getValueFrequencies(conditionsMatchList)))

    # Sort the list according to abscence violations and additional reflection conditions
    spaceGroupMatchList.sort(key=lambda x: (x[1].get(1, 0), x[1].get(-1, 0)))

    # Remove the second setting that exists for some groups
    finalSpaceGroupMatchList = [x for x in spaceGroupMatchList if not ':2' in x[0]]

    # Print some information about the most likely matches
    print "5 best matching space groups:"

    for sgPair in finalSpaceGroupMatchList[:5]:
        sgStatus = sgPair[1]
        print "    {0}: {1} absence violations, {2: >2} additional absences, {3: >2} matches".format(sgPair[0], sgStatus.get(1, 0), sgStatus.get(-1, 0), sgStatus.get(0, 0))

    print "The best matching space group is:", finalSpaceGroupMatchList[0][0]

The script should produce the following output:

.. testoutput:: ExSpaceGroupCheck

    There are 54 reflections to consider.
    5 best matching space groups:
        F d -3 m: 0 absence violations,  3 additional absences, 51 matches
        F m -3 m: 0 absence violations,  6 additional absences, 48 matches
        P n -3 m: 0 absence violations, 31 additional absences, 23 matches
        P m -3 m: 0 absence violations, 42 additional absences, 12 matches
        F d -3 c: 6 absence violations,  3 additional absences, 45 matches
    The best matching space group is: F d -3 m

In this case, the script gave the right answer, because the list of reflections was created using the crystal structure of silicon, which, as mentioned above, belongs to space group type :math:`Fd\bar{3}m`. The systematic absences derived from the symmetry operations explain all observations (and absences) expcept three. These are caused by the Si-atom on a special position. The presence of atoms in special positions can lead to incorrect determination of the space group because the introduced additional reflection conditions may match those of a different space group.

.. [ITAPointGroups] International Tables for Crystallography (2006). Vol. A, ch. 10.1, p. 762

.. [Shmueli84] U. Shmueli, Acta Crystallogr. A, 40, p. 559 `DOI: 10.1107/S0108767384001161 <http://dx.doi.org/10.1107/S0108767384001161>`_)

.. [#f1] In the case of the monoclinic Laue class :math:`2/m` it's a bit more complicated, because there are two conventions regarding the unique axis. According to current crystallographic standards, the :math:`b`-axis is used, but in some cases one may find the :math:`c`-axis for this purpose. To resolve this, both options are offered in Mantid. When using the symbol ``2/m``, the :math:`b`-axis convention is used, for :math:`c` one has to explicitly provide the symbol as ``112/m``.


.. categories:: Concepts
