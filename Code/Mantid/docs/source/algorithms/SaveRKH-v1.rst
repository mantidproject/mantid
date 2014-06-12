.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Saves the the given workspace to a file which will be formatted in one
of the LOQ data formats (see
`here <http://www.isis.rl.ac.uk/archive/LargeScale/LOQ/other/formats.htm>`__).
1D or 2D workspaces may be saved. If a 1D workspace is 'horizontal' (a
single spectrum) then the first column in the three column output will
contain the X values of the spectrum (giving the bin centre if histogram
data). For a 'vertical' (single column) 1D workspace, the first column
of the file will contain the spectrum number.

.. categories::
