from numpy import random, uint64
from ast import literal_eval
from json import JSONDecodeError, dump, load
from math import log2


def MASK_CLEAR(buf, pos):
    buf &= ~pos
    return buf


def BIT_CHECK(buf, pos):
    return not (not (buf & (1 << pos)))


def BIT_SET(buf, pos):
    buf |= (1 << pos)
    return buf


def BIT_CLEAR(buf, pos):
    buf &= ~(1 << pos)
    return buf


class Cache:
    def __init__(self, line_sz, assoc, cache_sz, bnk_addr):
        self.line_sz: int = line_sz
        self.assoc: int = assoc
        self.cache_sz: int = cache_sz
        self.bnk_addr: list = bnk_addr


class Script:
    def __init__(self, cache, addr_ranges, set_rng, bnk_rng, lines_coeff):
        self.cache: Cache = cache
        self.addr_ranges: list = addr_ranges
        self.set_rng: list = set_rng
        self.bnk_rng: list = bnk_rng
        self.lines_coeff: float = lines_coeff


def script(filename):
    try:
        with open(filename) as file_object:
            data = load(file_object)
    except JSONDecodeError as e:
        print(e.msg)
    cache = Cache(data["cache_descr"]["line_size"],
                  data["cache_descr"]["associativity"],
                  data["cache_descr"]["cache_size"],
                  data["cache_descr"]["bnk_addr_bits"])
    addr_ranges = []
    for addr in data["addr_ranges"]:
        addr_ranges.append([int(literal_eval(addr['bgn'])), int(literal_eval(addr['end']))])
    set_range = [int(data['set_range']['min']), int(data['set_range']['max'])]
    bnk_range = [int(data['bnk_range']['min']), int(data['bnk_range']['max'])]
    return Script(cache, addr_ranges, set_range, bnk_range, data["lines_assoc_coeff"])


def mapgen(seed: int, task: Script):
    random.seed(seed)
    bnk_am = int(2 ** len(task.cache.bnk_addr))
    sets_am = int(task.cache.cache_sz / task.cache.line_sz / bnk_am / task.cache.assoc)
    if task.set_rng[0] != task.set_rng[1]:
        sets = random.randint(low=task.set_rng[0], high=task.set_rng[1] + 1)
    else:
        sets = task.set_rng[0]
    if task.bnk_rng[0] != task.bnk_rng[1]:
        bnks = random.randint(low=task.bnk_rng[0], high=task.bnk_rng[1] + 1)
    else:
        bnks = task.bnk_rng[0]
    lines_am = int(task.cache.assoc * task.lines_coeff)
    set_addr = []
    bnk_excl = []
    set_excl = []
    res = []
    i = int(log2(task.cache.line_sz))
    while len(set_addr) < int(log2(sets_am)):
        if i not in task.cache.bnk_addr:
            set_addr.append(i)
        i += 1
    for j in range(0, bnks):
        bnk = random.randint(low=0, high=bnk_am)
        while bnk in bnk_excl:
            bnk = (bnk + 1) % bnk_am
        bnk_excl.append(bnk)
        for k in range(0, sets):
            set = random.randint(low=0, high=sets_am)
            while set in set_excl:
                set = (set + 1) % sets_am
            set_excl.append(set)
            for m in range(0, lines_am):
                while 1:
                    addr = random.randint(low=0, high=len(task.addr_ranges))
                    buf = random.randint(2**64, dtype=uint64) % (task.addr_ranges[addr][1] - task.addr_ranges[addr][0] + 1) + task.addr_ranges[addr][0]
                    buf = MASK_CLEAR(int(buf), task.cache.line_sz - 1)
                    for l in range(0, len(task.cache.bnk_addr)):
                        if BIT_CHECK(bnk, l):
                            buf = BIT_SET(buf, task.cache.bnk_addr[l])
                        else:
                            buf = BIT_CLEAR(buf, task.cache.bnk_addr[l])
                    for n in range(0, len(set_addr)):
                        if BIT_CHECK(set, n):
                            buf = BIT_SET(buf, set_addr[n])
                        else:
                            buf = BIT_CLEAR(buf, set_addr[n])
                    if task.addr_ranges[addr][0] <= buf and buf + task.cache.line_sz - 1 <= task.addr_ranges[addr][1] and buf not in res:
                        break
                res.append(buf)
        set_excl.clear()
    return res


def to_json(file, buf, task):
    ranges = []
    for addr in buf:
        ranges.append({'bgn': hex(addr), 'end': hex(int(addr + task.cache.line_sz - 1))})
    dict = {'addr_ranges': ranges}
    with open(file, 'w') as outfile:
        dump(dict, outfile)
