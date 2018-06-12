.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Creates a table workspace of the average values of log values against
the run number.

There are special cases for:

-  beamlog\_(counts, frames, etc): last few points end up in next run's
   log. Find Maximum.
-  comment (separate function)
-  time series, take average for t>0 (if available)

It should:

#. Load any file type that :ref:`algm-Load` can handle.
#. Not crash with multiperiod data - although values will be from period
   1
#. Handle gaps in the file structure (although this can be slow over a
   network if you choose a range of 100s)
#. Load only a single spectra of the data (if the file loader supports
   this).
#. Print out the list of acceptable log names if one is entered
   incorrectly.
#. Use a hidden workspace for the temporary loaded workspaces, and clean
   up after itself.

Usage
-----

**Example:**

.. testcode:: Exlogtable
    
        def print_table_workspace(wsOut):
	    print(" ".join(wsOut.getColumnNames()[i]
	          for i in range(wsOut.columnCount())))

            for rowIndex in range(wsOut.columnCount()):
  	        print(" ".join(str(wsOut.column(i)[rowIndex])
	              for i in range(wsOut.columnCount())))

        wsComment = LoadLogPropertyTable(FirstFile = "MUSR00015189.nxs", 
                    LastFile = "MUSR00015193.nxs", LogNames="comment")
        print("The comments of all the files")
        print_table_workspace(wsComment)

        wsMultiple = LoadLogPropertyTable(FirstFile = "MUSR00015189.nxs", 
                    LastFile = "MUSR00015193.nxs", LogNames="Temp_Sample,dur")
        print("\nThe Temp_Sample and dur logs")
        print_table_workspace(wsMultiple)


Output:

.. testoutput:: Exlogtable

    The comments of all the files
    RunNumber comment
    15189 18.95MHz 100W
    15190 18.95MHz 100W

    The Temp_Sample and dur logs
    RunNumber Temp_Sample dur
    15189 0.0 100
    15190 0.0 100
    15191 0.0 101


.. categories::

.. sourcelink::
