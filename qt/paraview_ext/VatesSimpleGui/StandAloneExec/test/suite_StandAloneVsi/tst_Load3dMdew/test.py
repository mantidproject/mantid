
def main():
    source(findFile("scripts", "test_helpers.py"))
    source(findFile("scripts", "common_checks.py"))
    startApplication("VatesSimpleGui")
    open_file("TOPAZ_3680_5_sec_MDEW.nxs")
    check_mode_buttons(std=False, ms=True, ts=True, sp=True)
    check_time_controls(False)
    quit_program()

