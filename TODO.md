yes bhaskar i have generated the files with ai to fill them in is our work

# M0 — scaffolding & core plumbing

* [ ] **Repo layout**: `include/`, `src/`, `asm/`, `tests/`, `tools/`, `bench/`
* [ ] **Build system**: CMake or Meson with `-O3 -Wall -Wextra -Wconversion -fPIC`
* [ ] **Common API**

  * [ ] `include/codectk.h`: `codectk_codec`, error codes, registry hooks
  * [ ] `src/registry.c`: `codectk_get("hamming"|"bch"|"goppa"|"huffman")`
* [ ] **Bit I/O**

  * [ ] `include/bitio.h` & `src/bitio.c`: writer/reader + flush/EOF handling
  * [ ] Unit tests: partial bytes, exact bit counts, truncation

# M1 — math & primitives

* [ ] **GF(2) matrix & bit ops**

  * [ ] `include/gf2.h` / `src/gf2.c`: bitset XOR, row-reduction, parity
* [ ] **GF(2^m) context**

  * [ ] `include/gf2m.h`: ctx {m, prim, log/alog}
  * [ ] `src/gf2m.c` (C fallback): `mul`, `inv`, `sqr`, field init from primitive poly
  * [ ] Table gen: log/alog build, guard zero, modulus `(2^m−1)`
  * [ ] Tests: field axioms, inverses, random vectors
* [ ] **Polynomials over GF(2) and GF(2^m)**

  * [ ] `include/poly.h` / `src/poly.c`: add, mul, mod, div, gcd, eval, formal derivative
  * [ ] Bench tiny sizes (m≤12) to catch perf cliffs

# M2 — algorithms: source coding (top) & simple channel code (bottom)

* [ ] **Huffman (dynamic)**

  * [ ] `include/huffman.h`, `src/huffman.c`
  * [ ] Frequency table (257 symbols incl. EOF), header format `HUF1`
  * [ ] Min-heap, tree build, canonicalize (optional), code emit
  * [ ] Decoder: rebuild from header; streaming bit decode
  * [ ] Tests: empty, 1-symbol file, uniform/random, golden round-trip
  * [ ] API adapter to `codectk_codec` (encode/decode signatures)
* [ ] **Hamming (dynamic m)**

  * [ ] `include/hamming.h`, `src/hamming.c`
  * [ ] Runtime `m` → `(n,k)=(2^m−1, 2^m−1−m)`, auto parity positions
  * [ ] Encode: place data, compute parity masks; Decode: syndrome + single-bit fix
  * [ ] Streaming blocks via `bitio`
  * [ ] Tests: flip 0..2 bits, boundary packing, random vectors
  * [ ] Optional: SECDED (overall parity) toggle

# M3 — heavier channel codes

* [ ] **BCH (binary, length n=2^m−1)**

  * [ ] `include/bch.h`, `src/bch.c`
  * [ ] Param & precompute

    * [ ] choose `m` (≤12 to start), `t`
    * [ ] minimal polys for α^{1..2t} (odd exponents if primitive narrow-sense)
    * [ ] generator g(x)=lcm of minimal polys
  * [ ] Encode (systematic): compute remainder of `x^{deg g} M(x) mod g(x)`
  * [ ] Decode:

    * [ ] Syndromes S₁..S_{2t}
    * [ ] Berlekamp–Massey → Λ(x), Ω(x)
    * [ ] Chien search roots of Λ(α^{-i})
    * [ ] Forney magnitudes (binary BCH ⇒ 1)
    * [ ] Flip bits; return correction count
  * [ ] Tests:

    * [ ] Known small params (m=4,t=1; m=5,t=2; m=7,t=3)
    * [ ] Random messages with ≤t errors → success, >t → EDECODE
    * [ ] Edge: all-zero/all-one codewords, burst patterns
  * [ ] Bench: encode/decoded Mbps vs Hamming

