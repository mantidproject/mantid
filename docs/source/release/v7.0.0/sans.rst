============
SANS Changes
============

New Features
------------
- (`#42054 <https://github.com/mantidproject/mantid/pull/42054>`_) The :ref:`Runs Tab <ISIS_SANS_Runs_Tab-ref>` of the :ref:`ISIS SANS GUI<ISIS_Sans_interface_contents>` now has a ``Clean Up ADS`` checkbox, which automatically deletes all non-reduced workspaces (optimization, raw, transmission, and their monitors) from the ADS at the end of a Batch Reduction.
- (`#42054 <https://github.com/mantidproject/mantid/pull/42054>`_) The :ref:`BatchReduce <SANSScriptingBatchReduce>` function on the :ref:`ISIS SANS Command Interface<ScriptingSANSReductions>` now has a ``clean_up_ads`` parameter to automatically delete all non-reduced workspaces (optimization, raw, transmission, and their monitors) from the ADS at the end of a Batch Reduction.
- (`#42334 <https://github.com/mantidproject/mantid/pull/42334>`_) :ref:`ISIS SANS TOML <sans_toml_v1-ref>` now has updated documentation with a toml code block example for polarization fields.

Bugfixes
--------
- (`#41877 <https://github.com/mantidproject/mantid/pull/41877>`_) The :ref:`ISIS SANS Command Interface <ScriptingSANSReductions>` now selects the correct range for transmission fitting when using the ``TransFit`` command for every container selector.
- (`#41971 <https://github.com/mantidproject/mantid/pull/41971>`_) The ``AddRuns`` command of the :ref:`ISIS SANS Command Interface <ScriptingSANSReductions>` and the ``Sum Runs`` tab of the :ref:`ISIS SANS GUI <ISIS_Sans_interface_contents>` now correctly use the custom save directory to save the added runs.
- (`#41997 <https://github.com/mantidproject/mantid/pull/41997>`_) The old ISIS SANS reduction now functions correctly when the sample workspace carries X error (``Dx``) values. Passing those values on to the can-subtracted workspace now correctly copies the values instead of raising an ``AttributeError``.
- (`#42084 <https://github.com/mantidproject/mantid/pull/42084>`_) BIOSANS data now loads with its instrument parameters attached, so :ref:`algm-SolidAngle` works again. The instrument parameters had been missing since v6.16 whenever the instrument was loaded under its short name ``CG3``.
- (`#42236 <https://github.com/mantidproject/mantid/pull/42236>`_) In the :ref:`ISIS SANS GUI <ISIS_Sans_interface_contents>`, reloading a batch file saved from the interface now restores the sample shape and related options that were entered.

:ref:`Release 7.0.0 <v7.0.0>`
