.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Some instrument definition file (`IDF <InstrumentDefinitionFile>`__)
positions are only approximately correct and the true positions are
located within data files. This algorithm reads the detector positioning
from the supplied file and updates the instrument accordingly. It
currently supports ISIS Raw, ISIS NeXus files and ASCII files.

It is assumed that the positions specified in the file are all with
respect to the a coordinate system defined with its origin at the sample
position. Note that this algorithm moves the detectors without
subsequent rotation, hence this means that detectors may not for example
face the sample perfectly after this algorithm has been applied.

Additional Detector Parameters Using ASCII File
###############################################

The ASCII format allows a multi-column text file to provide new
positions along with additional parameters for each detector. If a text
file is used then the ``AsciiHeader`` parameter is required as it
identifies each column in the file as header information in the file is
always ignored. There is a minor restriction in that the first column is
expected to specify either a detector ID or a spectrum number and will
never be interpreted as anything else.

The keywords recognised by the algorithm to pick out detector position
values & spectrum/ID values are: spectrum, ID, R,theta, phi. The
spectrum/ID keywords can only be used in the first column. A dash (-) is
used to ignore a column.

As an example the following header:

::

    spectrum,theta,t0,-,R

and the following text file:

::

        1   0.0000  -4.2508  11.0550  -2.4594
        2   0.0000   0.0000  11.0550   2.3800
        3 130.4653  -0.4157  11.0050   0.6708
        4 131.9319  -0.5338  11.0050   0.6545
        5 133.0559  -0.3362  11.0050   0.6345

would tell the algorithm to interpret the columns as:

#. Spectrum number
#. Theta position value
#. A new instrument parameter called t0
#. This column would be ignored
#. R position value

.. categories::
