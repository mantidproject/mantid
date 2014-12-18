"""
   Defines a set functions for interrogating the current enviroment.

   The standard platform module doesn't have simple things like is_windows(),
   is_linux(). The set of functions defined here should make it clearer what is going
   on when they are used.
"""
import platform as _platform
import sys as _sys

def is_windows():
    """
       Returns True if the current platform is Windows (regardless of version/32- or 64-bit etc)
    """
    return _sys.platform == "win32"

def is_mac():
    """
        Returns True if the current platform is Mac (regardless of version/32- or 64-bit etc)
    """
    return _sys.platform == "darwin"

def is_linux():
    """
        Returns True if the current platform is OS X (regardless of version/32- or 64-bit etc)
        Variant on is_apple
    """
    return _sys.platform.startswith("linux")

def is_32bit():
    """
        Returns True if the current platform is 32-bit
    """
    if is_mac():
        # See Python documentation on platform module for why this is different
        return _sys.maxsize == 2**31 - 1 # Max size of integer
    else:
        bits = _platform.architecture()[0]
        return bits == '32bit'

def is_64bit():
    """
        Returns True if the current platform is 64-bit
    """
    if is_mac():
        # See Python documentation on platform module for why this is different
        return _sys.maxsize > 2**32
    else:
        bits = _platform.architecture()[0]
        return bits == '64bit'