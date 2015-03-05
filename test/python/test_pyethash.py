import pyethash

def test_get_params_not_null():
    block_num = 123456
    out = pyethash.core.get_params(block_num)
    assert out != None

def test_get_params_based_on_EPOCH():
    block_num = 123456
    out1 = pyethash.core.get_params(block_num)
    out2 = pyethash.core.get_params((block_num // pyethash.EPOCH_LENGTH) * pyethash.EPOCH_LENGTH)
    assert out1["DAG Size"] == out2["DAG Size"]
    assert out1["Cache Size"] == out2["Cache Size"]

def test_get_params_returns_different_values_based_on_different_block_input():
    out1 = pyethash.core.get_params(123456)
    out2 = pyethash.core.get_params(12345)
    assert out1["DAG Size"] != out2["DAG Size"]
    assert out1["Cache Size"] != out2["Cache Size"]

def test_get_cache_smoke_test():
    params = pyethash.core.get_params(123456, bytes("~~~~~"))
    assert pyethash.core.get_cache(params) != None
