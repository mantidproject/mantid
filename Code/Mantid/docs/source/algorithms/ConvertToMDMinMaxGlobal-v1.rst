.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm will try to calculate the MinValues and MaxValues limits
that are required in the ConvertToMD algorithm, using the following
procedure:

-  If QDimensions is CopyToMD the first value in MinValues is going to
   be the workspace minimum X coordinate, and the first value in
   MaxValues is going to be the maximum X coordinate
-  If QDimensions is \|Q\| or Q3D, first we calculate the maximum
   momentum transfer, Qmax. If dEAnalysisMode is Elastic, we convert to
   Momentum units, find the maximum value, and multiply by 2, since the
   maximum momentum transfer occurs when the incident beam and the
   scattered beam are anti-parallel.
-  If dEAnalysisMode is Direct or Indirect, we convert to DeltaE units,
   find the minimum and maximum (dEmin, dEmax), calculate to ki and kf.
   The maximum momentum transfer is ki+kf.
-  If QDimensions is \|Q\|, the first value of the MinValues is 0, and
   the first value of MaxValues is Qmax
-  If QDimensions is Q3D, and Q3DFrames is Q the first three values of
   the MinValues are -Qmax, -Qmax, -Qmax, and the first three values of
   MaxValues are Qmax, Qmax, Qmax
-  If QDimensions is Q3D, and Q3DFrames is HKL the first three values of
   the MinValues are -Qmax\*a/(2\*pi), -Qmax\*b/(2\*pi),
   -Qmax\*c/(2\*pi), and the first three values of MaxValues are
   Qmax\*a/(2\*pi), Qmax\*b/(2\*pi), Qmax\*c/(2\*pi). Note: for HKL mode
   one needs to have an OrientedLattice attached to the sample.
-  If QDimensions is \|Q\| or Q3D, and dEAnalysisMode is Elastic or
   Inelastic, the next value in MinValues is dEmin, and the next value
   in MaxValues is dEmax
-  If any OtherDimensions are added, the last values in MinValues
   (MaxValues) are the minimum (maximum) of each of the sample log
   values selected

.. categories::
