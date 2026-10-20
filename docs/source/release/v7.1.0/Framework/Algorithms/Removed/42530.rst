- The following deprecated algorithms have now been removed:

  - ``LoadSINQFocus`` (deprecated 2013-10-28). Use :ref:`algm-LoadSINQ` instead.
  - ``LoadPreNexus`` (deprecated 2025-04-10). There is no replacement.
  - ``LoadPreNexusMonitors`` (deprecated 2025-04-10). There is no replacement.
  - ``LoadEventPreNexus`` (deprecated 2025-05-06). There is no replacement.
  - ``LoadPreNexusLive`` (deprecated 2025-05-27), which depended on ``LoadEventPreNexus``. There is no replacement.
- :ref:`algm-DetermineChunking` no longer accepts SNS ``_runinfo.xml`` (PreNeXus) files.
- :ref:`algm-DgsReduction` no longer loads monitors from PreNeXus ``_neutron_event.dat`` files when determining the incident energy.
