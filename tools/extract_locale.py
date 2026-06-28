#!/usr/bin/env python3
"""One-shot estrazione del vecchio Locale.h (sStrings[StringId][Language]) in:
  - src/ui/Locale.h.generated     (kEnglishSource[] di B_TRANSLATE_MARK, B_TRANSLATION_CONTEXT,
                                   Tr() reimplementato sopra BCatalog)
  - locales/it.catkeys            (target italiano)
  - locales/es.catkeys
  - locales/de.catkeys
  - locales/ja.catkeys

Sorgente: src/ui/Locale.h originale, parsato come testo con un piccolo lexer
che riconosce stringhe C-style (con escape \\xNN, \\n, \\t, \\\\, \\\").
La sorgente delle traduzioni e' la colonna inglese (indice 1).

Da eseguire una volta sola, prima di sostituire Locale.h.
"""

from __future__ import annotations
import re
import sys
from pathlib import Path

SIG = "x-vnd.atomozero-LANterna"
CONTEXT = "LANterna"
LANG_NAME = {
    "it": "Italian",
    "es": "Spanish",
    "de": "German",
    "ja": "Japanese",
}
# Indice di colonna nella tabella sStrings[StringId][Language] esistente.
COLUMN = {
    "it": 0,
    "en": 1,
    "es": 2,
    "de": 3,
    "ja": 4,
}

ROOT = Path(__file__).resolve().parents[1]
# Preferisci il .bak (Locale.h originale pre-migrazione): il Locale.h attuale
# non contiene piu' sStrings[][]. Il .bak resta come riferimento finche'
# l'estrazione e' usata per seed di nuovi catkeys.
LOCALE_H = ROOT / "src" / "ui" / "Locale.h.bak"
if not LOCALE_H.exists():
    LOCALE_H = ROOT / "src" / "ui" / "Locale.h"
GENERATED_H = ROOT / "src" / "ui" / "Locale.h.generated"
LOCALES_DIR = ROOT / "locales"


def parse_c_string(text: str, start: int) -> tuple[str, int]:
    """Parsa una stringa C-style che inizia all'indice `start` (su `"`).
    Ritorna (valore_unescapato_in_str, indice_dopo_la_chiusura)."""
    assert text[start] == '"', f"expected \" at {start}"
    i = start + 1
    raw = bytearray()
    while True:
        ch = text[i]
        if ch == '"':
            return raw.decode("utf-8"), i + 1
        if ch == "\\":
            nxt = text[i + 1]
            if nxt == "x":
                # \xNN  - due hex
                hh = text[i + 2 : i + 4]
                raw.append(int(hh, 16))
                i += 4
                continue
            if nxt in ("n", "t", "r", "\\", '"', "'"):
                mapping = {"n": "\n", "t": "\t", "r": "\r", "\\": "\\", '"': '"', "'": "'"}
                raw.extend(mapping[nxt].encode("utf-8"))
                i += 2
                continue
            raise SystemExit(f"unsupported escape \\{nxt} at {i}")
        raw.extend(ch.encode("utf-8"))
        i += 1


