import { defineConfig } from 'vitepress'
import fs from 'node:fs'
import { fileURLToPath, URL } from 'node:url'

const docsRoot = fileURLToPath(new URL('..', import.meta.url))

function apiItems(lang: string) {
  const dir = `${docsRoot}/${lang}/api`
  let files: string[] = []
  try {
    files = fs.readdirSync(dir).filter((f) => f.endsWith('.md')).sort()
  } catch {
    return []
  }
  return files.map((f) => {
    const name = f.replace(/\.md$/, '')
    return { text: name.replace(/-/g, '::'), link: `/${lang}/api/${name}` }
  })
}

function firstApiLink(lang: string) {
  const items = apiItems(lang)
  return items.length ? items[0].link : `/${lang}/`
}

function guideItems(lang: string, labels: string[]) {
  const slugs = [
    'getting-started',
    'components-and-prefabs',
    'entities-and-views',
    'systems-and-scheduling',
    'events',
    'commands',
    'recipes-and-cooking',
  ]
  return slugs.map((slug, i) => ({ text: labels[i], link: `/${lang}/guides/${slug}` }))
}

const enGuides = [
  'Getting started',
  'Components & prefabs',
  'Entities & views',
  'Systems & scheduling',
  'Events',
  'Commands',
  'Recipes & cooking',
]

const ruGuides = [
  'Начало работы',
  'Компоненты и префабы',
  'Сущности и представления',
  'Системы и планирование',
  'События',
  'Команды',
  'Рецепты и приготовление',
]

export default defineConfig({
  title: 'R-ECS',
  description: 'Archetype-based Entity-Component-System framework for modern C++ (C++20)',
  base: process.env.DOCS_BASE || '/',
  cleanUrls: true,
  ignoreDeadLinks: true,
  srcExclude: ['**/.build/**', '**/node_modules/**'],
  rewrites: {
    'en/README.md': 'en/index.md',
    'ru/README.md': 'ru/index.md',
  },
  themeConfig: {
    search: { provider: 'local' },
    socialLinks: [{ icon: 'github', link: 'https://github.com/semenshestakov/R-ECS' }],
    nav: [
      {
        text: 'Guides',
        items: [
          { text: '🇬🇧 English', link: '/en/guides/getting-started' },
          { text: '🇷🇺 Русский', link: '/ru/guides/getting-started' },
        ],
      },
      {
        text: 'API',
        items: [
          { text: '🇬🇧 English', link: firstApiLink('en') },
          { text: '🇷🇺 Русский', link: firstApiLink('ru') },
        ],
      },
      {
        text: '🌐 Language',
        items: [
          { text: '🇬🇧 English', link: '/en/' },
          { text: '🇷🇺 Русский', link: '/ru/' },
        ],
      },
    ],
    sidebar: {
      '/en/': [
        { text: 'Guides', items: guideItems('en', enGuides) },
        { text: 'API Reference', collapsed: true, items: apiItems('en') },
      ],
      '/ru/': [
        { text: 'Руководства', items: guideItems('ru', ruGuides) },
        { text: 'Справочник API', collapsed: true, items: apiItems('ru') },
      ],
    },
  },
})
