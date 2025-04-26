import hashlib
from subprocess import Popen, PIPE
import os
from pathlib import Path
from argparse import ArgumentParser
import glob
import json
import shlex

script_dir_path = Path(os.path.dirname(os.path.realpath(__file__)))

windows_sdk_root = os.environ.get("WINDOWS_SDK_ROOT")

parser = ArgumentParser(prog='shader_compile')
parser.add_argument('-i', '--input', dest='input_path', required=True)
parser.add_argument('-o', '--output', dest='output_path', required=True)
parser.add_argument('--sdk', dest='sdk_root', required=False)
parser.add_argument('--cachePath', dest='cache_path', required=False)

args = parser.parse_args()

if not os.path.isdir(args.input_path):
    print(f"input path \"{args.input_path}\" is not a valid directory!")
    exit()

input_path = Path(args.input_path)

if os.path.isfile(args.output_path):
    print(f"output path \"{args.output_path}\" already exists as a file!")
    exit()

cache_path = None
in_cache = {}
if args.cache_path is not None:
    cache_path = Path(args.cache_path)
    os.makedirs(cache_path.parent, exist_ok=True)
    if cache_path.exists():
        with cache_path.open() as cache_file:
            in_cache = json.load(cache_file)

os.makedirs(args.output_path, exist_ok=True)
output_path = Path(args.output_path)

if args.sdk_root is not None:
    windows_sdk_root = args.sdk_root
    
if not os.path.isdir(windows_sdk_root):
    print(f"sdk path \"{windows_sdk_root}\" is not a valid directory!")
    exit()

fxc_path = Path(windows_sdk_root).joinpath("fxc.exe")

if not fxc_path.exists:
    print(f"given sdk path \"{windows_sdk_root}\" does not contain the effect-compiler tool (fxc.exe)")
    exit()

with script_dir_path.joinpath("shader_profiles.json").open() as shader_profiles_file:
    shader_profiles = json.load(shader_profiles_file)

out_cache = {}

for profile in shader_profiles:
    print(profile["profile_name"])
    source_files = glob.glob(f'**/{profile["source_pattern"]}', root_dir=input_path, recursive=True)
    for file in source_files:
        input_file = input_path.joinpath(file)
        input_file_path = str(input_file)
        input_file_path_absolute = str(input_file.absolute())
        output_file = output_path.joinpath(file).with_suffix(".cso")
        
        with input_file.open() as input_contents:
            input_hash = hashlib.md5(input_contents.read().encode()).hexdigest()

        cache_hash = in_cache.pop(input_file_path_absolute, None)
        if cache_hash is not None:
            if input_hash == cache_hash and output_file.exists():
                out_cache[input_file_path_absolute] = input_hash
                continue

        os.makedirs(output_file.parent, exist_ok=True)
        print(f"- {input_file} -> {output_file}")
        cmd = f'"{fxc_path}" /T {profile["profile_name"]} /Fo "{output_file}" "{input_file}"'
        process = Popen(shlex.split(cmd), stdout=PIPE, stderr=PIPE,)
        output, err = process.communicate()
        if len(err):
            print(" ".join(cmd))
            print(output.decode("utf-8"))
            print(err.decode("utf-8"))
        else:
            out_cache[input_file_path_absolute] = input_hash

for path_str in in_cache:
    file_path = Path(path_str)
    if not file_path.exists():
        cache_file_path = output_path.joinpath(file_path.relative_to(input_path)).with_suffix(".cso")
        cache_file_path.unlink(missing_ok=True)

if cache_path is not None:
    with cache_path.open('w') as cache_file:
        json.dump(out_cache, cache_file)