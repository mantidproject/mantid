"""
Constants for instruments
"""

# ABINS internal constants
all_instruments = ["None", "TOSCA"]
all_sample_forms = ["SingleCrystal", "Powder"]
all_q_formats = ["scalars", "vectors"]
all_keywords_abins_data = ["weight", "value", "frequencies", "atomic_displacements"]

DW_critical_temperature = 20 # temperature in  K; if the temperature set in calculation is below or equal  this
# critical temperature then we have low temperature scenario
# and otherwise we have high temperature scenario.

# Instruments constants
TOSCA_constant = 1 / 16.0 # magic number for TOSCA...

