
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Checks if a new release of Mantid is available using the Github API for inspecting the most recent release.
In order to reduce API usage and optimise performance the request is sent with a "if-modified-since" header
with the date of the current release.

This algorithm is run on asynchronously on start-up in release builds and official releases of Mantid.
It only outputs messages to the logs and output properties, it does not change anything else.

If you want to disable the check on start-up add this to your mantid.user.properties file::

    CheckMantidVersion.OnStartup = 0

Usage
-----

**Example - CheckMantidVersion**

.. code::

    (current_version, most_recent_version, is_new_version_available)=CheckMantidVersion()
    print("Current Version: {}".format(current_version))
    print("Most Recent Version: {}".format(most_recent_version))
    print("Is a newer version available? {}".format(is_new_version_available))

Output:

.. code::

    Current Version: ...
    Most Recent Version: ...
    Is a newer version available? ...


.. categories::

.. sourcelink::
