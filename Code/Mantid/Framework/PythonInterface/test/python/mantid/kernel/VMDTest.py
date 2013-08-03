import unittest
from mantid.kernel import VMD

class VMDTest(unittest.TestCase):

    def test_default_construction_gives_object_with_single_dimension(self):
        one = VMD()
        self.assertEquals(1, one.getNumDims())

    def test_constructors_with_dimension_pts(self):
        pts = [1]
        for i in range(2,7):
            pts.append(i)
            vector = VMD(*pts) #unpack list
            self.assertEquals(i, vector.getNumDims())

    def test_value_access_is_read_only(self):
        vector = VMD(1.0,2.0)
        self.assertAlmostEqual(1.0, vector[0])
        self.assertAlmostEqual(2.0, vector[1])

        try:
            vector[1] = 5.0
        except TypeError:
            pass
        except:
            self.fail("Operator setters have thrown an unexpected exception type. Expected TypeError.")
        else:
            self.fail("Operator setters have not thrown an exception. Expected to be read only.")


if __name__ == '__main__':
    unittest.main()
