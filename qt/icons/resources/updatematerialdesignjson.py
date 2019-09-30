# Script from https://github.com/spyder-ide/qtawesome/commit/4985bd6e9bb75824de2d2f789fe5fbbf4016b823
# script used to create charmap
import re
import json
from collections import OrderedDict

with open('materialdesignicons.css', 'r') as fp:
    rawcss = fp.read()

charmap = OrderedDict()
pattern = r'^\.mdi-(.+):before {\s*content: "(.+)";\s*}$'
data = re.findall(pattern, rawcss, re.MULTILINE)
for name, key in data:
    key = key.replace('\\F', '0xf').lower()
    key = key.replace('\\', '0x')
    name = name.lower()
    charmap[name] = key

print(len(charmap))

with open('materialdesignicons-webfont-charmap.json', 'w') as fp:
    json.dump(charmap, fp, indent=4)
