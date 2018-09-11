.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm authenticates the user's credentials against a specific catalog facility. The catalog is determined from the facility chosen, and is obtained from the config file Facilities.xml.

Usage
-----

**Example - log a user into the catalog.**

.. code-block:: python

    # Attempts to authenticate the user against the ISIS catalog.
    # When logging into the catalog the session class is returned.
    # This can then be used throughout to perform other ICAT routines.
    session = CatalogLogin(username='SECRET',password='SECRET',Facility="ISIS")

    # View the session ID for this catalog session.
    print("The session ID is: {}".format(session.getPropertyValue("Session")))

Output:

.. code-block:: python

    The session ID is: b931877c-3cfb-460e-9e88-ed4257020477

.. categories::

.. sourcelink::
