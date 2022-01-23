# Possible low-level optimizations

## Avoid making copies

- The subformulas of _OR_ should have the same layout  
  __Why:__ we have to permute one of the two rows otherwise
- The bound variable of _EXISTS_ should be at the last position  
  __Why:__ the operation is `O(cols)` otherwise, because the whole row must be copied/permuted

Are non-trivial problems, but working on this could improve performance a lot for easy formulas.
- Remove _EXISTS_ that don't bind any variables

## Avoid memory allocations

- Replace sequences of empty tables by:  
  `struct EmptyTable {size_t num_empty;}`  
  __Why:__ vector realloc if there are temporal operators in the formula, shouldn't improve performance too much
- Encode predicate names as integers  
  __Why:__ can be hashed much faster


## Table representation

- Table representation which is optimized for typed columns  
  __Why__: each row uses twice the amount of memory → cache misses and memory usage
- Table with hash precomputation on insert/emplace  
  __Why__: We often move rows from one table to another. For the memory this only requires a pointer swap, but the row is reshashed each time we move it


## Other optimizations

- Implement context-sensitive opimizations in the parser  
  See: [Github issue of the parser library](http://www.google.de)  
  __Done:__ 10% parser improvement
