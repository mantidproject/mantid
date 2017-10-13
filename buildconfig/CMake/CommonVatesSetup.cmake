# Setup common things for the Vates subprojects

include ( SetMantidSubprojects )

set_mantid_subprojects (
Framework/API
Framework/Geometry
Framework/HistogramData
Framework/Types
Framework/Kernel
Framework/DataObjects
)

set ( COMMONVATES_SETUP_DONE TRUE )
