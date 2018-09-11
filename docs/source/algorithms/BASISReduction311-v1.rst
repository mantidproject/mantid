.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

For each property, the algorithm will remember the last value used. If user deletes
this value and leaves blank the property field, the default value will be used. Default
values are typical of the silicon311 reflection.

Description
-----------

**Run numbers**:
The syntax for the run numbers designation allows runs to be segregated
into sets. The semicolon symbol ";" is used to separate the runs into sets.
Runs within each set are jointly reduced.

Examples:

- 2144-2147,2149,2156  is a single set. All runs jointly reduced.

- 2144-2147,2149;2156  is set 2144-2147,2149 and set 2156. The sets are reduced separately from each other.

If *DoIndividual* is checked, then each run number is reduced separately
from the rest. The semicolon symbol is ignored.

**Momentum transfer binning scheme**: Three values are required, the
lower boundary the bin with the minimum momentum, the bin width, and the
upper boundary ofthe bin with the maximum momentum.

Reflection Selector
###################

There are typical values for the properties of the 311 reflection:

+------------+----------------+------------------------+
| Reflection |  Energy bins   | Momentum transfer bins |
|            |   (micro-eV)   |   (inverse Angstroms)  |
+============+================+========================+
| silicon311 | -740, 1.6, 740 |      0.5, 0.2, 3.7     |
+------------+----------------+------------------------+

Also the following mask files is associated to the 311 reflection:

+-----------+------------------------------------------------------------------------------------------------+
|Reflection | Mask file                                                                                      |
+===========+================================================================================================+
|silicon311 | BASIS_Mask_OneQuarterRemains_SouthBottom.xml                                                   |
+-----------+------------------------------------------------------------------------------------------------+

This mask files can be found in the SNS filesystem
(**/SNS/BSS/shared/autoreduce/new_masks_08_12_2015/**)


Usage
-----

.. warning::

    This algorithm is not meant to be run from the command line.

.. categories::

.. sourcelink::

