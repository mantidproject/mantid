.. _Amend Config:

==============
 amend_config
==============

.. module:`mantid.kernel`

.. autoclass:: mantid.kernel.amend_config
   :members:

Usage
---------

.. code-block:: python

    from mantid.kernel import amend_config

    with amend_config(facility="FacilityA", instrument="InstrumentX"):
        # Inside this block, configuration for FacilityA and InstrumentX is active.
        perform_custom_operation()

    # Outside the block, the original configuration is restored.

    with amend_config(data_dir="/custom/data"):
        # Configuration with a custom data directory.
        do_something_with_custom_data()

    # Original configuration is restored again.

    temp_config = {"Notifications.Enabled": "On", "logging.loggers.root.level": "debug"}
    with amend_config(**temp_config):
        # Configuration with dictionary of properties
        do_something_else()

    # Original configuration is restored again.

