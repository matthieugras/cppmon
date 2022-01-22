# Possible low-level optimizations

## Avoid making copies

- The subformulas of _OR_ should have the same layout  
  __Why:__ we have to permute one of the two rows otherwise
- The bound variable of _EXISTS_ should be at the last position  
  __Why:__ the operation is `O(cols)` otherwise, because the whole row must be copied/permuted
- Remove _EXISTS_ that don't bind any variables

## Avoid memory allocations

- Replace sequences of empty tables by:  
  `struct EmptyTable {size_t num_empty;}`  
  __Why:__ vector realloc if there are temporal operators in the formula

## Other optimizations

- Table representation which is optimized for typed columns  
  Why: each row uses twice the amount of memory → branch and cache misses
- Implement context-sensitive opimizations in the parser  
  __See:__ [Github issue of the parser library](http://www.google.de)
