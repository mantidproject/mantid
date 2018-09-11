.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm logs a user out of all catalogs, or a specific catalog.

Usage
-----

**Example - logs a user out of the catalog.**

.. code-block:: python

    # Logs the user out of the catalog and kills CatalogKeepAlive()
    CatalogLogout()

    # Logs the user out of a specific catalog, and kills the related CatalogKeepAlive()
    # Assuming session is the return value of CatalogLogin()
    CatalogLogout(session)

.. categories::

.. sourcelink::
