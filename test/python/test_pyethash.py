import pyethash
from random import randint

def test_get_cache_size_not_None():
    for _ in range(100):
        block_num = randint(0,12456789)
        out = pyethash.get_cache_size(block_num)
        assert out != None

def test_get_full_size_not_None():
    for _ in range(100):
        block_num = randint(0,12456789)
        out = pyethash.get_full_size(block_num)
        assert out != None

def test_get_cache_size_based_on_EPOCH():
    for _ in range(100):
        block_num = randint(0,12456789)
        out1 = pyethash.get_cache_size(block_num)
        out2 = pyethash.get_cache_size((block_num // pyethash.EPOCH_LENGTH) * pyethash.EPOCH_LENGTH)
        assert out1 == out2

def test_get_full_size_based_on_EPOCH():
    for _ in range(100):
        block_num = randint(0,12456789)
        out1 = pyethash.get_full_size(block_num)
        out2 = pyethash.get_full_size((block_num // pyethash.EPOCH_LENGTH) * pyethash.EPOCH_LENGTH)
        assert out1 == out2

def test_get_cache_smoke_test():
    assert pyethash.mkcache_bytes(
            pyethash.get_cache_size(0), 
            "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~") != None
