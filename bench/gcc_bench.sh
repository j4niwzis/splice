#!/usr/bin/env bash
# onion (peel_to, with and without always_inline) against fold + assume, on
# gcc: compile time of the dispatch alone, the write-then-read pair's code
# (does the second dispatch reload?), and its run time.
set -u
CXX=${CXX:-g++-14}
$CXX --version | head -1
cd bench
for v in peel peel_inline fold_assume; do
  case $v in
    peel) src=twice_peel_to.cc; def=-DPEEL; disp=bench4.cc ;;
    peel_inline) src=twice_peel_to_always_inline.cc; def=-DPEEL; disp=bench4_ai.cc ;;
    fold_assume) src=twice_fold_assume.cc; def=-DFOLD; disp=bench4_ai.cc ;;
  esac
  for opt in -O0 -O3; do
    /usr/bin/time -f "$v $opt dispatch compile: %e s, %M KB" $CXX -std=c++23 $opt $def -ftemplate-depth=4096 -c -o /dev/null $disp 2>&1 | tail -1
  done
  # The write-then-read pair: its code, and how many loads of the value it does.
  $CXX -std=c++23 -O3 $def -ftemplate-depth=4096 -S -o $v.s $src
  awk '/^_Z3run.*:$/,/\.cfi_endproc/' $v.s > $v.run.s
  echo "$v -O3 run(): $(grep -cE '^\s+[a-z]' $v.run.s) instructions, $(grep -cE '^\s+j[a-z]+\s' $v.run.s) jumps, $(grep -cE 'jmp\s+\*' $v.run.s) indirect, $(grep -cE 'call\s' $v.run.s) calls, $(grep -cE 'mov[a-z]*\s+\(%r[a-z0-9]+\),\s*%r' $v.run.s) loads"
  cat $src ${src%.cc}_main.cc > /dev/null 2>&1
  for opt in "-O3" "-O3 -flto"; do
    $CXX -std=c++23 $opt $def -ftemplate-depth=4096 -o $v.exe ${src%.cc}_main.cc 2>&1 | tail -3
    times=$(for i in 1 2 3 4 5 6 7; do ./$v.exe | cut -d' ' -f1; done | sort -n | sed -n 4p)
    echo "$v $opt run time: median $times ms"
  done
done
