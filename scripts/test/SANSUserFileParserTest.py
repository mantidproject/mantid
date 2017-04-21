import unittest
import mantid
import SANSUserFileParser as UserFileParser

class BackCommandParserTest(unittest.TestCase):
    def test_can_parse_correctly_initial_command(self):
        # Arrange
        correct1 = "TImE /sdlf/sdf" # Correct MAIN
        correct2 = "UAMp/sdlf/sdf"  # Correct HAB
        correct3 = "MON/RUN=1234/sdf/sdf" # Correct Mon/RUN=
        parser = UserFileParser.BackCommandParser()

        # Act and assert
        self.assertTrue(parser.can_attempt_to_parse(correct1))
        self.assertTrue(parser.can_attempt_to_parse(correct2))
        self.assertTrue(parser.can_attempt_to_parse(correct3))

    def test_cannot_parse_correctly_initial_command(self):
        # Arrange
        correct1 = "FRoNT=/sdlf/sdf" # Wrong specifier
        correct2 = "MON/sdf/sdf/sdf" # No run number
        correct3 = "Time/sdf" # Correct first but incorrect length

        parser = UserFileParser.BackCommandParser()

        # Act and assert
        self.assertFalse(parser.can_attempt_to_parse(correct1))
        self.assertFalse(parser.can_attempt_to_parse(correct2))
        self.assertFalse(parser.can_attempt_to_parse(correct3))

    def test_that_can_parse_TIME_MEAN_RUN(self):
        argument = "TIME/ mEAN/RuN=SANS2D1111111"
        uniform = True
        mean = True
        run_number ="SANS2D1111111"
        is_mon = False
        mon_number = 0
        self.do_test_can_parse_correctly(argument, uniform, mean, run_number, is_mon, mon_number)

    def test_that_can_parse_UAMP_TOF_RUN(self):
        argument = "Uamp/ToF /Run=2222"
        uniform = False
        mean = False
        run_number ="2222"
        is_mon = False
        mon_number = None
        self.do_test_can_parse_correctly(argument, uniform, mean, run_number, is_mon, mon_number)

    def test_that_can_parse_TIME_MEAN_RUN(self):
        argument = "TIME/tof/run=LOQ33333333"
        uniform = True
        mean = False
        run_number ="LOQ33333333"
        is_mon = False
        mon_number = None
        self.do_test_can_parse_correctly(argument, uniform, mean, run_number, is_mon, mon_number)

    def test_that_can_parse_UAMP_MEAN_RUN(self):
        argument = " UAMP/mean /RuN=444444444"
        uniform = False
        mean = True
        run_number ="444444444"
        is_mon = False
        mon_number = None
        self.do_test_can_parse_correctly(argument, uniform, mean, run_number, is_mon, mon_number)

    def test_that_can_parse_MON_RUN_TIME_MEAN(self):
        argument = "MON/RUN=123124/time/mean"
        uniform = True
        mean = True
        run_number ="123124"
        is_mon = True
        mon_number = None
        self.do_test_can_parse_correctly(argument, uniform, mean, run_number, is_mon, mon_number)
    
    def test_rejects_bad_first_value(self):
        argument = "GUN/RUN=123124/time/mean  "
        self.do_test_parsing_fails(argument)

    def test_rejects_bad_first_value(self):
        argument = "mean/UAMP//RuN=444444444"
        self.do_test_parsing_fails(argument)

    def test_rejects_bad_second_value(self):
        argument = "UAMP/meanTT/RuN=444444444"
        self.do_test_parsing_fails(argument)

    def test_rejects_bad_third_value(self):
        argument = "UAMP/mean/RuN 44444"
        self.do_test_parsing_fails(argument)
    
    def test_that_can_pars_M3_RUN_TIME_MEAN(self):
        argument = "M3/RUN=123124/time/mean"
        uniform = True
        mean = True
        run_number ="123124"
        is_mon = True
        mon_number = 3
        self.do_test_can_parse_correctly(argument, uniform, mean, run_number, is_mon, mon_number)

    def do_test_can_parse_correctly(self, arguments, expected_uniform,
                                    expected_mean, expected_run_number,
                                    is_mon, expected_mon_number):
        # Arrange
        parser = UserFileParser.BackCommandParser()
        # Act
        result = parser.parse_and_set(arguments)
        # Assert
        self.assertEqual(result.mean, expected_mean)
        self.assertEqual(result.time, expected_uniform)
        self.assertEqual(result.mon, is_mon)
        self.assertEqual(result.run_number, expected_run_number)
        self.assertEqual(result.mon_number, expected_mon_number)

    def do_test_parsing_fails(self, arguments):
        # Arrange
        parser = UserFileParser.BackCommandParser()
        # Act
        args = [arguments]
        self.assertRaises(RuntimeError, parser.parse_and_set,*args)


if __name__ == "__main__":
    unittest.main()
