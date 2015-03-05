import pyethash.core

def test_get_params():
    block_num = 123456
    out = pyethash.core.get_params(block_num)
    assert out != None

if __name__ == '__main__':
    test_me()
