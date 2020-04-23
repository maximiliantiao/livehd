#!/bin/bash
rm -f *.cfg

function generate () {
  ./prp $1.prp | sort -n > y && column -t y > x && rm -f y && mv x $1.cfg
}

# wait for no bug cfg = ="
# generate function_call
# generate nested_if
# generate tuple
# generate ssa_if
# generate ssa_nested_if
# generate ssa_no_else_if
./prp simple_tuple.prp  | sort -n  > y && column -t y > x && rm -f y && mv x simple_tuple.cfg
./prp function_call.prp | sort -n  > y && column -t y > x && rm -f y && mv x function_call.cfg
./prp nested_if.prp     | sort -n  > y && column -t y > x && rm -f y && mv x nested_if.cfg
./prp tuple.prp         | sort -n  > y && column -t y > x && rm -f y && mv x tuple.cfg
./prp tuple_if.prp      | sort -n  > y && column -t y > x && rm -f y && mv x tuple_if.cfg
./prp tuple_if2.prp     | sort -n  > y && column -t y > x && rm -f y && mv x tuple_if2.cfg
./prp if.prp            | sort -n  > y && column -t y > x && rm -f y && mv x if.cfg
./prp if2.prp           | sort -n  > y && column -t y > x && rm -f y && mv x if2.cfg
./prp nested_if2.prp    | sort -n  > y && column -t y > x && rm -f y && mv x nested_if.cfg


