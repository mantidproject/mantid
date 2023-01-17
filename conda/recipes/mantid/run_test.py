# Turn off network required things that the framework does
from mantid.kernel import ConfigService

ConfigService["UpdateInstrumentDefinitions.OnStartup"] = "0"
ConfigService["usagereports.enabled"] = "0"
ConfigService["CheckMantidVersion.OnStartup"] = "0"

import mantid.simpleapi

assert "Rebin" in dir(mantid.simpleapi), "Rebin not found in simpleapi!"
