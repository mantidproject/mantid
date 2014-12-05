
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm creates TableWorkspace with all symmetry independent reflections based on crystal structure and limits for lattice spacings. If a space group that belongs to a point group other than :math:`\bar{1}` is specified, the lattice parameters supplied to the algorithm are corrected according to the crystal system:

============== ============================================================================== ================
Crystal system Lattice parameters used by the algorithm                                       Constrained Cell
============== ============================================================================== ================
Triclinic      :math:`a`, :math:`b`, :math:`c`, :math:`\alpha`, :math:`\beta`, :math:`\gamma` :math:`a`, :math:`b`, :math:`c`, :math:`\alpha`, :math:`\beta`, :math:`\gamma`
Monoclinic     :math:`a`, :math:`b`, :math:`c`, :math:`\beta`                                 :math:`a`, :math:`b`, :math:`c`, :math:`90^\circ`, :math:`\beta`, :math:`90^\circ`
Orthorhombic   :math:`a`, :math:`b`, :math:`c`                                                :math:`a`, :math:`b`, :math:`c`, :math:`90^\circ`, :math:`90^\circ`, :math:`90^\circ`
Tetragonal     :math:`a`, :math:`c`                                                           :math:`a`, :math:`a`, :math:`c`, :math:`90^\circ`, :math:`90^\circ`, :math:`90^\circ`
Hexagonal      :math:`a`, :math:`c`                                                           :math:`a`, :math:`a`, :math:`c`, :math:`90^\circ`, :math:`90^\circ`, :math:`120^\circ`
Trigonal       :math:`a`, :math:`\alpha`                                                      :math:`a`, :math:`a`, :math:`a`, :math:`\alpha`, :math:`\alpha`, :math:`\alpha`
Cubic          :math:`a`                                                                      :math:`a`, :math:`a`, :math:`a`, :math:`90^\circ`, :math:`90^\circ`, :math:`90^\circ`
============== ============================================================================== ================

If other parameters are supplied, for example `a = 2.0` and `b = 5.0` with point group :math:`m\bar{3}m`, these parameters are discarded by the algorithm. The resulting TableWorkspace can be used by other POLDI-related routines.

Usage
-----

The following usage example illustrates how the algorithm can be used to generate a table of symmetry independent reflections for a given lattice, in this case using the crystal structure of CsCl.

.. testcode:: PoldiCreatePeaksFromCellExample

    # Generate all unique reflections for CsCl between 0.55 and 4.0 Angstrom
    csClReflections = PoldiCreatePeaksFromCell(
                        SpaceGroup="P m -3 m",
                        Atoms="Cl 0 0 0 1.0 0.005; Cs 0.5 0.5 0.5 1.0 0.005",
                        a=4.126,
                        LatticeSpacingMin=0.55, LatticeSpacingMax=4.0)

    print "CsCl has", csClReflections.rowCount(), "unique reflections in the range between 0.55 and 4.0 Angstrom."

Output:

.. testoutput:: PoldiCreatePeaksFromCellExample

    CsCl has 68 unique reflections in the range between 0.55 and 4.0 Angstrom.

.. categories::

