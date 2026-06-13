# R-ECS Documentation

<p align="center">
  <b>🌐 Language / Язык</b><br>
  <a href="./en/README.md">🇬🇧 English</a>
  &nbsp;·&nbsp;
  <a href="./ru/README.md">🇷🇺 Русский</a>
</p>

---

R-ECS is an archetype-based **Entity-Component-System** framework for modern
C++ (C++20). This folder holds the full documentation set: hand-written guides
plus an API reference generated from the source docstrings.

| | |
|---|---|
| 🇬🇧 **English docs** | [`en/README.md`](./en/README.md) |
| 🇷🇺 **Русская документация** | [`ru/README.md`](./ru/README.md) |
| 📦 **Source code** | [`../include`](../include) |
| 🚀 **Examples** | [`../examples`](../examples) |

## How the docs are organised

```
docs/
├── README.md              ← you are here (language switcher)
├── doxygen/               ← Doxygen + doxybook2 configuration
│   ├── Doxyfile.in            template (placeholders filled per language)
│   ├── doxybook.en.json       doxybook2 settings — English
│   └── doxybook.ru.json       doxybook2 settings — Russian
├── scripts/
│   └── generate-docs.sh   ← one command rebuilds every language
├── en/                    ← English landing page + guides + generated api/
│   ├── README.md
│   ├── guides/
│   └── api/               (generated — git-ignored)
└── ru/                    ← Russian landing page + guides + generated api/
    ├── README.md
    ├── guides/
    └── api/               (generated — git-ignored)
```

The `en/api` and `ru/api` folders are produced by the generator and are
**git-ignored** — they are not committed, only rebuilt on demand.

## Building the API reference

The generator turns the C++ docstrings into Markdown with two tools:

```
include/*.hpp ──doxygen──▶ XML ──doxybook2──▶ Markdown (docs/<lang>/api)
```

**Prerequisites**

| Tool | Install |
|------|---------|
| [Doxygen](https://www.doxygen.nl/) ≥ 1.9 | `brew install doxygen` · `apt-get install doxygen` |
| [doxybook2](https://github.com/matusnovak/doxybook2) | download a release binary and put it on your `PATH` |

**Run it**

```bash
# from the repository root
docs/scripts/generate-docs.sh          # build all languages (en, ru)
docs/scripts/generate-docs.sh en       # build only English
LANGUAGES="en ru" docs/scripts/generate-docs.sh
```

## Adding a new language

1. Add the code and its Doxygen `OUTPUT_LANGUAGE` to the `LANG_TABLE` in
   [`scripts/generate-docs.sh`](./scripts/generate-docs.sh)
   (e.g. `[de]="German"`).
2. Copy `doxygen/doxybook.en.json` to `doxygen/doxybook.<code>.json` and set
   `baseUrl` to `/<code>/api/`.
3. Create `docs/<code>/README.md` and `docs/<code>/guides/` (you can start by
   copying the English tree and translating it).
4. Re-run the generator.

> **Note on translation scope.** Doxygen's `OUTPUT_LANGUAGE` localizes the
> structural labels it emits. The hand-written landing pages and guides are
> fully translated per language. To translate the auto-generated section
> headers that doxybook2 prints (e.g. *"Public Functions"*), drop a localized
> doxybook2 template set in `doxygen/templates.<code>/` and point the config's
> `templates` key at it.
