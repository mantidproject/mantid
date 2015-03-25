
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Some steps in the analysis of POLDI data require that detected peaks are indexed. This can be done by using
:ref:`algm-PoldiIndexKnownCompounds`, which accepts a table with unindexed peaks and one or more workspaces with
calculated peaks corresponding to the crystal structures that are expected in the sample. These can be calculated
using the algorithm :ref:`algm-PoldiCreatePeaksFromCell`. Calling this algorithm over and over with the same
parameters is not practical, but storing the tables is not practical either, since lattice parameters may change
slightly from sample to sample.

PoldiCreatePeaksFromFile reads a text file which contains one or more crystal structure definitions. Since the
analysis requires information mainly about the lattice and the symmetry, the format is very simple. The following
block shows how such a file would look when there are two compounds:

.. code-block:: none

    # The name may contain letters, numbers and _
    Iron_FCC {
        # Up to 6 values in the order a, b, c, alpha, beta, gamma.
        # Lattice parameters are given in Angstrom.
        Lattice: 3.65
        Spacegroup: F m -3 m
        Atoms: {
            # Element x y z are mandatory. Optional occupancy and isotropic ADP (in Angstrom^2)
            Fe 0.0 0.0 0.0
        }
    }

    Iron_BCC {
        Lattice: 2.88
        Spacegroup: F m -3 m
        Atoms: {
            Fe 0.0 0.0 0.0
        }
    }

Note that only the atoms in the asymmetric unit need to be specified, the space group is used to generate all
equivalent atoms. This information is used to determine systematic absences, while the space group is also used by
some POLDI algorithms to obtain the point group to get reflection multiplicities and more. Anything that follows the
`#`-character is considered a comment and is ignored by the parser to allow documentation of the crystal structures
if necessary.

The algorithm will always produce a WorkspaceGroup which contains as many peak tables as compounds specified in the
file.

Usage
-----

.. include:: ../usagedata-note.txt

The following usage example takes up the file showed above and passes it to the algorithm.

.. testcode::

    # Create two tables with expected peaks directly from a file
    compounds = PoldiCreatePeaksFromFile('PoldiCrystalFileExample.dat', LatticeSpacingMin=0.7)

    compound_count = compounds.getNumberOfEntries()
    print 'Number of loaded compounds:', compound_count

    for i in range(compound_count):
        ws = compounds.getItem(i)
        print 'Compound ' + str(i + 1) +':', ws.getName(), 'has', ws.rowCount(), 'reflections in the resolution range.'


The script produces a WorkspaceGroup which contains a table with reflections for each compound in the file:

.. testoutput::

    Number of loaded compounds: 2
    Compound 1: Iron_FCC has 11 reflections in the resolution range.
    Compound 2: Iron_BCC has 8 reflections in the resolution range.

.. testcleanup::

    DeleteWorkspace('compounds')

.. categories::
