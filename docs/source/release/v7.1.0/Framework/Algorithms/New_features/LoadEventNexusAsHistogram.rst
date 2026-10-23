- Added :ref:`LoadEventNexusAsHistogram <algm-LoadEventNexusAsHistogram>`, which bins event NeXus data directly into
  a histogram workspace without first constructing an event workspace. It can overlap serial NeXus reads with
  parallel histogramming, reads event times in their native numeric type, and selects safe native-unit binning and
  compact histogram counters automatically. The processing strategy, read size, and pipeline memory usage can be
  tuned for different instruments and systems.
