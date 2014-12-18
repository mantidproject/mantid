.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Use data from the supplied file, written in the RKH format, to correct
the input data. The operations allowed for the correction are
`multiply <http://www.mantidproject.org/multiply>`_ and `divide <http://www.mantidproject.org/divide>`_.

Allowed correction files may contain one spectrum with many bins or many
spectra each with one bin. If the are many bins then the
FirstColumnValue must match the :ref:`units <Unit Factory>` of the
(X-values on the) workspace on the InputWorkspace. When there are many
spectra (e.g. flood correction files) FirstColumnValue must be set to
"SpectrumNumber" and the number of spectra in the file and workspace
must match.

.. categories::
