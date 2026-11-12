#!/usr/bin/env python3
"""gen_prototypes.py — emit C++ forward declarations for the sketch functions.

Part of the P1P2MQTT fork, CC BY-NC-ND 4.0 with the exceptions in LICENSE.md.

Why this exists
---------------
Arduino sketches may call a function before it is defined; the Arduino IDE and
PlatformIO both insert prototypes for you. A plain host compiler does not, so the
host build would fail on the firmware's own call order (for example writePseudoPacket()
calls process_for_mqtt(), which is defined 2500 lines later).

This script reproduces that one preprocessor step for the host build:
  python3 gen_prototypes.py path/to/P1P2MQTT-bridge.ino > build/E_SERIES/prototypes.h

The firmware sources are never modified; this only produces a build artifact. If a
function is renamed or removed upstream, the declaration list simply follows the
source - and if a declaration here no longer matches, the host build fails loudly
instead of silently testing something different from the firmware.

Default arguments are dropped from the declarations (adding a default later, in the
definition, is valid C++), so the definitions stay untouched.
"""

import re
import sys

SKIP_NAMES = {
    # keywords that can start a line and are not function definitions
    "if", "for", "while", "switch", "return", "sizeof", "do", "else", "case", "struct",
    "typedef", "class", "enum", "union", "namespace", "using", "public", "private",
    "protected", "template", "operator",
}


def strip_noise(text):
    """Blank out comments and string/char literals, keeping offsets and newlines."""
    out = list(text)
    i, n = 0, len(text)
    state = "code"
    while i < n:
        c = text[i]
        nxt = text[i + 1] if i + 1 < n else ""
        if state == "code":
            if c == "/" and nxt == "/":
                state = "line"
                out[i] = " "
                out[i + 1] = " "
                i += 2
                continue
            if c == "/" and nxt == "*":
                state = "block"
                out[i] = " "
                out[i + 1] = " "
                i += 2
                continue
            if c == '"' or c == "'":
                state = c
                out[i] = " "
                i += 1
                continue
        elif state == "line":
            if c == "\n":
                state = "code"
            else:
                out[i] = " "
        elif state == "block":
            if c == "*" and nxt == "/":
                out[i] = " "
                out[i + 1] = " "
                state = "code"
                i += 2
                continue
            if c != "\n":
                out[i] = " "
        else:  # inside a " or ' literal
            if c == "\\":
                out[i] = " "
                if i + 1 < n:
                    out[i + 1] = " "
                i += 2
                continue
            if c == state:
                state = "code"
            out[i] = " "
        i += 1
    return "".join(out)


FUNC_RE = re.compile(
    r"""^[ \t]*
    ((?:(?:static|inline|extern|virtual)[ \t]+)*)            # 1 storage
    ((?:const[ \t]+)?[A-Za-z_][A-Za-z0-9_:<>, \t\*&]*?)      # 2 return type
    [ \t\*&]+
    ([A-Za-z_][A-Za-z0-9_]*)                                 # 3 name
    [ \t]*\(([^;{}]*)\)                                      # 4 parameters
    [ \t]*(?:const[ \t]*)?
    [ \t]*(\{)""",
    re.X | re.M,
)


def split_params(params):
    """Split a parameter list on top-level commas."""
    args, depth, cur = [], 0, ""
    for ch in params:
        if ch in "(<[":
            depth += 1
        elif ch in ")>]":
            depth -= 1
        if ch == "," and depth == 0:
            args.append(cur)
            cur = ""
        else:
            cur += ch
    if cur.strip():
        args.append(cur)
    return args


def declaration(return_type, storage, name, params):
    args = []
    for a in split_params(params):
        a = a.strip()
        if not a:
            continue
        a = a.split("=", 1)[0].strip()          # drop the default argument
        if a == "void":
            continue
        a = re.sub(r"\s+", " ", a)
        args.append(a)
    return f"{storage}{return_type.strip()} {name}({', '.join(args)});"


def main():
    if len(sys.argv) != 2:
        print("usage: gen_prototypes.py <path to P1P2MQTT-bridge.ino>", file=sys.stderr)
        return 2
    src = open(sys.argv[1], "r", errors="replace").read()
    clean = strip_noise(src)

    # Only look at brace depth 0: definitions inside functions (lambdas) are skipped.
    depth = 0
    lines = clean.split("\n")
    # rebuild a text where depth>0 regions are blanked, line by line
    blanked = []
    for line in lines:
        keep = []
        for ch in line:
            if ch == "{":
                keep.append(ch if depth == 0 else " ")
                depth += 1
            elif ch == "}":
                depth -= 1
                keep.append(ch if depth == 0 else " ")
            else:
                keep.append(ch if depth == 0 else " ")
        blanked.append("".join(keep))
    top = "\n".join(blanked)

    decls = []
    seen = set()
    for m in FUNC_RE.finditer(top):
        storage, rtype, name, params, _brace = m.groups()
        rtype = rtype.strip()
        if not rtype or name in SKIP_NAMES or rtype in SKIP_NAMES:
            continue
        if rtype in ("return", "else"):
            continue
        # "type name(...)" where the "return type" is actually the last word of a
        # variable declaration such as `const char * x[] = ...` never matches, but
        # `byte x[3] = {` can look like a call - guard against it:
        if "[" in rtype or "=" in rtype or "{" in rtype:
            continue
        key = name
        if key in seen:
            continue
        seen.add(key)
        decls.append(declaration(rtype, storage, name, params))

    print("// generated by test/host/gen_prototypes.py - do not edit, do not commit")
    print("// forward declarations for the sketch, the Arduino IDE does the same thing")
    print("#ifndef HOST_SKETCH_PROTOTYPES_H")
    print("#define HOST_SKETCH_PROTOTYPES_H")
    for d in decls:
        print(d)
    print("#endif")
    return 0


if __name__ == "__main__":
    sys.exit(main())
