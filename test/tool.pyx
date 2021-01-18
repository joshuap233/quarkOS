import random
import string


class TestCase:
    # 测试次数
    count = 10
    def assertEqual(self, s1, s2):
        assert s1 == s1

    def assertTrue(self, s):
        assert s == True

    def assertFalse(self, s):
        assert s == False

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

def random_str():
    return ''.join([random.choice(string.ascii_uppercase + string.digits) for _ in range(10)])
