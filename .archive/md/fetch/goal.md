



Yep—we can make this dead simple: a tiny **mask/value table** in `execute.c` and keep each `handle_*` as a single function that assumes “if I’m called, it’s my opcode.” No key12, no wrappers, no per-op tables elsewhere.



{ K12_MASK_DP_OPCODE, K12_VAL_DP(0x4), 0, 0, true, handle_add, "ADD" },
Mask/Value: matches DP class (00xxxx) + opcode 0100 (ADD).

xmask32/xvalue32: 0 so we don’t over-constrain (works for both reg and imm).

check_cond: true so your try_decode_key12() does the condition check.

fn: handle_add (your simplified, self-contained version taking instr).

That’s all you need—slide that single line into your K12_TABLE[] and you’re set.

Got it 👍 — you want a single, literal **`{ mask12, value12, xmask32, xvalue32, check_cond, fn, "ADD" }`** row, no helper macros, no indirection.



