.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Calculates mean, median, maximum, minimum and standard deviation for each
numerical column of a table workspace.

Usage
-----

**Example - Getting statistics of a table workspace.**

.. testcode:: ExStatisticsOfTableWorkspace

    ws = CreateEmptyTableWorkspace()
    ws.addColumn('int', 'a')
    ws.addColumn('float', 'b')
    ws.addRow([1, 3.2])
    ws.addRow([2, 3.4])
    ws.addRow([3, 3.6])
    ws.addRow([4, 3.8])

    stats = StatisticsOfTableWorkspace(ws)

    for idx in range(stats.rowCount()):
        stat_name = stats.column('Statistic')[idx]
        stat_value = stats.column('a')[idx]
        print('%s of column \'a\' is %.3f' % (stat_name, stat_value))

Output:

.. testoutput:: ExStatisticsOfTableWorkspace

    StandardDev of column 'a' is 1.118
    Minimum of column 'a' is 1.000
    Median of column 'a' is 2.500
    Maximum of column 'a' is 4.000
    Mean of column 'a' is 2.500

.. categories::

.. sourcelink::
