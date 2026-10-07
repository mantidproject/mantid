============
SANS Changes
============

New Features
------------
- (`#42054 <https://github.com/mantidproject/mantid/pull/42054>`_) :ref:`Runs Tab <ISIS_SANS_Runs_Tab-ref>` of the :ref:`ISIS SANS Interface<ISIS_Sans_interface_contents>` now has a `Clean Up ADS` checkbox to automatically delete all non-reduced workspaces (optimization, raw, transmission and their monitors) from the ADS at the end of a Batch Reduction.
- (`#42054 <https://github.com/mantidproject/mantid/pull/42054>`_) :ref:`BatchReduce <SANSScriptingBatchReduce>` function on the :ref:`SANS ISIS Command Interface<ScriptingSANSReductions>` now has a `clean_up_ads` parameter to automatically delete all non-reduced workspaces (optimization, raw, transmission and their monitors) from the ADS at the end of a Batch Reduction.
-  :ref:`ISIS SANS TOML <sans_toml_v1-ref>` documentation has been updated with a toml code block example for polarization fields.

Bugfixes
--------
- (`#41877 <https://github.com/mantidproject/mantid/pull/41877>`_) The ISIS SANS Command Interface selects the correct range for transmission fitting when using the `TransFit` command for every container selector.
- (`#41971 <https://github.com/mantidproject/mantid/pull/41971>`_) 'AddRuns' command of the :ref:`ISIS Command Interface <ScriptingSANSReductions>` and the  `Sum Runs` tab of the :ref:`ISIS SANS GUI <ISIS_Sans_interface_contents>` now correctly use a custom save directory to save the added runs.
- (`#41997 <https://github.com/mantidproject/mantid/pull/41997>`_) Fixed an error in the old ISIS SANS reduction when the sample workspace carries X error (``Dx``) values. Passing those values on to the can-subtracted workspace called two methods that do not exist on :class:`~mantid.api.MatrixWorkspace` (``getNumHistograms`` and ``dataDX``), which raised an ``AttributeError`` instead of copying the values.
- (`#42084 <https://github.com/mantidproject/mantid/pull/42084>`_) BIOSANS data now loads with its instrument parameters attached, so :ref:`algm-SolidAngle` works again. The instrument parameters had been missing since v6.16 whenever the instrument was loaded under its short name ``CG3``.
- (`#42236 <https://github.com/mantidproject/mantid/pull/42236>`_) In the :ref:`ISIS SANS GUI <ISIS_Sans_interface_contents>`, reloading a batch file saved from the GUI now restores sample shape and any options that were entered.

:ref:`Release 7.0.0 <v7.0.0>`
