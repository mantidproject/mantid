.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Creates a model for a chopper using the given parameters. The parameters
are given as a string to allow flexibility for each chopper model having
different parameterisation.

The chopper point is an index that can be used for multi-chopper
instruments. The indices start from zero, with this being closest to
moderator.

Available models with parameter names:

-  FermiChopper -

   -  AngularVelocity - The angular velocity value or log name
   -  ChopperRadius - The radius, in metres, of the whole chopper
   -  SlitThickness - The thickness, in metres, of the slit
   -  SlitRadius - The radius of curvature, in metres, of the slit
   -  JitterSigma - The FWHH value for the jitter calculation in
      microseconds
   -  Ei - The Ei for this run as a value or log name

.. categories::

.. sourcelink::
