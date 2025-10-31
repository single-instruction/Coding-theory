# Claude.md - Coding Theory Library Implementation Plan

## Project Overview
This is a comprehensive **coding theory library** implementing various error-correcting codes and source coding algorithms in C. The project aims to provide production-quality implementations of:
- **Channel codes**: Hamming, BCH, Binary Goppa (with Patterson decoder)
- **Source coding**: Huffman
- **Mathematical primitives**: GF(2), GF(2^m) arithmetic, polynomial operations
- **Infrastructure**: Bit-level I/O, codec registry, streaming API

## Current Implementation State

### ✅ COMPLETE Components

1. **Core API (`backend/codectk.h`, `src/registry.c`)**
   - Error codes: `CODECTK_OK`, `EINVAL`, `ENOMEM`, `EDECODE`, `ENOTSUP`
   - Codec vtable: `codectk_codec` with encode/decode function pointers
   - Registry: `codectk_get()` factory function for codec lookup
   - Status: **FULLY IMPLEMENTED** ✓

2. **Bit I/O (`backend/bitio.h`)**
   - `bitw_t` writer: `bitw_init()`, `bitw_put()`, `bitw_flush()`
   - `bitr_t` reader: `bitr_init()`, `bitr_get()`
   - Handles partial bytes, bit-level streaming
   - Status: **HEADER-ONLY IMPLEMENTATION COMPLETE** ✓

3. **GF(2^m) Arithmetic (`backend/gf2m.h`, `src/gf2m.c`)**
   - Context structure with log/alog tables
   - C fallback: `gf2m_mul_c()`, `gf2m_inv_c()`, `gf2m_sqr_c()`
   - Backend vtable for ASM swapping
   - Status: **CORE OPERATIONS IMPLEMENTED** ✓
   - **MISSING**: Field initialization, table generation functions

4. **Hamming Codes (`backend/hamming.h`, `src/hamming.c`)**
   - Runtime parameterization: `m → (n,k) = (2^m-1, 2^m-1-m)`
   - Encode: data placement, parity computation
   - Decode: syndrome calculation, single-bit correction
   - Streaming via bitio
   - Status: **FULLY IMPLEMENTED** ✓✓✓

### ⚠️ PARTIAL / STUB Components

5. **BCH Codes (`backend/bch.h`, `src/bch.c`)**
   - Header: parameter structure defined ✓
   - Source: skeleton with TODO comments
   - Missing:
     - Minimal polynomial computation
     - Generator polynomial g(x) construction
     - Berlekamp-Massey algorithm
     - Chien search
     - Forney algorithm
   - Status: **STRUCTURE ONLY - NEEDS IMPLEMENTATION** 🔴

6. **Goppa Codes (`backend/goppa.h`, `src/goppa.c`)**
   - Header: parameter structure (L, g, field tables) ✓
   - Source: skeleton with Patterson decoder notes
   - Missing:
     - Parity-check matrix H construction
     - Patterson decoder (S(x), T(x), quadratic split, locator)
   - Status: **STRUCTURE ONLY - NEEDS IMPLEMENTATION** 🔴

7. **Huffman Coding (`backend/huffman.h`, `src/huffman.c`)**
   - Header: minimal structure
   - Source: **EMPTY FILE**
   - Missing: frequency table, heap, tree build, encode/decode
   - Status: **NOT STARTED** 🔴

### 🚫 MISSING Components

8. **GF(2) Operations (`backend/gf2.h`, `src/gf2.c`)**
   - Files exist but are **EMPTY** (1 line only)
   - Needed for: bitset XOR, matrix row reduction, parity checks
   - Status: **NOT STARTED** 🔴

9. **Polynomials (`backend/poly.h`, `src/poly.c`)**
   - Files exist but are **EMPTY** (1 line only)
   - Needed for: add, mul, mod, div, gcd, eval, formal derivative
   - Critical dependency for BCH and Goppa
   - Status: **NOT STARTED** 🔴

10. **Bit I/O Tests (`src/bitio.c`)**
    - File is **EMPTY**
    - Header has inline implementations, but no unit tests
    - Status: **TESTS MISSING** 🔴

11. **Build System (`CMakeLists.txt`)**
    - File is **EMPTY** (1 line only)
    - Status: **NOT STARTED** 🔴

---

## Critical Dependencies Graph

