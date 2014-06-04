.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Saves a TableWorkspace at least 8 colunms wide into an ascii file in
17-column Reflectometry TBL format compatible with the old ISIS
reflectometry Interface.

The 8 columns are grouped into rows of 17 according to stitch index, so
up to 3 rows int he table would become a single row in the TBL file like
so: (Where Z is an identical stitch group index, and - is ignored as
only the first instance of P and Q are used in the file)

A, B, C, D, E, P, Q, Z F, G, H, I, J, -, -, Z K, L, M, N, O, -, -, Z

becomes

A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q

Limitations
###########

The Algorithm will fail if any stitch index appears more than 3 times,
as the old interface does not support more than 3 runs per row.

Stitch groups of index 0 are treated as non-grouped, and will not be
grouped with one another (and by extension can be larger than 3
members). They will however be moved to the end of the file

.. categories::
