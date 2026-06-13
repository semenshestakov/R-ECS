#!/usr/bin/env bash
# ════════════════════════════════════════════════════════════════════════
#  R-ECS — documentation generator
#
#  Pipeline:   include/*.hpp  ──doxygen──▶  XML  ──moxygen──▶  Markdown
#
#  For every language listed in LANGUAGES the script:
#    1. renders docs/doxygen/Doxyfile.in with the language-specific values,
#    2. runs Doxygen to emit XML into docs/.build/<lang>/xml,
#    3. runs doxybook2 to turn that XML into Markdown under docs/<lang>/api.
#
#  The hand-written landing pages and guides under docs/<lang>/ are NOT
#  touched — only the generated docs/<lang>/api/ tree is (re)built.
#
#  Usage:
#      docs/scripts/generate-docs.sh            # build every language
#      docs/scripts/generate-docs.sh en         # build only English
#      LANGUAGES="en ru" docs/scripts/generate-docs.sh
# ════════════════════════════════════════════════════════════════════════
set -euo pipefail

# ── Paths ───────────────────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DOCS_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
ROOT_DIR="$(cd "${DOCS_DIR}/.." && pwd)"
DOXY_DIR="${DOCS_DIR}/doxygen"
BUILD_DIR="${DOCS_DIR}/.build"

# ── Language table:  <code>:<Doxygen OUTPUT_LANGUAGE>  ──────────────────
#  Add a new language by adding a case below and dropping a docs/<code>/
#  landing tree. Doxygen OUTPUT_LANGUAGE values are
#  listed in the Doxygen manual (English, Russian, German, French, ...).
lang_name() {
  case "$1" in
    en) echo "English" ;;
    ru) echo "Russian" ;;
    *)  echo "" ;;
  esac
}

LANGUAGES="${LANGUAGES:-${*:-en ru}}"

# ── Tool checks ─────────────────────────────────────────────────────────
need() {
  command -v "$1" >/dev/null 2>&1 || {
    echo "✗ '$1' not found." >&2
    echo "  $2" >&2
    exit 1
  }
}
need doxygen   "Install:  brew install doxygen   |   apt-get install doxygen"
need moxygen   "Install:  npm install -g moxygen"

# ── Project version (read from CMakeLists.txt) ──────────────────────────
PROJECT_NUMBER="$(grep -Eo 'VERSION[[:space:]]+[0-9]+\.[0-9]+\.[0-9]+' "${ROOT_DIR}/CMakeLists.txt" \
                  | head -1 | grep -Eo '[0-9]+\.[0-9]+\.[0-9]+' || echo '0.0.0')"

echo "▶ R-ECS docs — version ${PROJECT_NUMBER} — languages: ${LANGUAGES}"

for code in ${LANGUAGES}; do
  out_lang="$(lang_name "${code}")"
  if [[ -z "${out_lang}" ]]; then
    echo "✗ unknown language code '${code}' (add it to lang_name)" >&2
    exit 1
  fi

  xml_dir="${BUILD_DIR}/${code}"
  api_dir="${DOCS_DIR}/${code}/api"
  doxyfile="${BUILD_DIR}/Doxyfile.${code}"

  echo "  ─ [${code}] Doxygen → XML (${out_lang})"
  mkdir -p "${xml_dir}"
  sed -e "s#@PROJECT_NUMBER@#${PROJECT_NUMBER}#g" \
      -e "s#@OUTPUT_LANGUAGE@#${out_lang}#g" \
      -e "s#@OUTPUT_DIRECTORY@#${xml_dir}#g" \
      -e "s#@INPUT_PATHS@#${ROOT_DIR}/include ${ROOT_DIR}/README.md#g" \
      -e "s#@MAINPAGE@#${ROOT_DIR}/README.md#g" \
      "${DOXY_DIR}/Doxyfile.in" > "${doxyfile}"
  doxygen "${doxyfile}"

  echo "  ─ [${code}] moxygen → Markdown (docs/${code}/api)"
  rm -rf "${api_dir}"
  mkdir -p "${api_dir}"
  moxygen --anchors --classes --output "${api_dir}/%s.md" "${xml_dir}/xml"
done

echo "✓ Done. Open docs/README.md and pick a language."
