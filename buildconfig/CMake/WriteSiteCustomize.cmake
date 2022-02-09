# CMake script to generate a Python sitecustomize.py file at build time It is intended to be used via cmake -P (script
# mode)
file(
  WRITE ${SITECUSTOMIZE_DIR}/sitecustomize_mantid.py
  "
\"\"\"
Add bin/<config> as a site directory so that .pth files in it get processed
Setuptools develop mode (=pip editable install) relies on this to find
python packages in the source directories
Can't rely on .egg-link files any more because not supported by importlib
which is successor to the deprecated pkgresources
\"\"\"
import site
site.addsitedir('${SITECUSTOMIZE_DIR}')
"
)
