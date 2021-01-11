.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------
This algorithm takes a table of peak centres and standard deviation and matches it to a database
to find probable energy transitions for peaks.


Input Table and datafile requirement
####################################

- input table must have atleast 2 tables in which one column is the peak centre and the other is the standard deviation,
the names of the columns must be given if they differ from the default values: centre and sigma respectively.

- json file can be loaded to override the default but must follow structure below:
	Format of file:
            {
            'Element': {
                'Z': z,
                'A': a,
                'Primary' : {
                    'Transition':energy,
                    ..., ...
                    },
                'Secondary': {
                    'Transition': energy,
                    ..., ...
                    }
				}
			}

Usage
-----

*Example: Using all defaults**

.. testcode:: OnlyPeaktable

	from mantid.simpleapi import *
	import matplotlib.pyplot as plt
	import numpy

	table = CreateEmptyTableWorkspace(OutputWorkspace="input")
	rows = [(900, 0.8), (306, 0.8), (567, 0.8), (3, 0.8)]

	table.addColumn("double","centre")
	table.addColumn("double","sigma")

	for row in rows:
		table.addRow(row)
    
	PeakMatching(table)

	primary_matches = mtd['primary_matches'] 
	secondary_matches = mtd['secondary_matches'] 
	all_matches = mtd['all_matches'] 
	sorted_by_energy = mtd['all_matches_sorted_by_energy'] 
	element_count = mtd[ 'element_count'] 

	print("--"*25)
	print(primary_matches.row(0))
	print("--"*25)
	print(secondary_matches.row(0))
	print("--"*25)
	print(all_matches.row(0))
	print("--"*25)
	print(sorted_by_energy.row(0))
	print("--"*25)
	print(element_count.row(0))
		
Output:

.. testoutput:: OnlyPeaktable

	--------------------------------------------------
	{'Peak centre': 900.0, 'Database Energy': 898.0, 'Element': 'Dy', 'Transition': 'M(5f->3d) ', 'Error': 2.4000000000000004, 'Difference': 2.0}
	--------------------------------------------------
	{'Peak centre': 567.0, 'Database Energy': 567.0, 'Element': 'Ag', 'Transition': 'M(7f->3d)', 'Error': 0.0, 'Difference': 0.0}
	--------------------------------------------------
	{'Peak centre': 567.0, 'Database Energy': 567.0, 'Element': 'Ag', 'Transition': 'M(7f->3d)', 'Error': 0.0, 'Difference': 0.0}
	--------------------------------------------------
	{'Peak centre': 3.0, 'Database Energy': 3.4, 'Element': 'Li', 'Transition': 'L(3d->2p)', 'Error': 0.8, 'Difference': 0.3999999999999999}
	--------------------------------------------------
	{'Element': 'Ag', 'Counts': 5}
	
*Example: Renaming tables**

.. testcode:: renametables

	from mantid.simpleapi import *
	import matplotlib.pyplot as plt
	import numpy

	table = CreateEmptyTableWorkspace(OutputWorkspace="input")
	rows = [(900, 0.8), (306, 0.8), (567, 0.8), (3, 0.8)]

	table.addColumn("double","centre")
	table.addColumn("double","sigma")

	for row in rows:
		table.addRow(row)
    
	PeakMatching(table,PrimaryPeaks="primary",SecondaryPeaks="secondary",AllPeaks="all",SortedByEnergy="sort",ElementCount="count")

	primary_matches = mtd['primary'] 
	secondary_matches = mtd['secondary'] 
	all_matches = mtd['all_matches'] 
	sorted_by_energy = mtd['sort'] 
	element_count = mtd[ 'count'] 

	print("--"*25)
	print(primary_matches.row(1))
	print("--"*25)
	print(secondary_matches.row(1))
	print("--"*25)
	print(all_matches.row(1))
	print("--"*25)
	print(sorted_by_energy.row(1))
	print("--"*25)
	print(element_count.row(1))
		
Output:

.. testoutput:: renametables

	--------------------------------------------------
	{'Peak centre': 900.0, 'Database Energy': 900.7, 'Element': 'Ag', 'Transition': 'L(3d3/2->2p3/2)', 'Error': 0.8, 'Difference': 0.7000000000000455}
	--------------------------------------------------
	{'Peak centre': 567.0, 'Database Energy': 567.0, 'Element': 'In', 'Transition': 'M(6f->3d)', 'Error': 0.0, 'Difference': 0.0}
	--------------------------------------------------
	{'Peak centre': 567.0, 'Database Energy': 566.7, 'Element': 'I', 'Transition': 'M(5f->3d)', 'Error': 0.8, 'Difference': 0.2999999999999545}
	--------------------------------------------------
	{'Peak centre': 306.0, 'Database Energy': 304.1, 'Element': 'W', 'Transition': 'O(7i->5g)', 'Error': 2.4000000000000004, 'Difference': 1.8999999999999773}
	--------------------------------------------------
	{'Element': 'W', 'Counts': 4}


	*Example: Using non default column names**

.. testcode:: non-defaultcolumns

	from mantid.simpleapi import *
	import matplotlib.pyplot as plt
	import numpy

	table = CreateEmptyTableWorkspace(OutputWorkspace="input")
	rows = [(900, 0.8), (306, 0.8), (567, 0.8), (3, 0.8)]

	table.addColumn("double","center")
	table.addColumn("double","standard deviation")

	for row in rows:
		table.addRow(row)
    
	PeakMatching(table, PeakCentreColumn = "center",SigmaColumn = "standard deviation")

	primary_matches = mtd['primary_matches'] 
	secondary_matches = mtd['secondary_matches'] 
	all_matches = mtd['all_matches'] 
	sorted_by_energy = mtd['all_matches_sorted_by_energy'] 
	element_count = mtd[ 'element_count'] 

	print("--"*25)
	print(primary_matches.row(2))
	print("--"*25)
	print(secondary_matches.row(2))
	print("--"*25)
	print(all_matches.row(2))
	print("--"*25)
	print(sorted_by_energy.row(2))
	print("--"*25)
	print(element_count.row(2))
		
Output:

.. testoutput:: non-defaultcolumns

	--------------------------------------------------
	{'Peak centre': 900.0, 'Database Energy': 899.2, 'Element': 'Au', 'Transition': 'M(4f5/2->3d3/2)', 'Error': 0.8, 'Difference': 0.7999999999999545}
	--------------------------------------------------
	{'Peak centre': 567.0, 'Database Energy': 566.7, 'Element': 'I', 'Transition': 'M(5f->3d)', 'Error': 0.8, 'Difference': 0.2999999999999545}
	--------------------------------------------------
	{'Peak centre': 3.0, 'Database Energy': 3.4, 'Element': 'Li', 'Transition': 'L(3d->2p)', 'Error': 0.8, 'Difference': 0.3999999999999999}
	--------------------------------------------------
	{'Peak centre': 306.0, 'Database Energy': 304.1, 'Element': 'W', 'Transition': 'O(7i->5g)', 'Error': 2.4000000000000004, 'Difference': 1.8999999999999773}
	--------------------------------------------------
	{'Element': 'Ni', 'Counts': 4}


.. categories::

.. sourcelink::
