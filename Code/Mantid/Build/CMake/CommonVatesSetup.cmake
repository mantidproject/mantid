# Setup common things for the Vates subprojects

include ( SetMantidSubprojects )

set_mantid_subprojects (
Framework/API
Framework/CurveFitting
Framework/Geometry
Framework/Kernel
Framework/Algorithms
Framework/DataObjects
Framework/DataHandling
Framework/MDEvents
Framework/MDAlgorithms
Framework/MDDataObjects
Framework/Nexus
)

set ( COMMONVATES_SETUP_DONE TRUE )
