.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Obtains a list of all instruments for active (logged in) catalogs by default. If a session is specified, then the instruments for that session will be returned.

Usage
-----

**Example - obtaining instrument names from current active catalogs.**

.. code-block:: python

    # Assuming you have previously logged into the ISIS catalog.
    instruments = CatalogListInstruments()

    # How many instruments are at ISIS?
    print("The number of instruments at ISIS is: {}".format(len(instruments)))

    for instrument in instruments:
        print("Instrument name is: {}".format(instrument))

    # Verify that the instrument belongs to ISIS.
    instrument = instruments[0]
    print("The facility of instrument {} is: {}".format(instrument, ConfigService.getInstrument(instrument).facility()))

Output:

.. code-block:: python

    The number of instruments at ISIS is: 38

    Instrument name is: ALF
    ...
    Instrument name is: WISH

    The facility of instrument ALF is: ISIS

.. categories::

.. sourcelink::
