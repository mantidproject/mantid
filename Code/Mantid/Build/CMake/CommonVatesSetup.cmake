# Setup common things for the Vates subprojects

include ( SetMantidSubprojects )

set_mantid_subprojects (
Framework/API
Framework/CurveFitting
Framework/Geometry
Framework/Kernel
Framework/MDAlgorithms
Framework/MDDataObjects
)

set ( COMMONVATES_SETUP_DONE TRUE )
