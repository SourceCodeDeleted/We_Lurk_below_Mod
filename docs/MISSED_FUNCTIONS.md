# Finding what IDA missed: rules

How functions, and arguments of functions, that IDA's analysis missed were found in this project, the
rules to follow whenever the program is analysed or exported again, and what to do better next time.
The numbers are from the reports the passes wrote (`ida/*_report.txt`, not in git) and from
`draft/build/fixups.json` and `draft/build/manifest.json`.

## What was found, and how

| Route | Found | Report |
|---|---:|---|
| Scan of the data sections for pointers into code (`tools/ida/find_pointer_functions.py`) | 3 functions (0x40AAB0, 0x40AB40, 0x40FFC0; all module start/stop table entries) | `ptrfuncs_report.txt`: 3028 → 3031 |
| Relocated code pointers that lead to no recovered function (`tools/draft_data.py`), handed to `tools/ida/add_functions.py` | 17 functions: 14 material handlers at 0x40EFA0–0x410610, `gge_material_set_shader_vars`, `create_thin_eye_warp_list`, `monster_do_thin_eye_seek` | `addfuncs_report.txt` |
| Functions IDA knew but the whole-program export left out | `nullsub_1` (0x467580) | `addfuncs_report.txt` |
| Register arguments missing from prototypes (`tools/ida/recover_register_args.py`) | 30 fixed, 1 partly in the first run; 2 more in the second | `regargs_report.txt`, `regargs_report2.txt` |
| Math helpers returning a double in `xmm0` (`tools/ida/retype_libm.py`) | 3 imports; 139 callers re-decompiled | `libm_report.txt` |
| Wrong conventions found from game bugs, fixed by hand (`db/prototype_fixes.json`) | 2 (0x44EE70, 0x416E60) | `blueprint_proto_report.txt` |

What is left, and why it is accepted:

- **19 relocated pointers into code still lead to no recovered function.** All point into the C runtime: `pre_c_initialization`, `std::exception` members, `type_info`'s destructor, and two exception-handler blocks inside `__scrt_common_main_seh` (+0x121, +0x135). The rebuilt program uses the host's own runtime, so none of them is needed.
- **110 of IDA's 3,031 functions are not in the generated program:** 44 thunks, 40 library functions, and 26 more, all in the C runtime start-up range from 0x582D30. Two functions are in the program but not in IDA's original list (0x4083A0, 0x492C00); they were added after the export.
- **258 uses of a lost value remain** (`draft/build/uninitialised.txt`). These are the open part of the argument problem.

## Rules: every time the program is analysed or exported again

### Finding functions

1. **Check every relocated code pointer.** Every relocation slot in the data sections that points into the code section must lead to the start of a recovered function.
   - `tools/draft_data.py` prints the ones that do not ("pointers into other code"). The expected list is the 19 C-runtime targets above.
   - Any new entry is a missed function until shown otherwise.
2. **A target inside another function is its own function.** IDA attaches code to the function that jumps to it with a tail call. When something else points at that code, it is a function of its own.
   - Detach it (`add_functions.py` does this) and re-decompile both: the former owner changes too.
3. **Without relocations, scan.** Every 4-byte value in `.rdata` and `.data` that points into `.text` outside a known function is a candidate if it is 16-byte aligned and decodes as code.
   - Repeat until nothing new appears: new functions reveal more code.
   - Treat these as lower confidence than rule 1.
4. **The export must contain every function IDA has.** Compare the function list (`dump/index.json`) with the generated program (`draft/build/manifest.json`). Every difference must be a thunk, a library function, or C runtime start-up code.
   - Anything else was skipped by the export (as `nullsub_*` were) and must be exported separately.
5. **Nothing may stay undefined at link time.** `tools/draft_symbols.sh` lists symbols no object defines. An undefined name that is not an import is a missing function.
6. **A crash into address 0, or into an original address, is a missed pointer.** The trace ring (`tools/native_trace.py`) shows the last functions entered.
   - Look up the slot the call went through in `draft/build/src/_image.c`.
   - Then apply rule 1 to it.

