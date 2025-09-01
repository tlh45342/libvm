Next question how hard is going to be to implement QCOW and a VMWARE Volume?

###### ChatGPT said:

Thought for 5s

Short version: **QCOW2 and VMDK (VMware) are both doable**, but they’re very different beasts. If you scope smartly—**read-only first, one or two sub-formats each, no snapshots, no compression/encryption**—you can get something useful working in a couple of weeks. Full, production-grade, read-write with snapshots/backing chains/compression is a multi-month project.

Here’s how I’d slice it for **guppy**.

------

# Target: a “virtual block device” layer

Both formats should plug into a common interface:

```
// blockdev.h
typedef struct BlockDev BlockDev;

typedef struct {
  uint64_t size_bytes;
  uint32_t logical_sector;   // usually 512
  uint32_t physical_sector;  // often 4096
  uint32_t flags;            // READ_ONLY, HAS_BACKING, …
} BlkInfo;

int blk_open_qcow2 (const char* path, BlockDev** out);
int blk_open_vmdk  (const char* path, BlockDev** out);
int blk_read(BlockDev*, uint64_t lba, void* buf, uint32_t nsectors);
int blk_write(BlockDev*, uint64_t lba, const void* buf, uint32_t nsectors); // start as ENOTSUP
void blk_close(BlockDev*);
int blk_info(BlockDev*, BlkInfo*);
```

Your FAT/ext tooling then reads/writes via `blk_*` without caring about the container.

------

## QCOW2 (QEMU Copy-On-Write v2) — difficulty: **Medium → Hard**

### What you *need* for a useful, read-only MVP

- Parse **header** (note: **big-endian** fields).
- Support **cluster size** (commonly 64 KiB), **L1** and **L2 tables**.
- Implement **refcount table** minimally (mostly to know if clusters exist; for RO you can often get by without updating it).
- **Backing file** chain (optional for v1; for MVP, allow a single backing image and fall back to zeros if absent).
- Handle **unallocated** = zero reads.
- Skip features: **encryption**, **compression**, **internal snapshots**, **extended L2**.

### Read algorithm (simplified)

1. For each requested LBA:
   - Compute guest offset = `lba * 512`.
   - Map to **cluster**: `cluster_index = offset / cluster_size`, `intra = offset % cluster_size`.
   - From **L1[cluster_index / entries_per_L2]** get L2 table address.
   - Read **L2[cluster_index % entries_per_L2]** → physical cluster address.
   - If zero → return zero-filled sector; else pread from file at `phys_cluster + intra`.
2. Repeat across cluster boundaries.

### Gotchas

- Endianness (headers, tables are **big-endian**).
- **Extended L2** bits (compression, zero clusters) – ignore at first except zero flag.
- **Refcount** consistency not needed for RO; required for RW (hard).
- Backing chains can be nested; support **one level** at first.

### Effort (reasonable, RO only)

- Parser + mapper + tests: **4–7 days**.
- Add one-level backing file: **1–2 days**.
- RW with refcounts/snapshots: **4–8+ weeks**.

------

## VMDK (VMware disk) — difficulty: **Varies by flavor**

VMDK is a *family* of formats. Pick **two** to start:

1. **MonolithicSparse** (single file, sparse) — *best first target*
2. **TwoGbMaxExtentSparse** (split sparse extents) — easy after #1

**StreamOptimized** (used in OVF/OVA) and **HostedSparse** add streaming/compression quirks—save for later.

### What MonolithicSparse looks like

- A small **descriptor** (sometimes external `.vmdk` text; sometimes embedded) with geometry + extent info.
- One sparse extent with:
  - **Grain size** (e.g., 128 sectors = 64 KiB).
  - **Grain Table (GT)** and **Grain Directory (GD)** that map virtual grain → file offset.
  - Optional **Redolog** structures (skip at first).

### Read algorithm (simplified)

1. Parse descriptor → get grain size, counts, GD/GTE sizes.
2. For a guest LBA:
   - `grain = (lba * 512) / (grain_size_bytes)`.
   - Locate **GT** via **GD[grain / entries_per_GT]**.
   - Read **GTE = GT[grain % entries_per_GT]** → file offset of the grain.
   - If GTE == 0 → unallocated → zeros.
   - Else pread from `file_offset + intra_grain_offset`.

