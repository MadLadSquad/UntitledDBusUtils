# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

UntitledDBusUtils is a standalone C++20 RAII/metaprogramming wrapper over the low-level `libdbus-1` C API. It has its own GitHub repo (`MadLadSquad/UntitledDBusUtils`) and is *also* vendored as a git submodule of UntitledImGuiFramework under `Framework/Modules/OS/ThirdParty/`. Treat it as an independent library: it does not depend on the framework, and changes here should make sense for standalone consumers.

Everything lives in the `UDBus` namespace. End-user API docs are on the [wiki](https://github.com/MadLadSquad/UntitledDBusUtils/wiki/) — consult it for usage examples rather than inferring API contracts from callers.

The header comment at the top of `DBusUtils.hpp` sets the tone: this is deliberately template-heavy code written to bend the C++ type system around dbus-1's dynamic type model. Expect that.

## Build

Only dependency is `dbus-1` (found via `pkg_check_modules`). No test suite exists — CI (`.github/workflows/CI.yaml`) only builds on Linux.

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=RELEASE
make -j"$(nproc)"
```

- `BUILD_VARIANT_STATIC` (CMake var) switches the library between `SHARED` (default) and `STATIC`. When consumed by the framework, this is driven by the framework's static/shared build mode.
- `UIMGUI_INSTALL` gates the `install()` rules (headers → `include/UntitledDBusUtils`, lib → `lib64/`, pkg-config → `lib/pkgconfig/`).
- The target unconditionally defines `UIMGUI_DBUS_SUBMODULE_ENABLED` for consumers.

## Architecture

The library splits cleanly into **RAII handle wrappers** and a **compile-time type-schema system** for (de)serialisation.

### RAII handle wrappers (thin, one class per dbus-1 object)
Each wraps a `DBus*` pointer, adds destructors, null-pointer guards, and `operator DBusFoo*()` conversions so instances pass straight into raw dbus-1 calls. Member functions mirror the C functions with the `dbus_<class>_` prefix stripped.
- `Error` (`Error.cpp`) — wraps `DBusError`.
- `Connection` (`Connection.cpp`) — bus/private connections, send / send_with_reply(_and_block).
- `Message` (`Message.cpp`, plus `MessageGet.cpp`) — wraps `DBusMessage*`; also the entry point for reading replies.
- `PendingCall` (`PendingCall.cpp`) — async reply handle.
- `Iterator` (`Iterator.cpp`) — thin wrapper over `DBusMessageIter` for users who want manual low-level control.

Naming conventions to preserve when adding methods: a `_raw` or `_1` suffix means "same operation but takes a raw dbus-1 type instead of our wrapper" (works around C++ overload rules). `_s` suffix denotes a `static` factory variant (e.g. `new_method_return_s`). Read `UDBUS_GET_MESSAGE(x)` / `getMessagePointer()` when you need a `DBusMessage**` out-param.

### Appending (serialisation) — `MessageBuilder` (`MessageAppend.cpp`)
Fluent `operator<<` / `.append()` API. Two-phase and **deferred**:
1. Streaming values and `MessageManipulators` enum tokens (`BeginStruct`/`EndStruct`, `BeginVariant`/`EndVariant`, `BeginArray`/`Next`/`EndArray`, `BeginDictEntry`/`EndDictEntry`) builds an in-memory tree of `AppendNode`s, each holding a deferred `event` lambda plus signature fragments. Nothing touches dbus-1 yet.
2. Streaming `EndMessage` walks the tree twice: `getSignature` composes the dbus type signature (wrapping structs in `()`, dict entries in `{}`, threading variant/array inner signatures), then `sendMessage` fires each node's `event` to actually append into the live `DBusMessageIter` stack held on the `Message`.

String handling is special: appended strings are copied into `MessageBuilder::tempStrings` and referenced **by index** (cast through `void*`) rather than by pointer, because tree construction invalidates pointers. Arrays of strings need a `char***` — see the cast magic in `append(const std::vector<T>&)`. Don't "simplify" these casts without understanding the pointer-lifetime reason documented inline.

### Getting (deserialisation) — `Message::handleMessage` + the `Type`/`Struct` tree (`DBusUtilsStructs.hpp`, `MessageGet.cpp`)
The user describes the expected reply shape as a compile-time linked list of references: `Type<A, B, C>` (nesting `Struct<...>`, `std::vector`, `std::map`, `Variant`, etc.). `handleMessage` walks an iterator stack in lockstep with this type tree; `routeType` (in `DBusUtils.hpp`) dispatches each field by category using `if constexpr` (struct → array → map → variant → basic). Field-count and type mismatches surface as `MessageGetResult` codes (`RESULT_MORE_FIELDS_THAN_REQUIRED`, `RESULT_INVALID_*`, etc.).
- `handleMethodCall` / `handleSignal` are guarded wrappers that only run the get if `is_method_call` / `is_signal` matches.
- `ignore()` (`IgnoreType`) skips a field; `bump()` (`BumpType`) is a hack to make single-field messages parse.
- `Variant` carries a user-supplied `parse` std::function; `ContainerVariantTemplate` (via `associateWithVariant`) lets a variant be re-instantiated per element inside arrays/maps.
- Ownership: `Type::destroy` / `destroyComplex` walk the tree freeing heap-allocated struct/array/map/variant elements. The `bShouldBeFreed` / `bIsOrigin` flags on `Struct` and the `bWasInArrayOrMap` / `bDestroyEverything` bookkeeping in `routeDestroy` decide what actually gets `delete`d — this is subtle; changing allocation in `allocateArrayElements` requires matching changes here.

### Type tag system (`DBusUtilsTags.hpp`)
`Tag<T>` is a tag-dispatch table mapping C++ types to dbus type constants (`Tag<T>::TypeString`). `MAKE_TAG` whitelists a type; `DISALLOW_TAG` emits a compile error for unsupported ones. Deliberately forbidden: `float` (use `double`), `bool` (use `udbus_bool_t` — dbus booleans are 4 bytes), and all `std::string`/wide-string types (dbus wants UTF-8 `char*`/`const char*`). To add a supported type, add a `MAKE_TAG`. `is_array_type` / `is_map_type` concepts detect container-shaped types structurally.

`udbus_bool_t` (`DBusUtilsTags.hpp` / `DBusUtils.cpp`) is a 4-byte `dbus_bool_t` wrapper with implicit `bool` conversion — use it everywhere a dbus boolean is expected.

### Metaprogramming helpers (`DBusUtilsMeta.hpp`, `DBusUtilsStructs.hpp`)
- `DECLARE_TYPE_AND_STRUCT[_ADDITIONAL]` macros generate the same function body twice — once taking `Type<...>` and once taking `Struct<...>` — because there's no clean way to share one body across both. When you edit one of these bodies you're editing behaviour for both.
- `is_specialisation_of<Base, T>`, `is_complete<T>` are the standard detection idioms used throughout the dispatch code.

## Conventions

- Every wrapper method is `noexcept`; the library returns error codes / uses null guards instead of throwing. Keep it that way.
- When mirroring a new dbus-1 function, drop the `dbus_<class>_` prefix and add the `_raw`/`_1`/`_s` suffix per the rules above.
- After changing a header, remember downstream consumers (the framework) include these directly — keep the public surface in the four installed headers (`DBusUtils.hpp`, `DBusUtilsMeta.hpp`, `DBusUtilsStructs.hpp`, `DBusUtilsTags.hpp`) coherent.

## graphify

This project has a knowledge graph at graphify-out/ with god nodes, community structure, and cross-file relationships.

Rules:
- For codebase questions, first run `graphify query "<question>"` when graphify-out/graph.json exists. Use `graphify path "<A>" "<B>"` for relationships and `graphify explain "<concept>"` for focused concepts. These return a scoped subgraph, usually much smaller than GRAPH_REPORT.md or raw grep output.
- If graphify-out/wiki/index.md exists, use it for broad navigation instead of raw source browsing.
- Read graphify-out/GRAPH_REPORT.md only for broad architecture review or when query/path/explain do not surface enough context.
- After modifying code, run `graphify update .` to keep the graph current (AST-only, no API cost).
