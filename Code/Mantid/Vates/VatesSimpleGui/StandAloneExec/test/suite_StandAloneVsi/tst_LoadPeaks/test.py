
def main():
    source(findFile("scripts", "test_helpers.py"))
    source(findFile("scripts", "common_checks.py"))
    startApplication("VatesSimpleGui")
    open_file("TOPAZ_3680_10_sec_40.peaks")
    check_mode_buttons(std=False, ms=False, ts=False, sp=False)
    check_time_controls(False)
    quit_program()

