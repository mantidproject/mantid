.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Converts the representation of the vertical axis (the one up the side of
a matrix in MantidPlot) of a Workspace2D from its default of holding the
spectrum number to the target unit given - theta, elastic Q and elastic
Q^2.

The spectra will be reordered in increasing order by the new unit and
duplicates will not be aggregated. Any spectrum for which a detector is
not found (i.e. if the instrument definition is incomplete) will not
appear in the output workspace.

Usage
-----

**Example: Convert vertical axis to theta**

.. testcode::

   # Creates a workspace with some detectors attached
   dataws = CreateSampleWorkspace(NumBanks=1,BankPixelWidth=2) # 2x2 detector

   theta = ConvertSpectrumAxis(dataws, Target="Theta")

   vertical_axis = theta.getAxis(1)
   print "There are %d axis values" % vertical_axis.length()
   print "Final theta value: %f (degrees)" % vertical_axis.getValue(vertical_axis.length() - 1)

.. testoutput::

   There are 4 axis values
   Final theta value: 0.129645 (degrees)

**Example: Convert vertical axis to elastic Q for direct geometry**

.. testcode::

   # Creates a workspace with some detectors attached
   dataws = CreateSampleWorkspace(NumBanks=1,BankPixelWidth=2) # 2x2 detector

   qws = ConvertSpectrumAxis(dataws, Target="ElasticQ", EFixed=15., EMode="Direct")

   vertical_axis = qws.getAxis(1)
   print "There are %d axis values" % vertical_axis.length()
   print "Final Q value: %f (A^-1)" % vertical_axis.getValue(vertical_axis.length() - 1)

.. testoutput::

   There are 4 axis values
   Final Q value: 0.006088 (A^-1)

.. categories::
