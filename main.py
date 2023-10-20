#!/usr/bin/env python3

import argparse


from script import *

parser = argparse.ArgumentParser(description='Memory map generator.')
parser.add_argument('-s', '--seed', type=int, dest='seed', help='Seed for pseudorandom generation', required=True)
parser.add_argument('-f', '--file', type=str, dest='file', help='JSON file with description of cache and memory parameters', required=True)
parser.add_argument('-r', '--result', type=str, dest='result', help='JSON file where result will be saved', required=True)
args = parser.parse_args()

s = script(args.file)
to_json(args.result, mapgen(args.seed, s), s)
