// Make moxygen-generated Markdown safe for VitePress (Vue) rendering.
// Escapes stray < / > (C++ templates, #include <...>, etc.) everywhere
// EXCEPT inside fenced code, inline code, and a whitelist of real HTML tags
// (so moxygen's <a id="..."></a> anchors keep working).
//
// Usage:  node sanitize-md.mjs <dir> [<dir> ...]
import fs from 'node:fs'
import path from 'node:path'

const SENT = String.fromCharCode(0)
const KEEP_HTML =
  /<\/?(?:a|br|hr|sub|sup|b|i|em|strong|small|kbd|p|ul|ol|li|table|thead|tbody|tr|td|th|div|span|details|summary|img)\b[^>]*\/?>/gi

// In GFM tables a literal "|" inside a cell (even within `code`) must be
// escaped as "\|", otherwise it is read as a column separator. moxygen emits
// operator names like `operator|` unescaped, so fix them here.
function escapeTablePipes(text) {
  return text
    .split('\n')
    .map((line) =>
      /^\s*\|/.test(line)
        ? line.replace(/`[^`\n]*`/g, (code) => code.replace(/(?<!\\)\|/g, '\\|'))
        : line,
    )
    .join('\n')
}

function sanitize(text) {
  const stash = []
  const keep = (m) => SENT + (stash.push(m) - 1) + SENT
  let t = escapeTablePipes(text)
    .replace(/```[\s\S]*?```/g, keep) // fenced code blocks
    .replace(/`[^`\n]*`/g, keep) // inline code spans
    .replace(KEEP_HTML, keep) // genuine HTML tags
  t = t.replace(/</g, '&lt;').replace(/>/g, '&gt;')
  t = t.replace(new RegExp(SENT + '(\\d+)' + SENT, 'g'), (_, i) => stash[+i])
  return t
}

function walk(dir) {
  for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
    const p = path.join(dir, entry.name)
    if (entry.isDirectory()) walk(p)
    else if (entry.name.endsWith('.md')) {
      const orig = fs.readFileSync(p, 'utf8')
      const out = sanitize(orig)
      if (out !== orig) fs.writeFileSync(p, out)
    }
  }
}

const dirs = process.argv.slice(2)
if (dirs.length === 0) {
  console.error('usage: node sanitize-md.mjs <dir> [<dir> ...]')
  process.exit(1)
}
dirs.forEach(walk)
