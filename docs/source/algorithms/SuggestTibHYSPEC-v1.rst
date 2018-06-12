.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Suggest possible time independent background range for HYSPEC. It works
for incident energy range from 3 to 100 meV.

Usage
-----

**Example:**

.. testcode:: ExSuggestTibHYSPEC

    incidentEnergy = 5
    (tibMin,tibMax) = SuggestTibHYSPEC(incidentEnergy)
    print("Incident Energy = %i, tibMin = %.2f, tibMax = %.2f" % (incidentEnergy,tibMin,tibMax))

    incidentEnergy = 40
    (tibMin,tibMax) = SuggestTibHYSPEC(incidentEnergy)
    print("Incident Energy = %i, tibMin = %.2f, tibMax = %.2f" % (incidentEnergy,tibMin,tibMax))

Output:

.. testoutput:: ExSuggestTibHYSPEC

    Incident Energy = 5, tibMin = 39515.88, tibMax = 41515.88
    Incident Energy = 40, tibMin = 11898.11, tibMax = 13898.11

.. categories::

.. sourcelink::