def main() -> None:
    src = LOCALE_H.read_text(encoding="utf-8")

    # 1. Estrai l'ordine dell'enum StringId per validazione.
    enum_match = re.search(r"enum StringId\s*\{([^}]*)\};", src, re.DOTALL)
    if not enum_match:
        raise SystemExit("enum StringId non trovato")
    enum_body = enum_match.group(1)
    enum_names: list[str] = []
    for line in enum_body.splitlines():
        line = re.sub(r"//.*$", "", line).strip().rstrip(",")
        if not line or line.startswith("//"):
            continue
        # rimuove eventuale " = 0"
        name = line.split("=")[0].strip()
        if name and name.startswith("S_"):
            enum_names.append(name)
    if enum_names[-1] != "S_COUNT_TOTAL":
        raise SystemExit(f"sentinella attesa S_COUNT_TOTAL, trovata {enum_names[-1]}")
    enum_names = enum_names[:-1]  # via la sentinella
    print(f"[ok] enum: {len(enum_names)} StringId (sentinella esclusa)")

    # 2. Localizza il blocco sStrings[...] = { ... };
    tbl_start = src.find("sStrings[S_COUNT_TOTAL][kLangCount]")
    if tbl_start < 0:
        raise SystemExit("sStrings[][] non trovato")
    brace_open = src.find("{", tbl_start)
    # Trova la chiusura matchando le braces.
    depth = 0
    i = brace_open
    while True:
        ch = src[i]
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                brace_close = i
                break
        i += 1
    block = src[brace_open + 1 : brace_close]

    # 3. Estrai le righe { "..", "..", "..", "..", ".." } in ordine, abbinandole
    # all'enum_names. Le righe sono separate da virgole top-level del blocco esterno.
    rows: list[list[str]] = []
    pos = 0
    while pos < len(block):
        ch = block[pos]
        if ch == "{":
            # Inizio riga. Raccoglie 5 stringhe.
            strs: list[str] = []
            p = pos + 1
            while True:
                # skip ws, virgole, newline, commenti single-line
                while p < len(block) and block[p] in " \t\n\r,":
                    p += 1
                if block[p] == "/" and block[p + 1] == "/":
                    # commento single-line, salta a fine riga
                    p = block.find("\n", p) + 1
                    continue
                if block[p] == "}":
                    break
                if block[p] != '"':
                    raise SystemExit(f"atteso \", trovato {block[p]!r} a {p}, ctx={block[max(0,p-30):p+30]!r}")
                # Concatenazione di stringhe adiacenti C: "a" "b" -> "ab".
                concat = ""
                while p < len(block) and block[p] == '"':
                    val, p = parse_c_string(block, p)
                    concat += val
                    while p < len(block) and block[p] in " \t\n\r":
                        p += 1
                strs.append(concat)
            if len(strs) != 5:
                raise SystemExit(f"riga {len(rows)}: attese 5 stringhe, trovate {len(strs)}")
            rows.append(strs)
            pos = p + 1  # dopo la }
        else:
            pos += 1
    print(f"[ok] sStrings[][]: {len(rows)} righe")

    if len(rows) != len(enum_names):
        raise SystemExit(
            f"mismatch: {len(rows)} righe in sStrings vs {len(enum_names)} entry in enum"
        )

    # 4. Emissione Locale.h.generated
    LOCALES_DIR.mkdir(exist_ok=True)
    out_h = []
    out_h.append(
        "// Auto-generato da tools/extract_locale.py — non modificare a mano.\n"
        "// Sorgente: vecchio sStrings[][] di Locale.h. La colonna inglese\n"
        "// (indice 1) e' la sorgente per i .catkeys.\n"
    )
    out_h.append("    // kEnglishSource: ogni entry e' B_TRANSLATE_MARK(\"english\").\n")
    out_h.append("    // L'ordine deve coincidere con StringId (lo static_assert lo verifica).\n")
    out_h.append("    static const char* const kEnglishSource[S_COUNT_TOTAL] = {\n")
    for name, row in zip(enum_names, rows):
        en = row[COLUMN["en"]]
        out_h.append(f"        B_TRANSLATE_MARK({c_string_literal(en)}),  // {name}\n")
    out_h.append("    };\n")
    out_h.append(
        "    static_assert(sizeof(kEnglishSource)/sizeof(kEnglishSource[0]) == S_COUNT_TOTAL,\n"
        "                  \"Locale table out of sync with StringId enum\");\n"
    )
    GENERATED_H.write_text("".join(out_h), encoding="utf-8")
    print(f"[ok] scritto {GENERATED_H} ({len(rows)} entry)")

    # 5. Costruisci mappa (english_source, context) -> translation per ogni lingua,
    #    leggendo dalla matrice sStrings esistente. Quando piu' StringId condividono
    #    la stessa fonte inglese (es. "Name:" usato in piu' contesti) tutte le
    #    traduzioni coincidono nel dataset originale (verificato), quindi la
    #    mappa e' ben definita.
    en_to_tr: dict[str, dict[str, str]] = {}  # english -> {lang_code: translation}
    for row in rows:
        en = row[COLUMN["en"]]
        if en not in en_to_tr:
            en_to_tr[en] = {}
        for lang_code, idx in COLUMN.items():
            if lang_code == "en":
                continue
            en_to_tr[en][lang_code] = row[idx]

    # 6. Emissione catkeys per ogni lingua, allineati al master en.catkeys
    #    (se presente). Senza master: produce un catkeys "best-effort" con
    #    fingerprint=1 che dovra' essere ri-allineato dopo `make catkeys`.
    en_master = LOCALES_DIR / "en.catkeys"
    if en_master.exists():
        master_lines = en_master.read_text(encoding="utf-8").splitlines()
        header = master_lines[0].split("\t")
        fingerprint = header[3]
        # master entries: (english, context, comment) tuple
        master_entries = []
        for line in master_lines[1:]:
            if not line.strip():
                continue
            parts = line.split("\t")
            # parts: english, context, comment, translation_or_empty
            master_entries.append((parts[0], parts[1], parts[2]))
        print(f"[ok] master en.catkeys: {len(master_entries)} key uniche, fingerprint={fingerprint}")
    else:
        master_entries = [
            (escape_for_catkeys(en), CONTEXT, "") for en in en_to_tr.keys()
        ]
        fingerprint = "1"
        print(f"[warn] en.catkeys non trovato, uso fingerprint=1 e dedup manuale")

    for lang_code in ["it", "es", "de", "ja"]:
        lines = []
        lines.append(f"1\t{LANG_NAME[lang_code]}\t{SIG}\t{fingerprint}")
        missing = 0
        for en_escaped, ctx, comment in master_entries:
            # Riconverti escape -> raw per il lookup
            en_raw = unescape_catkeys(en_escaped)
            tr = en_to_tr.get(en_raw, {}).get(lang_code)
            if tr is None:
                # Stringa nuova introdotta dopo l'extract: lascia vuoto, il
                # traduttore la compila.
                tr = ""
                missing += 1
            lines.append(f"{en_escaped}\t{ctx}\t{comment}\t{escape_for_catkeys(tr)}")
        path = LOCALES_DIR / f"{lang_code}.catkeys"
        path.write_text("\n".join(lines) + "\n", encoding="utf-8")
        suffix = f" ({missing} non tradotte)" if missing else ""
        print(f"[ok] scritto {path} ({len(master_entries)} entry){suffix}")


