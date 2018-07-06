.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is a workflow algorithm that creates files suitable as input
into `PDFgetN <http://pdfgetn.sourceforge.net/>`_.

#. :ref:`algm-PDLoadCharacterizations`
#. :ref:`algm-LoadEventAndCompress` if ``InputWorkspace`` is not
   provided
#. :ref:`algm-PDDetermineCharacterizations` to determine information
   from the characterization file
#. :ref:`algm-AlignAndFocusPowder`
#. :ref:`algm-NormaliseByCurrent`
#. :ref:`algm-SetUncertainties` (``SetError="sqrt"``)
#. :ref:`algm-SaveGSS`

Workflow
########

.. diagram:: PDToPDFgetN-v1_wkflw.dot

.. categories::

.. sourcelink::
