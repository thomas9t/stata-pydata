import unittest, os, sys
import pandas as pd
import numpy  as np
sys.path.append('../src')
import shm

np.random.seed = 11122015

class test(unittest.TestCase):

    def setUp(self):
        self.float_variable = [3.14159, 2.87209, 234.7829]
        self.int_variable   = [1, 2, 3]
        self.long_variable  = [1L, 2L, 3L]
        self.string_variable = ['foo','bar','baz']
        self.mixed_variable  = [1, 2, 'foo']

        self.data = pd.DataFrame({
                        'float_var' : pd.Series(np.random.rand(1000000)),
                        'int_var'   : pd.Series(np.random.randint(1, high=1000, size=1000000))
                    })

    def tearDown(self):
        if os.path.exists('segment_info.txt'):
            os.unlink('segment_info.txt')
        if os.path.exists('../temp/test_segment_info.txt'):
            os.unlink('../temp/test_segment_info.txt')

    def test_basic(self):

        # Test essential basic functionality - these calls should all succeed
        float_segment = shm.write_list(self.float_variable, 'float', 'floats', 1)
        int_segment   = shm.write_list(self.int_variable, 'int', 'ints', 2)
        long_segment  = shm.write_list(self.long_variable, 'long', 'longs', 3)

        shm.deallocate(float_segment[1])
        shm.deallocate(int_segment[1])
        shm.deallocate(long_segment[1])

    def test_errors(self):

        # Test passing an unsupported data type (should raise a Type Error)
        with self.assertRaises(TypeError):
            shm.write_list(self.string_variable, 'string', 'strings', 1)

        # Test passing an unsupported data type as a valid one (should raise a Type Error from C)
        # Note also this test also confirms that segments are properly cleaned up after an 
        # exception
        with self.assertRaises(TypeError):
            shm.write_list(self.string_variable, 'int', 'string', 1)

        # Test passing a valid data type with an invalid element (should raise a Type Error from C)
        with self.assertRaises(TypeError):
            shm.write_list(self.mixed_variable, 'int', 'mixed', 1)

        # Test passing a seed used by an existing segment (should raise an OS error from C)
        int_segment = shm.write_list(self.int_variable, 'int', 'ints', 1)
        with self.assertRaises(OSError):
            shm.write_list(self.int_variable, 'int', 'ints', 1)
        shm.deallocate(int_segment[1])

        # Test passing a data frame with unsupported columns (should raise a Type Error)
        bad_data = self.data.copy()
        bad_data.loc[:,'string_var'] = 'Hi'
        with self.assertRaises(TypeError):
            shm.write_frame(bad_data)

    def test_stata(self):

        # Test writing to Stata
        stata_segment = shm.write_frame(self.data, info_file = '../temp/test_segment_info.txt')
        rc = os.system('stata-mp test_shm.do')
        self.assertTrue(rc == 0)

        stata_results = pd.read_csv('../temp/results_from_stata.csv')

        # data returned from Stata should be identical to minor precision
        # related issues. Note machine epsilon is about 2.2e-16.

        float_diff = (stata_results['float_var'] - self.data['float_var']).abs()
        self.assertTrue(float_diff.max() < 2.25e-16)

        # there should be no difference in integer variables
        int_diff = (stata_results['int_var'] - self.data['int_var']).abs()
        self.assertTrue(int_diff.max() == 0)

if __name__=='__main__':
    unittest.main()
