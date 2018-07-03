.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm refreshes the current session to the maximum amount provided by the catalog API. This algorithm is run when :ref:`CatalogLogin <algm-CatalogLogin>` is executed.

Usage
-----

**Example - keeps the current session alive.**

.. code-block:: python

    # Attempts to keep ALL sessions alive indefinitely.
    CatalogKeepALive()

    # Attempts to keep a specific session alive.
    # This assumes the return value of CatalogLogin() is stored in session.
    CatalogKeepALive(Session = session.getPropertyValue("Session"))

.. categories::

.. sourcelink::
