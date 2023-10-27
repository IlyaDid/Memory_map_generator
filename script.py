import sys

from numpy import random, uint64
from ast import literal_eval
from json import JSONDecodeError, dump, load
from math import log2


def MASK_CLEAR(buf, pos):
    buf &= ~pos
    return buf


def BIT_CHECK(buf, pos):
    return bool(buf & (1 << pos))


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
        print(e.msg, sys.stderr)
        exit(1)
    if int(data['cache_descr']['line_size']) % 2 != 0 or int(data['cache_descr']['line_size']) <= 0:
        print('ERROR: Cache line size must be degree of 2 and larger than 1', sys.stderr)
        exit(2)
    if int(data['cache_descr']['associativity']) % 2 != 0 or int(data['cache_descr']['associativity']) <= 0:
        print('ERROR: Cache associativity must be degree of 2 and larger than 1', sys.stderr)
        exit(2)
    if int(data['cache_descr']['cache_size']) % 2 != 0 or int(data['cache_descr']['cache_size']) <= 0:
        print('ERROR: Cache size must be degree of 2 and larger than 1', sys.stderr)
        exit(2)
    for bit in data['cache_descr']['bnk_addr_bits']:
        if int(bit) < 0:
            print('ERROR: Bank address bit must be a non negative number', sys.stderr)
            exit(2)
        if int(bit) > 63:
            print('ERROR: Bank address bit out of address range', sys.stderr)
            exit(2)
        if int(bit) < log2(int(data['cache_descr']['cache_size'] / len(data['cache_descr']['bnk_addr_bits']) / data['cache_descr']['associativity'] / data['cache_descr']['line_size'])):
            print('ERROR: Bank bits overlap data bits', sys.stderr)
            exit(2)
    cache = Cache(data['cache_descr']['line_size'],
                  data['cache_descr']['associativity'],
                  data['cache_descr']['cache_size'],
                  data['cache_descr']['bnk_addr_bits'])
    addr_ranges = []
    for addr in data['addr_ranges']:
        try:
            addr_ranges.append([int(literal_eval(addr['bgn'])), int(literal_eval(addr['end']))])
        except Exception:
            print('ERROR: Wrong addr_ranges format', sys.stderr)
            exit(2)
        if addr_ranges[len(addr_ranges) - 1][0] < 0 or addr_ranges[len(addr_ranges) - 1][1] < 0:
            print('ERROR: Addr_ranges must be a non negative number')
            exit(2)
        if addr_ranges[len(addr_ranges) - 1][1] - addr_ranges[len(addr_ranges) - 1][0] <= 0:
            print('ERROR: Addr_range must have a positive length', sys.stderr)
            exit(2)
        if int(literal_eval(addr['end']) - int(literal_eval(addr['bgn']))) < data['lines_assoc_coeff'] * data['set_range']['max'] * data['bnk_range']['max'] * data['cache_descr']['line_size']:
            print('WARNING: Addr range ' + str(len(addr_ranges) - 1) + ' may not be able to contain all generated addresses on some seeds. Decrease maximum bank/set amounts or increase length of this address range', sys.stderr)
    set_range = [int(data['set_range']['min']), int(data['set_range']['max'])]
    if set_range[0] < 0 or set_range[1] < 0:
        print('ERROR: Set_range borders must be positive numbers', sys.stderr)
        exit(2)
    if set_range[1] - set_range[0] < 0:
        print('ERROR: Set_range must be non negative', sys.stderr)
        exit(2)
    bnk_range = [int(data['bnk_range']['min']), int(data['bnk_range']['max'])]
    if bnk_range[0] < 0 or bnk_range[1] < 0:
        print('ERROR: Bnk_range borders must be positive numbers', sys.stderr)
        exit(2)
    if bnk_range[1] - bnk_range[0] < 0:
        print('ERROR: Bnk_range must be non negative', sys.stderr)
        exit(2)
    if data['lines_assoc_coeff'] <= 0:
        print('ERROR: Lines_assoc_coeff must be a positive number')
        exit(2)
    return Script(cache, addr_ranges, set_range, bnk_range, data['lines_assoc_coeff'])


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
                    if task.addr_ranges[addr][1] - task.addr_ranges[addr][0] < task.lines_coeff * sets_am * bnk_am * task.cache.line_sz:
                        print('ERROR: Can not fit addresses in address range ' + str(addr), sys.stderr)
                        exit(3)
                    if int((task.addr_ranges[addr][1] - task.addr_ranges[addr][0]) / task.cache.line_sz / bnk_am / sets_am) < lines_am:
                        print('ERROR: Addr range ' + str(addr) + ' can not fit needed amount of addresses')
                        exit(3)
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
