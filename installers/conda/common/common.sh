# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# Take an installed workbench conda environment
# and trim out as much as possible that leaves
# it functioning
# Arguments:
#   - $1 Base of the installed conda environment
function trim_conda() {
  local bundle_conda_prefix=$1
  echo "Purging '$bundle_conda_prefix' of unnecessary items"

  # Heavily cut down bin
  mv "$bundle_conda_prefix"/bin "$bundle_conda_prefix"/bin_tmp
  mkdir "$bundle_conda_prefix"/bin
  cp "$bundle_conda_prefix"/bin_tmp/python* "$bundle_conda_prefix"/bin/
  cp "$bundle_conda_prefix"/bin_tmp/Mantid.properties "$bundle_conda_prefix"/bin/
  cp "$bundle_conda_prefix"/bin_tmp/mantid-scripts.pth "$bundle_conda_prefix"/bin/
  cp "$bundle_conda_prefix"/bin_tmp/workbench "$bundle_conda_prefix"/bin/
  if [ -f "$bundle_conda_prefix"/bin_tmp/launch_mantidworkbench.standalone ]; then
    # Keep handwritten startup script used on Linux so that we use jemalloc
    cp "$bundle_conda_prefix"/bin_tmp/launch_mantidworkbench.standalone "$bundle_conda_prefix"/bin/mantidworkbench
    # We don't use this workbench script in the Linux standalone, so remove it to avoid confusion
    rm "$bundle_conda_prefix"/bin/workbench
  fi
  # Heavily cut down share
  mv "$bundle_conda_prefix"/share "$bundle_conda_prefix"/share_tmp
  mkdir "$bundle_conda_prefix"/share
  mv "$bundle_conda_prefix"/share_tmp/doc "$bundle_conda_prefix"/share/
  mkdir -p "$bundle_conda_prefix"/share/glib-2.0/schemas
  mv "$bundle_conda_prefix"/share_tmp/glib-2.0/schemas "$bundle_conda_prefix"/share/glib-2.0/
  mv "$bundle_conda_prefix"/share_tmp/X11 "$bundle_conda_prefix"/share/
  # Heavily cut down translations
  mv "$bundle_conda_prefix"/translations "$bundle_conda_prefix"/translations_tmp
  mkdir -p "$bundle_conda_prefix"/translations/qtwebengine_locales
  mv "$bundle_conda_prefix"/translations_tmp/qtwebengine_locales/en*.pak \
    "$bundle_conda_prefix"/translations/qtwebengine_locales/

  # Removals
  rm -rf "$bundle_conda_prefix"/bin_tmp \
    "$bundle_conda_prefix"/include \
    "$bundle_conda_prefix"/man \
    "$bundle_conda_prefix"/mkspecs \
    "$bundle_conda_prefix"/phrasebooks \
    "$bundle_conda_prefix"/qml \
    "$bundle_conda_prefix"/qsci \
    "$bundle_conda_prefix"/share_tmp \
    "$bundle_conda_prefix"/translations_tmp

  find "$bundle_conda_prefix" -name '*.a' -delete
  find "$bundle_conda_prefix" -name "*.pyc" -type f -delete
  find "$bundle_conda_prefix" -path "*/__pycache__/*" -delete
  find "$bundle_contents" -name '*.plist' -delete
}

# Take an installed workbench conda environment
# and fix the qt installation. Generates a qt.conf
# containing a relative prefix path
# Arguments:
#   - $1 Base of the installed conda environment
#   - $2 qt.conf file to install
function fixup_qt() {
  local bundle_conda_prefix=$1
  local qt_conf=$2
  echo "Fixing Qt installation"

  find "$bundle_conda_prefix" -name 'qt.conf' -delete
  cp "$qt_conf" "$bundle_conda_prefix"/bin/qt.conf
  cp "$qt_conf" "$bundle_conda_prefix"/libexec/qt.conf
}
