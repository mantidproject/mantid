.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Suggest possible time independent background range for CNCS. It works
for incident energy range from 0.5 to 50 meV. By default TibMax is 500
microseconds before the neutrons arrive at the sample, and TibMin is
3400 microseconds before Tibmax. This range is moved around if a prompt
pulse is in this interval, or it goes below the TOF frame minimum, or it
can be reduced to 2400 microseconds.

Usage
-----

**Example:**

.. testcode:: ExSuggestTibCNCS

    incidentEnergy = 3
    (tibMin,tibMax) = SuggestTibCNCS(incidentEnergy)
    print("Incident Energy = %i, tibMin = %.2f, tibMax = %.2f" % (incidentEnergy,tibMin,tibMax))

    incidentEnergy = 1
    (tibMin,tibMax) = SuggestTibCNCS(incidentEnergy)
    print("Incident Energy = %i, tibMin = %.2f, tibMax = %.2f" % (incidentEnergy,tibMin,tibMax))

Output:

.. testoutput:: ExSuggestTibCNCS

    Incident Energy = 3, tibMin = 44914.92, tibMax = 47314.92
    Incident Energy = 1, tibMin = 95621.15, tibMax = 99021.15

.. categories::

.. sourcelink::
