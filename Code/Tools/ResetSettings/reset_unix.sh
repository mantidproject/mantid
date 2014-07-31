#!/bin/bash

## Script to reset all Mantid settings on a Unix system
## Dan Nixon
## 31/07/2014

## This file is part of the Mantid project

pref_file=~/.mantid/Mantid.user.properties

# If the user properties file exists then remove it
if [ -f $pref_file ]
then
  echo "Removing "$pref_file
  rm $pref_file
else
  echo $pref_file" not found"
fi

# Remove all Qt preferences for MantidPlot
python ./qt_settings_editor.py -fc