```
Level 0 (No deps):
  - CMakeLists.txt [build system]
  - bitio.h/c [bit I/O]
  - gf2.h/c [GF(2) ops]

Level 1 (Depends on L0):
  - gf2m.h/c [needs: nothing, but should have init functions]
  - poly.h/c [needs: gf2.h for GF(2) polynomials]

Level 2 (Depends on L1):
  - huffman.h/c [needs: bitio]
  - hamming.h/c [needs: bitio] ✓ DONE
  - bch.h/c [needs: gf2m, poly]
  - goppa.h/c [needs: gf2m, poly]

Level 3 (Integration):
  - tools/pipe.c [needs: all codecs]
  - tests/ [needs: all components]
  - bench/ [needs: all components]
```

---

## Milestone-Based Implementation Plan

### 🔥 PHASE 0: Build Foundation (M0)
**Goal**: Get project building and testable

**Tasks:**
1. ✅ Review existing structure (DONE - this analysis)
2. ⚠️ Create `CMakeLists.txt` with:
   - Source/header organization (`include/` symlinks to `backend/`)
   - Compiler flags: `-O3 -Wall -Wextra -Wconversion -fPIC`
   - Test target with basic framework
   - Library targets (static/shared)
3. ⚠️ Add minimal `bitio.c` with unit tests:
   - Test partial bytes
   - Test exact bit counts
   - Test truncation/EOF handling
4. ⚠️ Verify Hamming codec builds and runs
5. ⚠️ Create `tests/test_hamming.c` to validate existing implementation

**Deliverables**: Buildable project, passing Hamming tests

---

### PHASE 1: Mathematical Primitives (M1)
**Goal**: Implement foundational math operations

**1.1 GF(2) Binary Operations**
- `backend/gf2.h`:
  - Bitset structure (uint64_t arrays or byte arrays)
  - `gf2_xor()`, `gf2_and()`, `gf2_popcount()`
  - `gf2_row_reduce()` (Gaussian elimination)
  - `gf2_dot()` (dot product for parity)
- `src/gf2.c`: Implement above
- `tests/test_gf2.c`: Unit tests for all operations

**1.2 GF(2^m) Field Initialization**
- Extend `gf2m.h` with:
  - `gf2m_ctx_init(ctx, m, prim_poly)`
  - `gf2m_ctx_free(ctx)`
  - Table generation (log/alog) with proper zero guards
- Update `gf2m.c` to build tables at runtime
- `tests/test_gf2m.c`: Test field axioms, inverses, random vectors

**1.3 Polynomial Algebra**
- `backend/poly.h`:
  - `poly_t` structure (coefficients + degree)
  - Over GF(2): `poly_add()`, `poly_mul()`, `poly_mod()`, `poly_gcd()`
  - Over GF(2^m): use `gf2m_ctx*` for coefficients
  - `poly_eval()`, `poly_deriv()`, `poly_div_rem()`
- `src/poly.c`: Implement all operations
- `tests/test_poly.c`: Test small examples, benchmark m≤12

**Deliverables**: Full math library with tests passing

---

### PHASE 2: Source Coding + Validation (M2)
**Goal**: Complete Huffman, validate Hamming thoroughly

**2.1 Huffman Dynamic Coding**
- `backend/huffman.h`:
  - `huffman_params` with frequency table pointer
  - Header format `HUF1` spec
- `src/huffman.c`:
  - Frequency analysis (257 symbols: 0-255 + EOF)
  - Min-heap for tree building
  - Canonical Huffman (optional optimization)
  - Encoder: emit codes via `bitio`
  - Decoder: rebuild tree from header, decode stream
- `tests/test_huffman.c`:
  - Empty file
  - 1-symbol file
  - Uniform/random distributions
  - Round-trip golden tests
- Integrate with `codectk_codec` vtable

**2.2 Hamming Validation**
- Existing implementation looks solid, but needs:
  - `tests/test_hamming.c`:
    - Test all m=3..7
    - Flip 0, 1, 2 bits (expect success for ≤1)
    - Boundary packing tests
    - Random vectors
  - Optional: Add SECDED (overall parity) toggle

**Deliverables**: Huffman + Hamming both production-ready

---

### PHASE 3: Heavy Channel Codes (M3)
**Goal**: BCH and Goppa implementations

