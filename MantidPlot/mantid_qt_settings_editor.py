#!/usr/bin/python

# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#

import sys
from PyQt4.QtCore import QSettings

try:
    from argparse import ArgumentParser as Parser
    have_argparse = True
except ImportError:
    from optparse import OptionParser as Parser
    have_argparse = False

def print_all_keys(settings):
    keys = settings.allKeys()

    if len(keys) == 0:
        sys.stdout.write("No properties\n")

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
        ok_to_set = props["force"]
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
        sys.stdout.write("All Keys:\n")
        print_all_keys(settings)

if __name__ == "__main__":
    parser = Parser(
            description="Tool to modify Qt preferences for MantidPlot (or any other Qt application)"
            )

    if have_argparse:
        add_arg = parser.add_argument
    else:
        add_arg = parser.add_option

    add_arg(
            '-c', '--clear',
            action='store_true',
            help="Clears all Qt settings"
            )

    add_arg(
            '-s', '--set',
            action='store',
            metavar='PROPERTY=VALUE',
            help="Sets a Qt property"
            )

    add_arg(
            '-l', '--list',
            action='store_true',
            help="Lists all Qt properties and their values"
            )

    add_arg(
            '-f', '--force',
            action='store_true',
            help="Will not ask for permission to do destructive tasks"
            )

    add_arg(
            '-q', '--quiet',
            action='store_true',
            help="Will not print to stdout"
            )

    add_arg(
            '-o', '--org',
            action='store',
            default='Mantid',
            metavar='ORGANISATION',
            help='Organisation name to pass to QSettings'
            )

    add_arg(
            '-a', '--app',
            action='store',
            default='MantidPlot',
            metavar='APPLICATION',
            help='Application name to pass to QSettings'
            )

    if have_argparse:
        props = parser.parse_args()
    else:
        (props, extra_args) = parser.parse_args()

    # Check not run with no command parameters
    if props.set == None and not props.clear and not props.list:
        parser.print_usage()
    else:
        run(props)
