.. _09_pim_ex_4:

============================
Python in Mantid: Exercise 4
============================

The aim of this exercise is to build some complex examples of data manipulation

Manipulating data arrays
========================

A - Create a MatrixWorkspace
----------------------------

#. In a new python script, Load the HRP39182.RAW file, and then run Rebin on it with Params=1e4 to overwrite your InputWorkspace
#. Confirm that your workspace now has 10 bins, by creating a variable called nbins and printing it in your script
#. Either using numpy functions, or by writing a nested loop, find the maximum value for the counts (Y array) in each spectrum
#. Create a new list for your output values, and construct a new workspace to hold your max values. *Note that the X and Error arrays are not important for this exercise, so you may fabricate them if you wish*.


B - Create a TableWorkspace
---------------------------

Instead of creating a MatrixWorkspace as an output as we did in Part One, create a
:ref:`table workspace <04_table_ws_py>` for the output, using :ref:`algm-CreateEmptyTableWorkspace`. Create a table
workspace containing 3 columns called *Spectrum Number*, *Max* and *Min*. Use the same rebinned input workspace in Part
One, as the source.


:ref:`Solutions <04_pim_sol>`