### Gotchas

- Descriptor may be **external** (two-file layout) or **embedded**.
- Different **grain sizes**; don’t assume 64 KiB.
- Little-endian on-disk.
- **TwoGbMaxExtentSparse** uses multiple extent files; the descriptor lists them → stitch as a virtual concatenation of extents, each with its own GD/GT space.
- **StreamOptimized** packs grains for network copy; involves markers and (optionally) deflate—postpone.
- RW requires grain allocation and updates to GT/GD; doable but a lot of edge cases.

### Effort (RO only)

- MonolithicSparse: **3–5 days**.
- TwoGbMaxExtentSparse: **1–2 days** once #1 done.
- StreamOptimized (RO): **3–6 days** (compression/markers).
- RW (allocate grains, zero detection): **3–6+ weeks** to be solid.

------

## Phased plan for **guppy**

**Phase 0 – Plumbing (1 day)**

- Add the `BlockDev` abstraction.
- Provide a raw file backend (flat images) for baseline.

**Phase 1 – VMDK RO (1 week)**

- MonolithicSparse reader.
- Optional: TwoGbMaxExtentSparse.
- Tests: create tiny images, verify reads return expected data/zeros.

**Phase 2 – QCOW2 RO (1–2 weeks)**

- Header/L1/L2 mapping, unallocated=zero.
- Optional: single backing image.

**Phase 3 – Integrate with copy tools (1–2 days)**

- Your FAT/ext writers consume from any BlockDev.

- Command like:

  ```
  guppy mount --image foo.qcow2 --as blk0
  guppy copy --from blk0:/path/in/fs --to target.img:/dst
  ```

  (Or straight sector reads if you’re layering a partition + FS parser above.)

**Phase 4 – Extras (later)**

- VMDK streamOptimized RO.
- QCOW2 compression/encryption (RO).
- Read-write paths (allocation, refcounts/grains, snapshots).

------

## Testing strategy

- Use `qemu-img` (or VMware Workstation) to **generate tiny fixtures**:
  - 8–64 MiB images, 512-byte sectors, known patterns.
  - Files at known offsets; verify readback.
- Fuzz **table indices** (bounds checks).
- Validate **zero regions** and **boundary crossings** (grain/cluster edges).
- Backing chain single-level for QCOW2 (child reads zeros vs parent data).

------

## Minimal on-disk struct hints

### QCOW2 (header sketch; big-endian)

```
struct Qcow2Header {
  uint32_t magic;          // 'QFI\xfb'
  uint32_t version;
  uint64_t backing_file_offset;
  uint32_t backing_file_size;
  uint32_t cluster_bits;   // cluster_size = 1u << cluster_bits
  uint64_t size;           // virtual disk size in bytes
  uint32_t crypt_method;
  uint32_t l1_size;
  uint64_t l1_table_offset;
  uint64_t refcount_table_offset;
  uint32_t refcount_table_clusters;
  // ...
};
```

### VMDK sparse extent header (little-endian, classic)

```
struct VmdkSparseHeader {
  uint32_t magicNumber;      // 0x564d444b 'VMDK'
  uint32_t version;
  uint32_t flags;
  uint64_t capacity;         // sectors
  uint64_t grainSize;        // sectors per grain
  uint64_t descriptorOffset; // sectors
  uint64_t descriptorSize;   // sectors
  uint32_t numGTEsPerGT;
  uint64_t rgdOffset;        // redundant GD (often present)
  uint64_t gdOffset;         // primary GD
  uint64_t overhead;         // sectors
  // ...
};
```

------

## Practical advice

- **Start read-only.** You’ll get 80% of the value fast.
- **Implement zero-fill fast paths** (unallocated → memset).
- **Cache** GD/GT/L2 tables (small LRU) to avoid death by seeks.
- Be strict on spec, but **tolerant** for common quirks (descriptor line endings, etc.).
- Keep **format-specific code separate** (`qcow2.c`, `vmdk_sparse.c`) under a shared `blockdev` interface.

If you want, I can draft the `BlockDev` interface and a small VMDK MonolithicSparse reader skeleton that compiles, so you can wire it to your copy path right away.