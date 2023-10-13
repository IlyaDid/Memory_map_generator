#!/usr/bin/env python3

from getopt import getopt, error
from sys import argv
from script import *

options = 'hs:f:r:'
long_options = ['help', 'seed', 'file', 'result']
seed = ''
file = ''
result = ''
try:
    arguments, values = getopt(argv[1:], options, long_options)
    for currentArgument, currentValue in arguments:
        if currentArgument in ('-h', '--help'):
            print('Options:\n\t'
                  '-h [ --help ]\t\tShow help\n\t'
                  '-s [ --seed ] arg\tSeed for pseudorandom generation\n\t'
                  '-f [ --file ] arg\tJSON file with description of cache and memory parameters\n\t'
                  '-r [ --result ] arg\tJSON file where result will be saved\n')
            exit()
        elif currentArgument in ('-s', '--seed'):
            seed = currentValue
        elif currentArgument in ('-f', '--file'):
            file = currentValue
        elif currentArgument in ('-r', '--result'):
            result = currentValue
except error as err:
    print(str(err))

s = script(file)
to_json(result, mapgen(int(seed), s), s)
