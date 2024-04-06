#!/bin/bash
# Usage: ./spec_build -m [BITS] -n [NAME] -s [SIZE] -b [benchmark]
# Examples
# 1) Run 'perlbench' with 'test' workload: ./spec_build -m 64 -n riscv-jh7100 -s test -b perlbench_r
# 2) Run 'integer' with 'ref' workload: ./spec_build -m 64 -n riscv-jh7100 -s ref -b intrate

# BITS: [32|64]
# BENCH: [fprate|fpspeed|intrate|intspeed|all]
# SIZE: [ref|train|test]

set -e

CPU2017="$HOME/cpu2017" #EDIT
NAME="riscv64-jh7100" #EDIT
BITS=64 #EDIT
SIZE="test" #EDIT
CONFIG=riscv64
WS="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd -P)"

# SOURCE cpu2017
cd "$CPU2017"
source shrc
cd "$WS"

while getopts ":m:n:s:b:" opt; do
  case $opt in
    m)
      if ! [[ "$OPTARG" =~ ^[0-9]+$ ]]; then
        echo "Integer only"
        exit 1
      fi
      if [ $OPTARG -ne 32 ] && [ $OPTARG -ne 64 ]; then
        echo "Invalid argument: $OPTARG" >&2
        exit 1
      fi

      BITS=$OPTARG
      ;;
    n)
      NAME=$OPTARG
      ;;
    b)
      BENCH=$OPTARG
      ;;
    s)
      SIZE=$OPTARG
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      exit 1
      ;;
    :)
      echo "Option -$OPTARG requires an argument." >&2
      exit 1
      ;;
  esac
done


if [ -z "$BENCH" ]; then
  echo 'Missing -b' >&2
  echo 'USAGE: ./spec_bench -m [32|64] -n [NAME] -b [BENCH]'
  exit 1
fi



# build
runcpu --define bits=${BITS} --config=$CONFIG --action=setup --size=${SIZE} $BENCH runcpu --define build_ncpus=$(nproc)

echo -e "\nyou can find output files in \"$CPU2017/benchspec/CPU/<BENCH>/run_base_<SIZE>_<NAME>\""
