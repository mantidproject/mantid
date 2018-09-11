.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

In algorithms related to `Le Bail Fit <Le Bail Fit>`__ and powder
diffractomer instrument profile calibration, TableWorkspace containing
the peak profile parameters' information are used as input and output.
*UpdatePeakParameterTableValue* gives user the method to change the
value of parameters' information, including its status to fit, value,
minimum/maximum value (for boundary contrains) and step size (for Monte
Carlo optimizer).

Format of TableWorkspace
------------------------

TableWorkspace containing peak profile parameters must have 2 columns,
"Name" and "Value". It can have but not be limited to the following
columns, "Min", "Max", "Stepsize" and "FitOrTie".

Specify A Cell or Cells
-----------------------

The cell to have value updated can be specified by its row and column
index.

-  Column index is determined by property "Column".
-  Row index can be specified by property "Row", which requires a list
   of row indexes, or property "ParameterNames", which requires a list
   of strings. If "ParameterNames" is used as the input of row indexes,
   the algorithm will go through each row to match the string value of
   cell "Name" (i.e., parameter name) to each input parameter name.
-  If neither "Row" nor "ParameterNames" is given by user, then all
   cells in the column will have the value updated to a same value from
   either "NewFloatValue" or "NewStringValue" according to the type of
   the cell.
-  If multiple row indexes are specified, then all the cells of the
   specified column and rows are updated to same value from either
   "NewFloatValue" or "NewStringValue".

How to use algorithm with other algorithms
------------------------------------------

This algorithm is designed to work with `other
algorithms <Le Bail Fit>`__ to do Le Bail fit.


Usage
-----

**Example - Changing the value for a parameter:**

.. testcode:: ExFloatValue

    tablews=CreateEmptyTableWorkspace()
    tablews.addColumn("str", "Name")
    tablews.addColumn("double", "Value")
    tablews.addRow(["A", 1.34])
    tablews.addRow(["B", 2.34])

    print("Value before %.2f" % tablews.cell(0, 1))

    UpdatePeakParameterTableValue(tablews, Column="Value", ParameterNames=["A"], NewFloatValue=1.00)
    
    print("Value after %.2f" % tablews.cell(0, 1))

Output:

.. testoutput:: ExFloatValue

    Value before 1.34
    Value after 1.00

.. categories::

.. sourcelink::
