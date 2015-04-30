.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Used in the Liquids Reflectometer reduction at the SNS, this algorithm
computes the absolute scaling factors for the data sets that we are going to stitch 
together.

The algorithm runs through a sequence of direct beam data sets
to extract scaling factors. The method was developed by J. Ankner (ORNL).

As we loop through, we find matching data sets with the only
difference between the two is an attenuator.
The ratio of those matching data sets allows use to rescale
a direct beam run taken with a larger number of attenuators
to a standard data set taken with tighter slit settings and
no attenuators.

The normalization run for a data set taken in a given slit setting
configuration can then be expressed in terms of the standard 0-attenuator
data set with:
   D_i = F_i D_0

  Here's an example of runs and how they are related to F.

        run: 55889, att: 0, s1: 0.26, s2: 0.26
        run: 55890, att: 1, s1: 0.26, s2: 0.26
        run: 55891, att: 1, s1: 0.33, s2: 0.26 --> F = 55891 / 55890
        run: 55892, att: 1, s1: 0.45, s2: 0.26 --> F = 55892 / 55890
        run: 55895, att: 1, s1: 0.81, s2: 0.26
        run: 55896, att: 2, s1: 0.81, s2: 0.26
        run: 55897, att: 2, s1: 1.05, s2: 0.35 --> F = 55897 / 55896 * 55895 / 55890

.. categories::