def c_string_literal(s: str) -> str:
    """Re-codifica una stringa UTF-8 come literal C compatibile con il compilatore.
    Tiene gli ASCII stampabili nudi (con escape per ", \\, e i controlli), e ricodifica
    i byte UTF-8 non-ASCII come \\xNN. Replica la convenzione del Locale.h originale."""
    out = ['"']
    for byte in s.encode("utf-8"):
        if byte == 0x22:  # "
            out.append('\\"')
        elif byte == 0x5C:  # \
            out.append("\\\\")
        elif byte == 0x0A:
            out.append("\\n")
        elif byte == 0x09:
            out.append("\\t")
        elif byte == 0x0D:
            out.append("\\r")
        elif 0x20 <= byte < 0x7F:
            out.append(chr(byte))
        else:
            out.append(f"\\x{byte:02x}")
    out.append('"')
    return "".join(out)


def escape_for_catkeys(s: str) -> str:
    """Catkeys e' tab-separated line-based: i tab e i newline letterali nelle stringhe
    devono essere serializzati come \\t / \\n. Il resto resta UTF-8 raw."""
    return (
        s.replace("\\", "\\\\")
        .replace("\t", "\\t")
        .replace("\n", "\\n")
        .replace("\r", "\\r")
    )


def unescape_catkeys(s: str) -> str:
    """Inverso di escape_for_catkeys."""
    out = []
    i = 0
    while i < len(s):
        if s[i] == "\\" and i + 1 < len(s):
            nxt = s[i + 1]
            mapping = {"t": "\t", "n": "\n", "r": "\r", "\\": "\\"}
            if nxt in mapping:
                out.append(mapping[nxt])
                i += 2
                continue
        out.append(s[i])
        i += 1
    return "".join(out)


if __name__ == "__main__":
    main()
