# System tests for the Polarization Corrections algorithms.

set(SYSTEST_NAMES DepolarizedAnalyserTransmissionTest FlipperEfficiencyTest HeliumAnalyserEfficiencyTest
                  PolarizerEfficiencyTest
)

pysystemtest_add_test(${SYSTEST_NAMES})
