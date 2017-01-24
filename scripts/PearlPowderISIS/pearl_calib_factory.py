# pylint: disable=too-many-branches,too-many-statements, superfluous-parens

from mantid.simpleapi import *


def get_calibration_dir(cycle, tt_mode, pearl_file_dir):
    if (cycle == "15_4"):
        instver = "new2"
        calfile = pearl_file_dir + "pearl_offset_15_3.cal"
        vabsorbfile = pearl_file_dir + "pearl_absorp_sphere_10mm_newinst2_long.nxs"

        if (tt_mode == "TT88"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_15_4.nxs"
        elif (tt_mode == "TT70"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT70.cal"
            vanfile = pearl_file_dir + "van_spline_TT70_cycle_15_4.nxs"
        elif (tt_mode == "TT35"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT35.cal"
            vanfile = pearl_file_dir + "van_spline_TT35_cycle_15_4.nxs"
        else:
            print "Sorry I don't know that Two Theta mode so assuming T88"
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_15_4.nxs"
    elif (cycle == "15_3"):
        instver = "new2"
        calfile = pearl_file_dir + "pearl_offset_15_3.cal"
        vabsorbfile = pearl_file_dir + "pearl_absorp_sphere_10mm_newinst2_long.nxs"

        if (tt_mode == "TT88"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_15_3.nxs"
        elif (tt_mode == "TT70"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT70.cal"
            vanfile = pearl_file_dir + "van_spline_TT70_cycle_15_3.nxs"
        elif (tt_mode == "TT35"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT35.cal"
            vanfile = pearl_file_dir + "van_spline_TT35_cycle_15_3.nxs"
        else:
            print "Sorry I don't know that Two Theta mode so assuming T88"
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_15_3.nxs"
    elif (cycle == "15_2"):
        instver = "new2"
        calfile = pearl_file_dir + "pearl_offset_15_2.cal"
        vabsorbfile = pearl_file_dir + "pearl_absorp_sphere_10mm_newinst2_long.nxs"

        if (tt_mode == "TT88"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_15_2.nxs"
        elif (tt_mode == "TT70"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT70.cal"
            vanfile = pearl_file_dir + "van_spline_TT70_cycle_15_2.nxs"
        elif (tt_mode == "TT35"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT35.cal"
            vanfile = pearl_file_dir + "van_spline_TT35_cycle_15_2.nxs"
        else:
            print "Sorry I don't know that Two Theta mode so assuming T88"
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_15_2.nxs"
    elif (cycle == "15_1"):
        instver = "new2"
        calfile = pearl_file_dir + "pearl_offset_15_1.cal"
        vabsorbfile = pearl_file_dir + "pearl_absorp_sphere_10mm_newinst2_long.nxs"

        if (tt_mode == "TT88"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_15_1.nxs"
        elif (tt_mode == "TT70"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT70.cal"
            vanfile = pearl_file_dir + "van_spline_TT70_cycle_15_1.nxs"
        elif (tt_mode == "TT35"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT35.cal"
            vanfile = pearl_file_dir + "van_spline_TT35_cycle_15_1.nxs"
        else:
            print "Sorry I don't know that Two Theta mode so assuming T88"
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_15_1.nxs"
    elif (cycle == "14_3"):
        instver = "new2"
        calfile = pearl_file_dir + "pearl_offset_14_3.cal"
        vabsorbfile = pearl_file_dir + "pearl_absorp_sphere_10mm_newinst2_long.nxs"

        if (tt_mode == "TT88"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_14_3.nxs"
        elif (tt_mode == "TT70"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT70.cal"
            vanfile = pearl_file_dir + "van_spline_TT70_cycle_14_3.nxs"
        elif (tt_mode == "TT35"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT35.cal"
            vanfile = pearl_file_dir + "van_spline_TT35_cycle_14_3.nxs"
        else:
            print "Sorry I don't know that Two Theta mode so assuming T88"
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_14_3.nxs"
    elif (cycle == "14_2"):
        instver = "new2"
        calfile = pearl_file_dir + "pearl_offset_14_2.cal"
        vabsorbfile = pearl_file_dir + "pearl_absorp_sphere_10mm_newinst2_long.nxs"

        if (tt_mode == "TT88"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_14_2.nxs"
        elif (tt_mode == "TT70"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT70.cal"
            vanfile = pearl_file_dir + "van_spline_TT70_cycle_14_2.nxs"
        elif (tt_mode == "TT35"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT35.cal"
            vanfile = pearl_file_dir + "van_spline_TT35_cycle_14_2.nxs"
        else:
            print "Sorry I don't know that Two Theta mode so assuming T88"
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_14_2.nxs"
    elif (cycle == "14_1"):
        instver = "new2"
        calfile = pearl_file_dir + "pearl_offset_14_1.cal"
        vabsorbfile = pearl_file_dir + "pearl_absorp_sphere_10mm_newinst2_long.nxs"

        if (tt_mode == "TT88"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_14_1.nxs"
        elif (tt_mode == "TT70"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT70.cal"
            vanfile = pearl_file_dir + "van_spline_TT70_cycle_14_1.nxs"
        elif (tt_mode == "TT35"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT35.cal"
            vanfile = pearl_file_dir + "van_spline_TT35_cycle_14_1.nxs"
        else:
            print "Sorry I don't know that Two Theta mode so assuming T88"
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_14_1.nxs"
    elif (cycle == "13_5"):
        instver = "new2"
        calfile = pearl_file_dir + "pearl_offset_13_5.cal"
        vabsorbfile = pearl_file_dir + "pearl_absorp_sphere_10mm_newinst2_long.nxs"

        if (tt_mode == "TT88"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_13_5.nxs"
        elif (tt_mode == "TT70"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT70.cal"
            vanfile = pearl_file_dir + "van_spline_TT70_cycle_13_5.nxs"
        elif (tt_mode == "TT35"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT35.cal"
            vanfile = pearl_file_dir + "van_spline_TT35_cycle_13_5.nxs"
        else:
            print "Sorry I don't know that Two Theta mode so assuming T88"
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_13_5.nxs"
    elif (cycle == "13_4"):
        instver = "new2"
        calfile = pearl_file_dir + "pearl_offset_13_4.cal"
        vabsorbfile = pearl_file_dir + "pearl_absorp_sphere_10mm_newinst2_long.nxs"

        if (tt_mode == "TT88"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_13_4.nxs"
        elif (tt_mode == "TT70"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT70.cal"
            vanfile = pearl_file_dir + "van_spline_TT70_cycle_13_4.nxs"
        elif (tt_mode == "TT35"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT35.cal"
            vanfile = pearl_file_dir + "van_spline_TT35_cycle_13_4.nxs"
        else:
            print "Sorry I don't know that Two Theta mode so assuming T88"
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_13_4.nxs"
    elif (cycle == "13_3"):
        instver = "new2"
        calfile = pearl_file_dir + "pearl_offset_13_3.cal"
        vabsorbfile = pearl_file_dir + "pearl_absorp_sphere_10mm_newinst2_long.nxs"

        if (tt_mode == "TT88"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_13_3.nxs"
        elif (tt_mode == "TT70"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT70.cal"
            vanfile = pearl_file_dir + "van_spline_TT70_cycle_13_3.nxs"
        elif (tt_mode == "TT35"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT35.cal"
            vanfile = pearl_file_dir + "van_spline_TT35_cycle_13_3.nxs"
        else:
            print "Sorry I don't know that Two Theta mode so assuming T88"
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_13_3.nxs"
    elif (cycle == "13_2"):
        instver = "new2"
        calfile = pearl_file_dir + "pearl_offset_13_2.cal"
        vabsorbfile = pearl_file_dir + "pearl_absorp_sphere_10mm_newinst2_long.nxs"

        if (tt_mode == "TT88"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_13_2.nxs"
        elif (tt_mode == "TT70"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT70.cal"
            vanfile = pearl_file_dir + "van_spline_TT70_cycle_13_2.nxs"
        elif (tt_mode == "TT35"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT35.cal"
            vanfile = pearl_file_dir + "van_spline_TT35_cycle_13_2.nxs"
        else:
            print "Sorry I don't know that Two Theta mode so assuming T88"
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_13_2.nxs"
    elif (cycle == "13_1"):
        instver = "new2"
        calfile = pearl_file_dir + "pearl_offset_13_1.cal"
        vabsorbfile = pearl_file_dir + "pearl_absorp_sphere_10mm_newinst2_long.nxs"

        if (tt_mode == "TT88"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_13_1.nxs"
        elif (tt_mode == "TT70"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT70.cal"
            vanfile = pearl_file_dir + "van_spline_TT70_cycle_13_1.nxs"
        elif (tt_mode == "TT35"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT35.cal"
            vanfile = pearl_file_dir + "van_spline_TT35_cycle_13_1.nxs"
        else:
            print "Sorry I don't know that Two Theta mode so assuming T88"
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_13_1.nxs"
    elif (cycle == "12_5"):
        instver = "new2"
        calfile = pearl_file_dir + "pearl_offset_12_5.cal"
        vabsorbfile = pearl_file_dir + "pearl_absorp_sphere_10mm_newinst2_long.nxs"

        if (tt_mode == "TT88"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_12_5.nxs"
        elif (tt_mode == "TT70"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT70.cal"
            vanfile = pearl_file_dir + "van_spline_TT70_cycle_12_5.nxs"
        elif (tt_mode == "TT35"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT35.cal"
            vanfile = pearl_file_dir + "van_spline_TT35_cycle_12_5.nxs"
        else:
            print "Sorry I don't know that Two Theta mode so assuming T88"
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_12_5.nxs"
    elif (cycle == "12_4"):
        instver = "new2"
        calfile = pearl_file_dir + "pearl_offset_12_4.cal"
        vabsorbfile = pearl_file_dir + "pearl_absorp_sphere_10mm_newinst2_long.nxs"

        if (tt_mode == "TT88"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_12_4.nxs"
        elif (tt_mode == "TT70"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT70.cal"
            vanfile = pearl_file_dir + "van_spline_TT70_cycle_12_4.nxs"
        elif (tt_mode == "TT35"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT35.cal"
            vanfile = pearl_file_dir + "van_spline_TT35_cycle_12_4.nxs"
        else:
            print "Sorry I don't know that Two Theta mode so assuming T88"
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_12_4.nxs"
    elif (cycle == "12_3"):
        instver = "new2"
        calfile = pearl_file_dir + "pearl_offset_12_3.cal"
        vabsorbfile = pearl_file_dir + "pearl_absorp_sphere_10mm_newinst2_long.nxs"

        if (tt_mode == "TT88"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_12_3.nxs"
        elif (tt_mode == "TT70"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT70.cal"
            vanfile = pearl_file_dir + "van_spline_TT70_cycle_12_3.nxs"
        elif (tt_mode == "TT35"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT35.cal"
            vanfile = pearl_file_dir + "van_spline_TT35_cycle_12_3.nxs"
        else:
            print "Sorry I don't know that Two Theta mode so assuming T88"
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_12_3.nxs"
    elif (cycle == "12_2"):
        instver = "new2"
        calfile = pearl_file_dir + "pearl_offset_12_2.cal"
        vabsorbfile = pearl_file_dir + "pearl_absorp_sphere_10mm_newinst2_long.nxs"

        if (tt_mode == "TT88"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_12_2.nxs"
        elif (tt_mode == "TT70"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT70.cal"
            vanfile = pearl_file_dir + "van_spline_TT70_cycle_12_2.nxs"
        elif (tt_mode == "TT35"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT35.cal"
            vanfile = pearl_file_dir + "van_spline_TT35_cycle_12_2.nxs"
        else:
            print "Sorry I don't know that Two Theta mode so assuming T88"
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_12_2.nxs"
    elif (cycle == "12_1"):
        instver = "new2"
        calfile = pearl_file_dir + "pearl_offset_12_1.cal"
        vabsorbfile = pearl_file_dir + "pearl_absorp_sphere_10mm_newinst2_long.nxs"

        if (tt_mode == "TT88"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_12_1.nxs"
        elif (tt_mode == "TT70"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT70.cal"
            vanfile = pearl_file_dir + "van_spline_TT70_cycle_12_1.nxs"
        elif (tt_mode == "TT35"):
            groupfile = pearl_file_dir + "pearl_group_12_1_TT35.cal"
            vanfile = pearl_file_dir + "van_spline_TT35_cycle_12_1.nxs"
        else:
            print "Sorry I don't know that Two Theta mode so assuming T88"
            groupfile = pearl_file_dir + "pearl_group_12_1_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_12_1.nxs"
    elif (cycle == "11_5"):
        instver = "new"
        calfile = pearl_file_dir + "pearl_offset_11_5.cal"
        vabsorbfile = pearl_file_dir + "pearl_absorp_sphere_10mm_newinst_long.nxs"

        if (tt_mode == "TT88"):
            groupfile = pearl_file_dir + "pearl_group_11_2_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_11_5.nxs"
        elif (tt_mode == "TT70"):
            groupfile = pearl_file_dir + "pearl_group_11_2_TT70.cal"
            vanfile = pearl_file_dir + "van_spline_TT70_cycle_11_5.nxs"
        elif (tt_mode == "TT35"):
            groupfile = pearl_file_dir + "pearl_group_11_2_TT35.cal"
            vanfile = pearl_file_dir + "van_spline_TT35_cycle_11_5.nxs"
        else:
            print "Sorry I don't know that Two Theta mode so assuming T88"
            groupfile = pearl_file_dir + "pearl_group_11_2_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_11_5.nxs"
    elif (cycle == "11_4"):
        instver = "new"
        calfile = pearl_file_dir + "pearl_offset_11_4.cal"
        vabsorbfile = pearl_file_dir + "pearl_absorp_sphere_10mm_newinst_long.nxs"

        if (tt_mode == "TT88"):
            groupfile = pearl_file_dir + "pearl_group_11_2_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_11_4.nxs"
        elif (tt_mode == "TT70"):
            groupfile = pearl_file_dir + "pearl_group_11_2_TT70.cal"
            vanfile = pearl_file_dir + "van_spline_TT70_cycle_11_4.nxs"
        elif (tt_mode == "TT35"):
            groupfile = pearl_file_dir + "pearl_group_11_2_TT35.cal"
            vanfile = pearl_file_dir + "van_spline_TT35_cycle_11_4.nxs"
        else:
            print "Sorry I don't know that Two Theta mode so assuming T88"
            groupfile = pearl_file_dir + "pearl_group_11_2_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_11_4.nxs"
    elif (cycle == "11_3"):
        instver = "new"
        calfile = pearl_file_dir + "pearl_offset_11_3.cal"
        vabsorbfile = pearl_file_dir + "pearl_absorp_sphere_10mm_newinst_long.nxs"

        if (tt_mode == "TT88"):
            groupfile = pearl_file_dir + "pearl_group_11_2_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_11_3.nxs"
        elif (tt_mode == "TT70"):
            groupfile = pearl_file_dir + "pearl_group_11_2_TT70.cal"
            vanfile = pearl_file_dir + "van_spline_TT70_cycle_11_3.nxs"
        elif (tt_mode == "TT35"):
            groupfile = pearl_file_dir + "pearl_group_11_2_TT35.cal"
            vanfile = pearl_file_dir + "van_spline_TT35_cycle_11_3.nxs"
        else:
            print "Sorry I don't know that Two Theta mode so assuming T88"
            groupfile = pearl_file_dir + "pearl_group_11_2_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_11_3.nxs"
    elif (cycle == "11_2"):
        instver = "new"
        calfile = pearl_file_dir + "pearl_offset_11_2.cal"
        vabsorbfile = pearl_file_dir + "pearl_absorp_sphere_10mm_newinst.nxs"
        vanfile = pearl_file_dir + "van_spline_mods_cycle_11_2.nxs"
        if (tt_mode == "TT88"):
            groupfile = pearl_file_dir + "pearl_group_11_2_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_11_2.nxs"
        elif (tt_mode == "TT70"):
            groupfile = pearl_file_dir + "pearl_group_11_2_TT70.cal"
            vanfile = pearl_file_dir + "van_spline_TT70_cycle_11_2.nxs"
        elif (tt_mode == "TT35"):
            groupfile = pearl_file_dir + "pearl_group_11_2_TT35.cal"
            vanfile = pearl_file_dir + "van_spline_TT35_cycle_11_2.nxs"
        else:
            print "Sorry I don't know that Two Theta mode so assuming T88"
            groupfile = pearl_file_dir + "pearl_group_11_2_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_11_2.nxs"
    elif (cycle == "11_1"):
        instver = "new"
        calfile = pearl_file_dir + "pearl_offset_11_2.cal"
        vabsorbfile = pearl_file_dir + "pearl_absorp_sphere_10mm_newinst_11_1.nxs"
        if (tt_mode == "TT88"):
            groupfile = pearl_file_dir + "pearl_group_11_2_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_11_1.nxs"
        elif (tt_mode == "TT70"):
            groupfile = pearl_file_dir + "pearl_group_11_2_TT70.cal"
            vanfile = pearl_file_dir + "van_spline_TT70_cycle_11_1.nxs"
        elif (tt_mode == "TT35"):
            groupfile = pearl_file_dir + "pearl_group_11_2_TT35.cal"
            vanfile = pearl_file_dir + "van_spline_TT35_cycle_11_1.nxs"
        else:
            print "Sorry I don't know that Two Theta mode so assuming T88"
            groupfile = pearl_file_dir + "pearl_group_11_2_TT88.cal"
            vanfile = pearl_file_dir + "van_spline_TT88_cycle_11_1.nxs"
    elif (cycle == "10_2"):
        instver = "old"
        calfile = pearl_file_dir + "pearl_offset_10_2.cal"
        groupfile = pearl_file_dir + "pearl_group_10_2.cal"
        vabsorbfile = pearl_file_dir + "pearl_absorp_sphere_10mm_all.nxs"
        vanfile = pearl_file_dir + "test_van_new_cycle_10_2.nxs"
    else:
        print "Sorry that cycle has not been defined yet"

    return calfile, groupfile, vabsorbfile, vanfile, instver
