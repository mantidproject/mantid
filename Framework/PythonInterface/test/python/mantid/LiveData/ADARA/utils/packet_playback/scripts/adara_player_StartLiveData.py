import logging
import os
import time

from mantid.api import AlgorithmManager, mtd
from mantid.kernel import ConfigService
from mantid.simpleapi import StartLiveData

logger = logging.getLogger("adara_player_StartLiveData")

"""
Instructions:
-------------

In the following we assume that:

- your mantid build directory is ${BUILD}


1. Either build mantid, or make sure you're using a version that has an updated `SNSLiveEventDataListener`.

::

  mkdir ${BUILD}; cd ${BUILD}
  cmake .. -GNinja
  cmake --build . --target AllTests workbench StandardTestData dev-docs-html

2. In its own console: start `adara_player` in playback mode:

::

  # activate your `mantid-developer` environment
  mamba activate mantid-developer

  # the location of these scripts:
  SCRIPTS=${BUILD}/../Framework/LiveData/test/Python/scripts
  cd ${SCRIPTS} # or not, but otherwise you'll need to add to your `PYTHONPATH`

  # from your ${BUILD}/bin/launch_mantidworkbench.sh:
  export LOCAL_PRELOAD=${HOME}/mambaforge/envs/mantid-developer/lib/libjemalloc.so.2
  export LOCAL_PYTHONPATH=${BUILD}/bin/
  export adara_player_conf=${SCRIPTS}/adara_player_conf.yml

  echo "default SERVER_ADDRESS: ${XDG_RUNTIME_DIR}/sock-adara_player"
  LD_PRELOAD=${LOCAL_PRELOAD} PYTHONPATH=${LOCAL_PYTHONPATH} python adara_player.py --files=${SCRIPTS}/example_sessions.yml

3. In another console, or in `workbench`, run the script that calls `StartLiveData`:

::

  ... # setup your env vars as above
  LD_PRELOAD=${LOCAL_PRELOAD} PYTHONPATH=${LOCAL_PYTHONPATH} python ${SCRIPTS}/adara_player_StartLiveData.py

  # ... log output summarizing each loaded chunk ...
"""

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
