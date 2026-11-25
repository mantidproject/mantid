"""
Calls `StartLiveData` against an `adara_player` running in playback mode.

For setup and usage, see the "Example playback mode session" section of
https://developer.mantidproject.org/Testing/LiveData/LiveDataPacketPlayback.html
(source: dev-docs/source/Testing/LiveData/LiveDataPacketPlayback.rst).
"""

import logging
import os
import time

from mantid.api import AlgorithmManager, mtd
from mantid.kernel import ConfigService
from mantid.simpleapi import StartLiveData

logger = logging.getLogger("adara_player_StartLiveData")

PROCESSING_SCRIPT = """
from mantid.api import mtd
from mantid.kernel import FloatTimeSeriesProperty


run = input.run()

event_ts = (input.getPulseTimeMin().to_datetime64(), input.getPulseTimeMax().to_datetime64())

pulse_ts = None
if run.hasProperty('proton_charge'):
    proton_charge = run.getProperty('proton_charge')
    if isinstance(proton_charge, FloatTimeSeriesProperty):
        pulse_ts = (min(proton_charge.times), max(proton_charge.times))

print(f"Loaded chunk: {input.getNumberEvents()} events")
if pulse_ts:
    print(f"  pulse-times: {pulse_ts[0]} -- {pulse_ts[1]}")
else:
    print("  pulse-times: N/A")
print(f"  event-times: {event_ts[0]} -- {event_ts[1]}")

output = input
"""


def captureLive():
    # We need the facility that has the `SNAP` instrument.
    ConfigService.setFacility("SNS")

    try:
        # get the default server address: this is set in `adara_player_conf.yml`
        SERVER_ADDRESS = f"{os.getenv('XDG_RUNTIME_DIR')}/sock-adara_player"

        # start a Live data listener updating every 20 seconds, that logs the pulse-time range for each chunk.
        StartLiveData(
            Instrument="SNAP",
            Address=SERVER_ADDRESS,
            OutputWorkspace="wsOut",
            UpdateEvery=10,
            ProcessingScript=PROCESSING_SCRIPT,
            AccumulationMethod="Replace",
            PreserveEvents=True,
        )

        # let it load a few chunks before stopping it
        time.sleep(40)
    finally:
        AlgorithmManager.cancelAll()
        time.sleep(1)


# --------------------------------------------------------------------------------------------------

oldFacility = ConfigService.getFacility().name()

try:
    # REMEMBER to start your `adara_player` in another console
    #   BEFORE you get to this point!
    captureLive()
except Exception as e:
    logger.error(f"Error occurred starting live data: {str(e)}")
else:
    # get the output workspace
    wsOut = mtd["wsOut"]
    logger.info("The 'wsOut' workspace contains %i events" % wsOut.getNumberEvents())
finally:
    # put back the facility
    ConfigService.setFacility(oldFacility)
