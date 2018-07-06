.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is a workflow algorithm that creates files suitable as input
into `GUDRUN <http://www.isis.stfc.ac.uk/instruments/sandals/data-analysis/gudrun8864.html>`_.

#. :ref:`algm-PDLoadCharacterizations`
#. :ref:`algm-LoadEventAndCompress` if ``InputWorkspace`` is not
   provided
#. :ref:`algm-PDDetermineCharacterizations` to determine information
   from the characterization file
#. :ref:`algm-AlignAndFocusPowder`
#. :ref:`algm-NormaliseByCurrent`
#. :ref:`algm-SetUncertainties` if ``SetUncertainties`` is specified
#. :ref:`algm-SaveNexusPD`

Workflow
########

.. diagram:: PDToGUDRUN-v1_wkflw.dot

.. categories::

.. sourcelink::
