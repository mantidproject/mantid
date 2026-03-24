"""
Centralized configuration for the ADARA packet-playback feature.

This module owns the lifecycle of the configuration dict:

    1. ``load_config(path)``   — load from a YAML file (production use)
    2. ``get_config()``        — retrieve the loaded dict
    3. ``reset()``             — clear state (for tests, or before reload)

It also provides logging helpers that depend on the loaded config.

For testing, ``_load_from_dict(d)`` allows direct injection of a config
dict without touching the filesystem.
"""

import logging
import os
from pathlib import Path
import re
from typing import Any

import yaml


# ---------------------------------------------------------------------------
# Module-level state
# ---------------------------------------------------------------------------
_config: dict[str, Any] | None = None
_config_path: Path | None = None


# ---------------------------------------------------------------------------
# Custom TRACE log level (shared across all modules)
# ---------------------------------------------------------------------------

TRACE_LEVEL = 5
logging.addLevelName(TRACE_LEVEL, "TRACE")


def _trace(self, message, *args, **kws):
    if self.isEnabledFor(TRACE_LEVEL):
        self._log(TRACE_LEVEL, message, args, **kws)


logging.Logger.trace = _trace


# ---------------------------------------------------------------------------
# YAML environment-variable substitution
# ---------------------------------------------------------------------------

# Match patterns like ${ENV_VAR}
_env_pattern = re.compile(r".*?\${(.*?)}.*?")


def _env_constructor(loader, node):
    value = loader.construct_scalar(node)
    for group in _env_pattern.findall(value):
        group_value = os.environ.get(group)
        if group_value is None:
            raise RuntimeError(f"load from YAML requires '${{{group}}}': which is not defined in the environment")
        value = value.replace(f"${{{group}}}", group_value)
    return value


# Register once at import time — safe because SafeLoader is a class, not
# per-instance state, and the resolver + constructor are idempotent.
yaml.SafeLoader.add_implicit_resolver("!env", _env_pattern, None)
yaml.SafeLoader.add_constructor("!env", _env_constructor)


# ---------------------------------------------------------------------------
# Public YAML helper (also used by session_server for sessions.yml)
# ---------------------------------------------------------------------------


def yaml_load(file_path: Path) -> dict[str, Any]:
    """Load a YAML file with ``${ENV_VAR}`` substitution."""
    with open(file_path, "rt") as f:
        return yaml.safe_load(f)


# ---------------------------------------------------------------------------
# Config lifecycle
# ---------------------------------------------------------------------------


def load_config(config_path: str | Path) -> dict[str, Any]:
    """
    Load the configuration from *config_path* (a YAML file) and store it
    as the module-global ``_config``.

    Raises ``SystemExit`` if the file is missing or unparseable.
    Raises ``RuntimeError`` if called a second time without ``reset()``.
    """
    global _config, _config_path

    if _config is not None:
        raise RuntimeError(f"Config already loaded from '{_config_path}'; cannot reload from '{config_path}'. Call reset() first.")

    config_path = Path(config_path).resolve()
    if not config_path.exists():
        raise SystemExit(f"Config file '{config_path}' not found on filesystem.")

    try:
        _config = yaml_load(config_path)
    except Exception as e:
        raise SystemExit(f"Error when loading {config_path}: {e}")

    _config_path = config_path
    return _config


def _load_from_dict(d: dict[str, Any]) -> None:
    """
    Directly install *d* as the module-global config.

    Internal helper for tests that already have a config dict and don't
    need YAML parsing or filesystem access.
    """
    global _config, _config_path
    if _config is not None:
        raise RuntimeError(f"Config already loaded (from '{_config_path}'); call reset() before _load_from_dict().")
    _config = d
    _config_path = Path("<dict>")


def get_config() -> dict[str, Any]:
    """
    Return the loaded config dict.

    Raises ``RuntimeError`` if ``load_config()`` has not been called.
    """
    if _config is None:
        raise RuntimeError("Configuration has not been loaded. Call load_config(path) or _load_from_dict(d) first.")
    return _config


def is_loaded() -> bool:
    """Return ``True`` if the config has been loaded."""
    return _config is not None


def reset() -> None:
    """
    Clear the loaded config, allowing ``load_config()`` to be called
    again.  Intended for test teardown.
    """
    global _config, _config_path
    _config = None
    _config_path = None


# ---------------------------------------------------------------------------
# Logging helpers
# ---------------------------------------------------------------------------


def get_log_formatter() -> logging.Formatter:
    """Return a ``Formatter`` using the configured format string."""
    cfg = get_config()
    return logging.Formatter(cfg["logging"]["format"])


def get_log_file_path_for_PID(pid: int) -> Path | None:
    """Return the log-file path for *pid*, or ``None`` if unusable."""
    cfg = get_config()
    template = cfg["logging"]["filename"]
    if not template:
        return None
    path = Path(template.format(PID=pid))
    return path if path.parent.exists() else None


def configure_logger(
    logger: logging.Logger,
    *,
    pid: int | None = None,
    console: bool = False,
) -> None:
    """
    (Re-)configure *logger* from the loaded config.

    - Removes all existing handlers.
    - Adds a ``FileHandler`` (if the filename template is usable) and/or
      a ``StreamHandler`` (if *console* is ``True``).
    - Sets the logger level from config.
    """
    cfg = get_config()
    formatter = get_log_formatter()

    # Remove pre-existing handlers
    for h in list(logger.handlers):
        logger.removeHandler(h)
        try:
            h.close()
        except Exception:
            pass

    # File handler
    if pid is not None:
        log_path = get_log_file_path_for_PID(pid)
        if log_path is not None:
            fh = logging.FileHandler(log_path)
            fh.setFormatter(formatter)
            logger.addHandler(fh)

    # Console handler
    if console:
        ch = logging.StreamHandler()
        ch.setFormatter(formatter)
        logger.addHandler(ch)

    logger.setLevel(cfg["logging"]["level"])


def init_process_logger(logger: logging.Logger) -> None:
    """Configure *logger* for the current process (file handler only)."""
    configure_logger(logger, pid=os.getpid())
