.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm searches over all of the values in a workspace and if it
finds a value set to NaN (not a number), infinity or larger than the
'big' threshold given then that value and the associated error is
replaced by the user provided values.

If no value is provided for either NaNValue, InfinityValue or
BigValueThreshold then the algorithm will exit with an error, as in this
case it would not be checking anything.

Algorithm is now event aware.

.. categories::
