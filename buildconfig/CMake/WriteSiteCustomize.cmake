# CMake script to generate a Python sitecustomize.py file at build time It is
# intended to be used via cmake -P (script mode)

file(
  WRITE ${SITECUSTOMIZE_DIR}/sitecustomize.py
  "
import site
site.addsitedir('${SITECUSTOMIZE_DIR}')
"
)
