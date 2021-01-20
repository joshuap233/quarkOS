import random
import string
from contextlib import contextmanager
import sys


class TestCase:
    # 测试次数
    count = 10
    def assertEqual(self, s1, s2):
        with self.catchAssertionError(s1, s2):
            assert s1 == s2

    def assertTrue(self, s):
        with self.catchAssertionError(s):
            assert s == True

    def assertFalse(self, s):
        with self.catchAssertionError():
            assert s == False

    @contextmanager
    def catchAssertionError(self, *args):
        try:
            yield
        except AssertionError as e:
            print(f"\nassert arg: {args}")
            raise e

    def run(self):
        print(f'start test ...')
        print("----------------------------------------------------------------------")
        for attr in dir(self):
            if attr.startswith('test_'):
                for i in range(self.count):
                    print(f'{attr}:{i}  ', end="")
                    self.__getattribute__(attr)()
                    print('pass')
        print("----------------------------------------------------------------------")
        print(f"test {self.count} OK")
        print()


def random_str(n=10):
    return ''.join([random.choice(string.ascii_uppercase + string.digits) for _ in range(n)])
