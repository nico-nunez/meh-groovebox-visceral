# Project Context

## About This Project
- **Production-quality wavetable synthesizer** - Building a professional-grade synthesizer rivaling Serum, Vital, and PhasePlant in capability and sound quality
- **Vision: one-stop-shop for sound design** — a single instrument configurable via presets to cover techno, drums, pads, industrial, drones, leads, and beyond. Users load a preset and get a sound; the synthesis complexity is abstracted away.
- **Wavetable-first architecture** — oscillators are wavetable oscillators (like Vital/Serum), enabling FM synthesis, wavetable scanning, and a far wider sonic palette than polyblep alone
- **Standalone desktop application** — primary target is a standalone app that does not require a DAW. CoreAudio + CoreMIDI + Lua scripting + preset system = complete instrument.
- Focus: Production patterns, performance, SIMD-ready architecture, real-time audio constraints
- Learning path: Direct implementation of industry-standard techniques, not educational simplifications

## Technical Approach
- **Production-first, always** - Use patterns from professional synthesizers (Vital, Serum, PhasePlant, FM8, etc.), not beginner shortcuts
- **Assume solid programming background** - Focus on C++ audio/DSP specifics, performance optimization, real-time constraints
- **Explain the "why" with context** - Explain rationale with references to production synthesizers when relevant
- **Performance matters** - SIMD-ready architecture, cache-friendly data structures, real-time safe code
- **Functional/procedural style preferred** - SoA + pure functions in hot paths, minimal OOP overhead in audio processing
- **Library functions assume valid input** - `dsp/` and other internal libs only implement the golden path; null checks, bounds guards, and enabled flags belong at the call site, not inside the primitive

## Working Style
- **DO NOT update files unless explicitly requested** - this is a learning project and automatic fixes defeat the purpose
- Offer suggestions, explanations, and guidance instead of making changes
- When presenting options, explain trade-offs but lean toward industry best practices
- Exception: Documentation and reference materials can be created/updated when asked
- **"Plan" means a doc** - When asked to "make a plan" or "create a plan", write a planning document in `_docs_/` (or update the roadmap). Do NOT enter plan mode.

## Project Roadmap
The two authoritative docs for project direction are:
- `_docs_/architecture/groovebox-vision.md` — the authoritative doc for overall project direction and roadmap

All other docs in `_docs_/` are potentially outdated. Do not reference them for project direction or scope decisions unless explicitly asked to. Use only the roadmap docs to understand what's planned, what's next, and what's in scope.

## Documentation Philosophy
**Docs describe production-quality solutions, not the current state of the code.**

- If the current implementation falls short of what a production synthesizer requires, **say so explicitly** — mark it `ASPIRATIONAL`, `CODE FIX`, or `ARCHITECTURAL GAP` as appropriate, and describe what correct looks like.
- Never route around a known architectural deficiency in a doc just because fixing it seems out of scope. Workarounds hidden in docs become hidden debt. Call them out so an informed decision can be made.
- The goal of the documentation phase is that when implementation begins, the full picture is visible: what to build, what already exists, and what existing code needs to change. Omitting the third item defeats the purpose.
- Validating architecture against external references (Vital source, published DSP techniques) is a requirement, not optional. "Seems right" is not good enough.

## Documentation Style
Reference docs in `_docs_/` (note the underscores) should be:
- **Concise and to the point** - no excessive filler
- **Scannable** - clear sections, code examples, key takeaways
- **Practical** - focus on "aha moments" and common gotchas
- If it's too long or wordy, it won't get read!

### Documentation Requirements
1. **Table of Contents** - All doc files must include a table of contents with section links at the top
2. **README.md Updates** - When creating new docs, add them to the corresponding README.md file in the directory
3. **Keep Index Current** - Ensure all existing docs are listed in their directory's README.md
