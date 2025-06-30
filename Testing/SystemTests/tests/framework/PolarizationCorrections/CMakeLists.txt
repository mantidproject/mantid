# System tests for the Polarization Corrections algorithms.

set(SYSTEST_NAMES DepolarizedAnalyserTransmissionTest.py FlipperEfficiencyTest.py HeliumAnalyserEfficiencyTest.py
                  PolarizerEfficiencyTest.py
)

pysystemtest_add_test(${SYSTEST_NAMES})
