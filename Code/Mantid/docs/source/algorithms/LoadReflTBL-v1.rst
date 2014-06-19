.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

LoadReflTBl is loads ascii files in Reflectometry TBL format into a
tableworkspace. Format accepted is strict to only allow 17 columns of
data.

The 17 columns are split up into rows of 8, so a single row in the TBL
file would be split into 3 colums like so: (Where Z is the newly created
stitch group index) A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q

becomes

A, B, C, D, E, P, Q, Z F, G, H, I, J, P, Q, Z K, L, M, N, O, P, Q, Z

.. categories::
