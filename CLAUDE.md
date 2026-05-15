# Project Context

## About This Project
- **Production-quality groovebox** - Building a standalone groovebox for sequencing, arranging, performing, and sound design, with an embedded production-grade synth engine as one core runtime domain.
- **Vision: one-stop-shop for electronic music creation** — tracks, patterns, mixer state, synth programs, modulation, effects, MIDI, Lua scripting, and document-authored sessions work together as one instrument. Users can author or load a musical scene and get coherent groovebox behavior, not isolated engine features.
- **Synth engine as a subsystem** — the wavetable synth remains professional-grade and wavetable-first, but architectural decisions should serve the groovebox product: multi-track sequencing, atomic cross-domain state, mixer/synth/sequencer coordination, live control, and standalone performance.
- **Standalone desktop application** — primary target is a standalone app that does not require a DAW. CoreAudio + CoreMIDI + Lua scripting + preset/session system = complete instrument.
- Focus: Production patterns, performance, SIMD-ready architecture, real-time audio constraints
- Learning path: Direct implementation of industry-standard techniques, not educational simplifications

## Technical Approach
- **Production-first, always** - Use patterns from professional grooveboxes, sequencers, DAWs, and synthesizers, not beginner shortcuts. Prioritize correct cross-domain state management, realtime-safe publication, deterministic sequencing, and robust session/preset handling.
- **No quick-fix/myopic solutions** - Do not solve only the immediate symptom if the change creates hidden architectural debt. Prefer the production pattern first, including correct ownership, lifetime, allocation strategy, concurrency model, and real-time safety.
- **Assume solid programming background** - Focus on C/C++ audio app architecture, DSP, sequencing, state publication, performance optimization, and real-time constraints.
- **Explain the "why" with context** - Explain rationale with references to production grooveboxes, DAWs, sequencers, and synth engines when relevant.
- **Performance matters** - SIMD-ready DSP, cache-friendly data structures, deterministic scheduling, bounded work per callback, and real-time safe code.
- **Functional/procedural style preferred** - SoA + pure functions in hot paths, explicit state flow, minimal OOP overhead in audio, sequencing, and publication paths.
- **Prefer C-style explicit state flow** - For substantial objects/state, favor caller-owned storage, in-place initialization, and out-parameters over returning objects by value or hiding allocation in factories. Small scalar returns and small result structs are fine. Prefer APIs like `initThing(Thing* thing, ...)` or `buildThing(..., Thing* out)` when ownership, lifetime, or allocation matter.
- **Library functions assume valid input** - `dsp/` and other internal libs only implement the golden path; null checks, bounds guards, and enabled flags belong at the call site, not inside the primitive

## Working Style
- **DO NOT update files unless explicitly requested** - this is a learning project and automatic fixes defeat the purpose
- Offer suggestions, explanations, and guidance instead of making changes
- When presenting options, explain trade-offs but lean toward industry best practices
- All solutions, plans, and documentation must target the durable production pattern, not the minimum work needed to move forward. Re-examine surrounding usage and include the correct ownership, caller responsibilities, lifetime, allocation, concurrency, and real-time constraints.
- Exception: Documentation and reference materials can be created/updated when asked
- **"Plan" means a doc** - When asked to "make a plan" or "create a plan", write a planning document in `_docs_/` (or update the roadmap). Do NOT enter plan mode.

## Project Roadmap
The two authoritative docs for project direction are:
- `_docs_/architecture/groovebox-vision.md` — the authoritative doc for overall project direction and roadmap

All other docs in `_docs_/` are potentially outdated. Do not reference them for project direction or scope decisions unless explicitly asked to. Use only the roadmap docs to understand what's planned, what's next, and what's in scope.

## Documentation Philosophy
**Docs describe production-quality solutions, not the current state of the code.**

- If the current implementation falls short of what a production groovebox or its synth engine requires, **say so explicitly** — mark it `ASPIRATIONAL`, `CODE FIX`, or `ARCHITECTURAL GAP` as appropriate, and describe what correct looks like.
- Never route around a known architectural deficiency in a doc just because fixing it seems out of scope. Workarounds hidden in docs become hidden debt. Call them out so an informed decision can be made.
- The goal of the documentation phase is that when implementation begins, the full picture is visible: what to build, what already exists, and what existing code needs to change. Omitting the third item defeats the purpose.
- Validating architecture against external references (production grooveboxes/DAWs/sequencers, Vital source, and published DSP techniques) is a requirement, not optional. "Seems right" is not good enough.

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
