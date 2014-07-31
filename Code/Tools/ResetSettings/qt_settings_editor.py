#!/usr/bin/python

## Qt settings editor
## Dan Nixon
## 31/07/2014

## This file is part of the Mantid project

## For usage run with -h parameter

import sys, argparse
from PyQt4.QtCore import QSettings

def print_all_keys(settings):
    keys = settings.allKeys()
    for k in keys:
	value = settings.value(k).toString()
	if value == "":
	    value = "[obj]"
	sys.stdout.write("%s = %s\n" % (k, value))

def get_verification(notify_string):
    sys.stdout.write("%s\nAre you sure (y/N): " % (notify_string))
    response = raw_input();

    if len(response) == 0:
	return False
    return response.upper()[0] == "Y"

def run(props):
    # Create a new Qt settings object using supplied application
    # and organisation name
    settings = QSettings(props.org, props.app)

    # Have the clear preferences parameter
    if props.clear:
	# Check if we can clear by force
	ok_to_clear = props.force
	# If not ask the user if we can
	if not ok_to_clear:
	    ok_to_clear = get_verification(
		    "This will clear all Qt preferences for %s (by %s)" % (props.app, props.org))
	
	# If so clear all Qt preferences in the selected application
	# and report back unless quiet is enabled
	if ok_to_clear:
	    settings.clear()
	    if not props.quiet:
	        sys.stdout.write("Qt preferences cleared\n")
	else:
	    if not props.quiet:
		sys.stdout.write("Qt preferences not cleared\n")

    # Have the set preference parameter
    if props.set != None:
	# Input data is in the form k=v, split this into a list of k and v
	data = props.set.split("=")

	# Check if we can set by force
	ok_to_set = props.force
	# If not ask the user if we can
	if not ok_to_set:
	    ok_to_set = get_verification(
		    "This will set Qt preference %s to %s for %s (by %s)" %
		    (data[0], data[1], props.app, props.org))

	if ok_to_set:
	    settings.setValue(data[0], data[1])
	    if not props.quiet:
		sys.stdout.write("Set preference %s to %s\n" % (data[0], data[1]))
	else:
	    if not props.quiet:
		sys.stdout.write("Preference not set\n")

    # Have the list all preferences parameter
    if props.list and not props.quiet:
	sys.stdout.write("All Keys\n")
	print_all_keys(settings)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Tool to modify Qt preferences for MantidPlot (or any other Qt application)"
    )

    parser.add_argument(
	'-c', '--clear',
	action='store_true',
	help="Clears all Qt settings"
    )

    parser.add_argument(
	'-s', '--set',
	action='store',
	metavar='PROPERTY=VALUE',
	help="Sets a Qt property"
    )

    parser.add_argument(
	'-l', '--list',
	action='store_true',
	help="Lists all Qt properties and their values"
    )

    parser.add_argument(
	'-f', '--force',
	action='store_true',
	help="Will not ask for permission to do destructive tasks"
    )

    parser.add_argument(
	'-q', '--quiet',
	action='store_true',
	help="Will not print to stdout"
    )

    parser.add_argument(
	'-o', '--org',
	action='store',
	default='Mantid',
	metavar='ORGANISATION',
	help='Organisation name to pass to QSettings'
    )

    parser.add_argument(
	'-a', '--app',
	action='store',
	default='MantidPlot',
	metavar='APPLICATION',
	help='Application name to pass to QSettings'
    )

    props = parser.parse_args()

    # Check not run with no command parameters
    if props.set == None and not props.clear and not props.list:
	parser.print_usage()
    else:
	run(props)
