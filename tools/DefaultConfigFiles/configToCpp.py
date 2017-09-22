## Converts a config file to C++ stream outputs
## Usage: configToCpp.py [stream] [config_file]

## Dan Nixon

from __future__ import (absolute_import, division, print_function)
import sys

with open(sys.argv[2]) as f:
    for line in f:
        line = line.rstrip('\n')
        if line == "":
            print(sys.argv[1] + " << std::endl;")
        else:
            print(sys.argv[1] + " << \"" + line + "\" << std::endl;")