### Finding missed arguments

7. **Every use of an unset variable is a lost value until explained.** Regenerate `draft/build/uninitialised.txt` (`tools/draft_uninit.sh`) after every export and compare the count with the last one. It must not rise.
8. **Run `recover_register_args.py` on the list, then again until a run fixes nothing.**
   - The script keeps a change only when the decompiler's undefined-variable count falls, and queues the callers of every function it changed.
   - Callers must then be exported again.
9. **Read the "untouched" cases by category, not one by one.** In the first run the 194 untouched functions split into:

   | Category | Count | What it usually means |
   |---|---:|---|
   | Prototype not expressible (the register is already a `__fastcall`/`__thiscall` argument, or the callee pops its own arguments) | 80 | The convention guess itself is wrong; read the call sites |
   | Undefined, but not read before written at entry | 57 | The value is lost across a call, not an argument: the callee was assumed to clobber it |
   | Read at entry, but not reported undefined | 22 | Probably harmless, but check the callers |
   | Undefined, and also read at entry | 6 | The script's choice did not help; check by hand |
   | Neither | 28 | The undefined variable is not a register; look elsewhere |

   The second run's split was similar (86 / 47 / 26 / 18 / 24).
10. **Never trust a named convention for an internal function.** The binary was built with link-time optimisation, and the `__cdecl` in its signature strings is not what the code does.
    - Who removes stack arguments is measured: `ret n` in the callee, the stack adjustment after each call in the caller.
    - That is how 0x44EE70 was found.
11. **Check what every caller sets up before the call.** A register that every caller loads just before the call, and that the callee reads before writing, is an argument.
    - That is how 0x416E60 was found. It had cost the game every building resident without any warning.
12. **Imports with no prototype need one.** List every called import that IDA has no type for, and check its register use. The `_libm_sse2_*_precise` helpers take and return a double in `xmm0`; without that their argument vanished at 139 call sites.

### Working rules

13. **Never edit generated output.** Every fix is either a named rule in `tools/draft_fix.py` or an entry in a `db/` file. Hand prototypes go in `db/prototype_fixes.json` with a `why` that names the evidence.
14. **Work on a copy of the IDA database.** Make a backup before any script that changes it (`ida/ILB.before_pointer_funcs.i64` is one). Run read-only queries on a scratch copy.
15. **After any prototype change, re-decompile its callers.** Their argument lists change with it.
16. **Keep every pass's report and write the totals down.** The counts in the table at the top are the baseline. A pass that makes any of them worse is reverted.

## Doing better next time

These are known weaknesses of the passes above. They are proposals, not done.

- **Explain the C runtime targets instead of listing them.** Parse the exception scope tables and the runtime's start-up tables, so that the 19 known targets are classified automatically. Only truly unknown ones would then be reported.
- **Follow paths past calls.** `recover_register_args.py` stops each path at the first call, so a register first read after a call is not seen as an input. With per-function clobber sets (which registers a callee really changes), the path can continue. This also addresses the 57 "lost across a call" cases.
- **Derive the convention from the call sites for the 80 "not expressible" cases.** That means rule 11, done by a script instead of by hand.
- **Use table membership.** Handlers in one pointer table (the mode table, the emitter table, the material handler table) are called the same way, so they take the same arguments. A table entry whose recovered prototype differs from its neighbours is suspect.
- **One driver for all of it.** Run rules 1, 4, 5 and 7 in one script that prints the counts next to the baseline and fails when any count rises. Today they are separate steps.
- **Do it without IDA.** The same rules are the specification for Anastasia's function discovery and argument recovery (`docs/recovery-capabilities.md` in the Anastasia repository, sections A1–A3). Anastasia already takes its pointer targets from the relocation table, which is rule 1, and is more exact than rule 3.
