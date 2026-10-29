#!/usr/bin/env python3
"""HMS-NF-008: no function may be longer than 50 lines.

Usage:  python tools/check_function_length.py [folder ...]      (default: src)
Counts the lines of every function body (from the line with its opening brace to the line with its closing brace)
in .cpp and .h files. Lambdas and blocks inside a function count as part of that function.
Exit code 1 if any function is longer than the limit.
"""
import os
import re
import sys

LIMIT = 50


def strip(code):
    """Remove comments and string / char literals so braces inside them are ignored."""
    code = re.sub(r"//[^\n]*", "", code)
    code = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), code, flags=re.S)
    code = re.sub(r'"(\\.|[^"\\\n])*"', '""', code)
    code = re.sub(r"'(\\.|[^'\\\n])*'", "''", code)
    return code


def functions(path):
    code = strip(open(path, encoding="utf-8").read())
    stack = []          # kinds of open blocks: 'scope' (namespace / class / struct / enum) or 'func' or 'inner'
    head = ""           # text since the last ; { or }
    line = 1
    results = []
    for ch in code:
        if ch == "\n":
            line += 1
        if ch == "{":
            h = head.strip()
            inside_func = any(k[0] in ("func", "inner") for k in stack)
            if inside_func:
                stack.append(("inner", line))
            elif re.search(r"\b(namespace|class|struct|enum|union)\b[^()]*$", h) or h.endswith("=") or not re.search(r"\)", h):
                stack.append(("scope", line))
            else:
                name = re.findall(r"([~\w:]+)\s*\([^()]*(?:\([^()]*\)[^()]*)*\)\s*(?:const)?\s*(?:noexcept)?\s*(?:->\s*[\w:<>]+)?\s*(?::[^{]*)?$", h)
                stack.append(("func", line, name[-1] if name else h[-40:]))
            head = ""
        elif ch == "}":
            if stack:
                top = stack.pop()
                if top[0] == "func":
                    results.append((top[2], top[1], line, line - top[1] + 1))
            head = ""
        elif ch == ";":
            head = ""
        else:
            head += ch
    return results


def main():
    roots = sys.argv[1:] or ["src"]
    bad, total = [], 0
    for root in roots:
        for d, _, files in os.walk(root):
            for f in sorted(files):
                if not f.endswith((".cpp", ".h")):
                    continue
                p = os.path.join(d, f)
                for name, start, end, n in functions(p):
                    total += 1
                    if n > LIMIT:
                        bad.append(f"{p}:{start}: {name} has {n} lines (limit {LIMIT})")
    print(f"Checked {total} functions in {', '.join(roots)}")
    for b in bad:
        print("  TOO LONG: " + b)
    if bad:
        sys.exit(1)
    print(f"OK: every function has at most {LIMIT} lines (HMS-NF-008)")


if __name__ == "__main__":
    main()