**3.1 BCH Codes**
- **Precompute Step** (`bch_init()` or similar):
  - Choose m (≤12 initially), t
  - Compute minimal polynomials for α^1, α^3, ..., α^(2t-1)
  - Generator g(x) = lcm of minimal polys
  - Store in `bch_params` or context

- **Encode**:
  - Systematic: `x^(deg g) * M(x) mod g(x)`
  - Stream via `bitio`

- **Decode**:
  - Syndromes S₁..S₂ₜ
  - Berlekamp-Massey → Λ(x), Ω(x)
  - Chien search: roots of Λ(α^-i)
  - Forney: magnitudes (binary BCH ⇒ all 1)
  - Flip bits, return correction count

- **Tests** (`tests/test_bch.c`):
  - Known params: (m=4,t=1), (m=5,t=2), (m=7,t=3)
  - Random messages with ≤t errors → success
  - >t errors → `EDECODE`
  - Edge cases: all-zero, all-one, burst errors

- **Bench** (`bench/bench_bch.c`): Mbps vs Hamming

**3.2 Binary Goppa Codes (Patterson)**
- **Precompute** (`goppa_init()` or similar):
  - Field context for GF(2^m)
  - Support set L (distinct, non-roots of g)
  - Irreducible g(x) over GF(2^m), degree t

- **Encode**:
  - Build parity-check H from (L, g):
    - H_{j,i} = L_i^j / g(L_i) for j=0..t-1
  - Compute parity bits = H * message mod 2

- **Decode (Patterson)**:
  1. Build S(x) = Σ r_i/(x - L_i) mod g(x)
  2. T(x) = S(x)^-1 mod g(x)
  3. Quadratic split: find a(x), b(x) s.t. a² + xb² ≡ T (mod g)
  4. Error locator from a, b
  5. Evaluate at L to find positions
  6. Flip bits

- **Tools**: `tools/gen_goppa_poly.py` for irreducible g(x), random L

- **Tests** (`tests/test_goppa.c`):
  - Small params: m=8, t=2..4
  - Random error patterns ≤t
  - Compare with Python reference

**Deliverables**: BCH + Goppa working, tested, benchmarked

---

### PHASE 4: Polish & Production (M4)
**Goal**: UX, performance, robustness

**4.1 CLI Tool**
- `tools/pipe.c`:
  - `huffman -> hamming|bch|goppa` encode pipeline
  - Reverse decode pipeline
  - File/stream I/O
  - Header/param TLV format

**4.2 ASM Backends (Optional)**
- x86-64: PCLMULQDQ for poly mul, AVX2 for log/alog
- ARM64: PMULL path
- Backend selection via `gf2m_backend` vtable
- `bench/micro_gf2m.c`: cycles/op for mul/inv/sqr

**4.3 Security Hardening**
- Remove secret-dependent branches in decode paths
- Masked table reads for log/alog (if crypto context)
- Audit memory wipes

**4.4 Robustness**
- Parameter validation (m bounds, t bounds, buffer sizes)
- Clear error returns
- Fuzzers: `tests/fuzz_*.c` for random/truncated streams
- Property tests: encode→noise→decode round-trip

**4.5 Documentation**
- `README.md`: Feature matrix, complexity table
- `docs/FORMAT.md`: Wire formats, headers, padding
- `docs/PARAMS.md`: Choosing m,t, rate vs correction charts

**4.6 CI/CD**
- GitHub Actions: build (Linux/macOS/Windows)
- Tests + sanitizers (ASan/UBSan/MSan)
- Code coverage badge

**4.7 Packaging**
- `pkg-config` file
- Version header
- Install targets: headers + libs

**Deliverables**: Production-ready library

---

## Priority Task List (Next Actions)

### IMMEDIATE (Do First):
1. ✅ **Read all files** - DONE
2. ⚠️ **Write CMakeLists.txt** - Basic build working
3. ⚠️ **Implement GF(2) ops** (`gf2.h/c`) - Foundation for everything
4. ⚠️ **Implement Polynomial ops** (`poly.h/c`) - Required for BCH/Goppa
5. ⚠️ **Add GF(2^m) init functions** - Table generation
6. ⚠️ **Test Hamming thoroughly** - Validate what exists

### SHORT-TERM (This Week):
7. ⚠️ **Implement Huffman** - Complete M2
8. ⚠️ **Start BCH implementation** - Encode first
9. ⚠️ **Create test framework** - Organized test suite

