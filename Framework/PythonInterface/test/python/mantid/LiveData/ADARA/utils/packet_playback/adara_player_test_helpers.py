"""
Shared test helpers for ADARA packet-playback tests.

This module provides ``apply_test_config()`` which installs a test
configuration dict and initializes packet_player class-level constants,
without touching the filesystem.

Usage from unittest.TestCase::

    from adara_player_test_helpers import apply_test_config
    import player_config

    class MyTest(unittest.TestCase):
        def setUp(self):
            apply_test_config()

        def tearDown(self):
            player_config.reset()

        def test_with_overrides(self):
            apply_test_config({"playback.rate": "normal"})
            player = Player()
"""

import copy

import player_config
import packet_player


DEFAULT_TEST_CONFIG = {
    "server": {
        "name": "adara_player",
        "address": "127.0.0.1:31415",
        "buffer_MB": 64,
        "socket_timeout": 1.0,
    },
    "source": {"address": "127.0.0.1:31415"},
    "playback": {
        "handshake": "none",
        "handshake_timeout": 10.0,
        "rate": "unlimited",
        "timestamps": {"offset": None},
        "ignore_packets": [],
        "packet_ordering": "none",
        "packet_glob": "*.adara",
    },
    "record": {
        "transfer_limit": 16000,
        "file_output": "multi_packet",
    },
    "summarize": {
        "timestamp_per_type": False,
        "timestamp_resolution": 0.017,
    },
    "logging": {
        "format": "%(asctime)s: %(levelname)s: %(message)s",
        "level": "INFO",
        "filename": "",
        "packet_summary": "type: {type:#06x}: {timestamp}, size: {size:>7}, CRC: {CRC:#010x}",
    },
}


def apply_test_config(overrides: dict[str, object] | None = None) -> dict:
    """
    Install a test configuration and initialize packet_player class constants.

    1. Calls ``player_config.reset()``
    2. Deep-copies ``DEFAULT_TEST_CONFIG``, applies *overrides*
    3. Installs the dict via ``player_config._load_from_dict()``
    4. Calls ``packet_player.initialize()``

    Returns the installed config dict.

    *overrides* uses dotted keys, e.g.::

        apply_test_config({"playback.rate": "normal", "server.buffer_MB": 2})
    """
    cfg = copy.deepcopy(DEFAULT_TEST_CONFIG)

    if overrides:
        for dotted_key, value in overrides.items():
            keys = dotted_key.split(".")
            d = cfg
            for k in keys[:-1]:
                d = d[k]
            d[keys[-1]] = value

    player_config.reset()
    player_config._load_from_dict(cfg)
    packet_player.initialize()
    return cfg
