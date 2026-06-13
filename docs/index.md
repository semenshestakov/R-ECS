---
layout: home

hero:
  name: R-ECS
  text: Archetype-based ECS for modern C++
  tagline: Cache-friendly chunked storage · DAG-scheduled systems · type-safe events & commands
  actions:
    - theme: brand
      text: English docs
      link: /en/
    - theme: alt
      text: Русская документация
      link: /ru/

features:
  - title: ⚡ Archetype storage
    details: Entities are packed into chunks grouped by their component set, giving cache-friendly iteration with swap-remove dense archetypes.
  - title: 🧩 DAG scheduling
    details: Systems are ordered by their declared dependencies through a directed acyclic graph (ECS_DEPENDENT_SYSTEMS).
  - title: 📨 Events & commands
    details: Type-safe event queues (ECS_EVENT) and deferred structural commands keep mutation predictable inside the frame loop.
---