### MEDIUM-TERM (This Month):
10. ⚠️ **Complete BCH** - Full encode/decode/test
11. ⚠️ **Implement Goppa** - Patterson decoder
12. ⚠️ **Write benchmark suite** - Performance validation

### LONG-TERM (Nice-to-Have):
13. ⚠️ **ASM optimization** - Backend swapping
14. ⚠️ **CLI tool** - User-facing pipeline
15. ⚠️ **Documentation** - README, format specs, param guides
16. ⚠️ **CI/CD** - Automated testing, packaging

---

## Key Implementation Notes

### For BCH:
- Minimal polynomials can be precomputed tables for m≤12
- Generator g(x) is product of minimal polys (use poly_lcm)
- Berlekamp-Massey is ~20 lines but subtle
- Chien search is exhaustive α^0, α^1, ..., α^(n-1)

### For Goppa:
- Patterson is more complex than basic syndrome decoding
- Quadratic splitting (a² + xb² ≡ T) needs special algorithm
- Can use Extended Euclidean Algorithm approach
- Support set L must avoid roots of g(x)

### For Polynomial Ops:
- GF(2) polys: just XOR for add, careful mul with carryless mult
- GF(2^m) polys: each coeff is field element, use `gf2m_*` ops
- Need both representations, or template-like interface

### Testing Strategy:
- Unit tests: one operation at a time
- Integration tests: full encode→decode cycles
- Property tests: algebraic invariants
- Golden tests: known good vectors from papers/standards
- Fuzz tests: random/malformed inputs

---

## Questions / Decisions Needed

1. **Directory structure**: Keep `backend/` or move to `include/`?
   - Current: headers in `backend/`, sources in `src/`
   - Standard: headers in `include/`, sources in `src/`
   - Recommendation: Add `include/` symlinks to `backend/` for pkg-config

2. **Poly representation**:
   - Option A: Separate `poly_gf2_t` and `poly_gf2m_t` types
   - Option B: Single `poly_t` with field ctx pointer
   - Recommendation: Option A for type safety

3. **Table generation**:
   - Option A: Precomputed static tables for common (m,prim) pairs
   - Option B: Runtime generation only
   - Recommendation: Hybrid - static for m≤8, runtime for larger

4. **Error handling**:
   - Current: enum `codectk_err`
   - Question: Add error messages/strings?
   - Recommendation: Add `codectk_strerror(err)` later

5. **Streaming semantics**:
   - Current: `bitio` handles partial bytes well
   - Question: Should codecs support partial blocks?
   - Recommendation: Document block alignment requirements

---

## Success Metrics

- [ ] All code compiles with `-Wall -Wextra -Wconversion` clean
- [ ] All unit tests pass
- [ ] Hamming: 1000+ encode/decode cycles, 100% success on ≤1 error
- [ ] BCH: Successfully corrects random t errors for m=4,5,7
- [ ] Goppa: Successfully corrects random t errors for m=8
- [ ] Huffman: Round-trip on 100KB+ files with <5% overhead
- [ ] Benchmarks: >1 Mbps encode/decode on modest hardware
- [ ] Zero ASan/UBSan/MSan violations
- [ ] Code coverage >80% on core paths

---

## Timeline Estimate

- **Phase 0** (Build foundation): 1-2 days
- **Phase 1** (Math primitives): 3-5 days
- **Phase 2** (Huffman + validation): 2-3 days
- **Phase 3** (BCH + Goppa): 5-7 days
- **Phase 4** (Polish): 3-5 days

**Total**: ~2-3 weeks for core implementation, +1 week for polish

---

## Conclusion

This project has a **solid foundation** with excellent structure. The Hamming implementation is complete and can serve as a reference for the codec vtable pattern. The main work ahead is:

1. **Filling empty files**: gf2, poly, huffman
2. **Completing stubs**: BCH, Goppa
3. **Testing everything**: Comprehensive test suite
4. **Building infrastructure**: CMake, CI, docs

The mathematical algorithms (Berlekamp-Massey, Patterson) are well-documented in the TODOs, so implementation is primarily translating known algorithms into clean C code.

**Next immediate action**: Implement CMakeLists.txt to get building, then tackle GF(2) and polynomial operations as they're critical dependencies.

---

*Generated by Claude on 2025-10-31*
