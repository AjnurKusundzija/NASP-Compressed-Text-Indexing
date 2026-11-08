# Algorithms and measurements

## Problem

Given a byte string `T` and a pattern `P`, report every starting position where `P` occurs in
`T`, including overlapping occurrences. The benchmark compares methods on exactly this API; no
document parsing or UI behavior is mixed into the result.

## Implementations

### Naive scan

For every position `i`, compare `T[i..i+|P|)` with `P`. Counting costs
`O(|T||P|)` in the worst case and locating costs the same plus output. It is the correctness
reference and has `O(1)` index memory.

### Suffix array

The suffix array stores all suffix start positions in lexicographic order. The implementation
constructs it with prefix doubling and counting sort. Two binary searches find the contiguous
suffix range beginning with `P`; the range size is the count and its suffix positions are the
locations. Construction is `O(n log n)`, count is `O(|P| log n)`, and the array uses 32-bit
positions.

### FM-index

The FM-index stores:

1. the Burrows-Wheeler transform (BWT);
2. cumulative counts `C`;
3. one rank structure over the BWT;
4. sampled suffix-array values for locating matches.

Backward search processes `P` right-to-left. Each character narrows a suffix-array interval:

```text
l = C[c] + rank(c, l)
r = C[c] + rank(c, r)
```

The count is `r - l`, so counting does not need the original text. Locating walks BWT rows with
LF-mapping until a sampled suffix-array value is reached.

The checkpoint rank structure stores cumulative counts every `k` rows and scans at most `k - 1`
BWT symbols. The wavelet tree answers rank in `O(log sigma)` time using bit vectors, where
`sigma` is the number of distinct byte values.

## Benchmark design

The benchmark uses a deterministic generated log so results are repeatable and contain both
frequent structured patterns and missing patterns. It verifies all measured methods against the
naive implementation before timing them. Each latency row is the median of per-pattern medians
over repeated runs.

The reported experiments separate:

- **construction and size**: original text, suffix array, both FM-index variants;
- **count latency**: existing versus missing patterns by pattern length;
- **locate latency**: position recovery and average LF steps;
- **sampling tradeoffs**: Occ sampling affects rank memory/count time, while suffix-array sampling
  affects locate memory/time.

Do not treat one machine's absolute timings as universal. The useful conclusions are the relative
curves: scanning degrades with text size, indexed queries avoid a full scan, and less sampling
uses less memory at the cost of more work.