* [ ] **Binary Goppa (Γ(L,g)) — Patterson path**

  * [ ] `include/goppa.h`, `src/goppa.c`
  * [ ] Parameters

    * [ ] Field ctx, degree `t`, support set `L` (distinct; non-roots of g)
    * [ ] Irreducible `g(x)` over GF(2^m), degree t
  * [ ] Encode (systematic) via parity-check H from (L, g)
  * [ ] Decode (Patterson)

    * [ ] Build S(x) = Σ r_i/(x − L_i) mod g(x)
    * [ ] Compute T(x) = S(x)^{-1} mod g(x)
    * [ ] Split T into squares: find a(x), b(x) s.t. a^2 + x b^2 ≡ T (mod g)
    * [ ] Recover error locator; evaluate at L to locate error positions
    * [ ] Flip bits; report corrections
  * [ ] Tools:

    * [ ] `tools/gen_goppa_poly.py`: generate irreducible g(x), random L
  * [ ] Tests: small (m=8,t=2..4), random error patterns; correctness vs reference (python)

# M4 — plumbing, UX, and performance polish

* [ ] **Codec registry & CLI**

  * [ ] `codectk_get()` returns vtable for "huffman"/"hamming"/"bch"/"goppa"
  * [ ] Example `tools/pipe.c`: `huffman -> hamming|bch|goppa` encode pipeline, and reverse
  * [ ] File/stream options, bit-padding notes, headers/params TLV
* [ ] **ASM backends (optional)**

  * [ ] x86-64:

    * [ ] PCLMULQDQ polynomial multiply (if going polynomial basis)
    * [ ] or fast log/alog table inner loops with AVX2 prefetch
  * [ ] ARM64:

    * [ ] PMULL path
  * [ ] Backend switch in `gf2m_backend` vtable
  * [ ] Micro-benchmarks: cycles/op for mul/inv/sqr
* [ ] **Security & constant-time (where sensible)**

  * [ ] Remove secret-dependent branches in decoding hotpaths
  * [ ] Masked table reads for `log/alog` (optional)
  * [ ] Audit memory wipes for temporary secrets (if used in crypto contexts)
* [ ] **Robustness**

  * [ ] Parameter validation: m bounds, t bounds, buffer sizing, bitlength alignment
  * [ ] Clear error returns: `CODECTK_EINVAL/ENOMEM/EDECODE/ENOTSUP`
  * [ ] Fuzzers (`tests/fuzz_*.c` or libFuzzer): random bits, truncated streams
  * [ ] Property tests: encode→channel noise→decode round-trip invariants
* [ ] **Docs**

  * [ ] `README.md`: matrix of supported codes & complexities
  * [ ] `docs/FORMAT.md`: on-wire formats, headers, padding
  * [ ] `docs/PARAMS.md`: picking `m,t`, rate vs correction charts
* [ ] **CI**

  * [ ] GitHub Actions: build (Linux/macOS/Windows), run tests, ASan/UBSan/MSan
  * [ ] Code coverage badge; style/lint job
* [ ] **Packaging**

  * [ ] `pkg-config` file, version header
  * [ ] install targets: headers + static/shared libs

# Nice-to-have extras

* [ ] **Canonical Huffman** for faster table-based decoding
* [ ] **Streaming adapters**: file descriptors, mmap, zero-copy blocks
* [ ] **Examples**: encode text with `huffman+bch`, flip bits, decode back
* [ ] **Bindings**: minimal C++/Rust header or FFI (extern "C")
* [ ] **Visualization**: dot export for Huffman trees (debug)
* [ ] **Profiling**: perf flamegraphs, annotate hotspots

# Concrete “fill the TODOs” list by file

* [ ] `include/codectk.h`: finalize vtable, doc comments
* [ ] `src/registry.c`: map names → codecs
* [ ] `include/bitio.h` / `src/bitio.c`: finalize flush/EOF semantics + tests
* [ ] `include/gf2m.h` / `src/gf2m.c`: table generation; C backend; hooks for ASM
* [ ] `include/poly.h` / `src/poly.c`: gcd, mod-exp, inv mod poly, eval, square
* [ ] `include/huffman.h` / `src/huffman.c`: wrap to `codectk_codec`
* [ ] `include/hamming.h` / `src/hamming.c`: done logic + SECDED toggle
* [ ] `include/bch.h` / `src/bch.c`:

  * [ ] minimal poly builder
  * [ ] generator g(x)
  * [ ] BM, Chien, Forney
* [ ] `include/goppa.h` / `src/goppa.c`:

  * [ ] parity-check build
  * [ ] Patterson steps (S, T, split, locator, evaluation)
* [ ] `tools/gen_goppa_poly.py`: irreducible test, random support
* [ ] `tests/`: unit + property + fuzz across all codecs
* [ ] `bench/`: encode/decode throughput; gf2m micro-ops
* [ ] `asm/`: stubs + wiring + feature detection


