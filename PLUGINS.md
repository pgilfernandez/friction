# Friction native plugins (API v1)

Friction loads native Qt plugins at startup. Installing a plugin does not
require rebuilding Friction: copy its compiled library into the user plugins
directory and restart the application.

The user directory is `Plugins` inside Friction's configuration directory.
Friction creates it on startup. Bundled plugins are also discovered in the
platform installation directories and in `plugins` beside a development
build. Directories are scanned recursively, so a plugin may keep resources and
private libraries in its own subdirectory.

> Native plugins execute with the same permissions as Friction. Only install
> binaries from sources you trust.

## API model

The public entry point is
[`frictionplugin.h`](src/plugins/api/frictionplugin.h). A plugin is a `QObject`
that implements `Friction::Plugins::Plugin` and declares
`FRICTION_PLUGIN_IID`. It provides:

- stable identity and version strings;
- the plugin API version (`ApiVersion`, currently 1);
- one or more action descriptions;
- `initialize`, `triggerAction`, and `shutdown` lifecycle methods.

An action chooses an Import, Export, Object, Effects, Scene, or Help menu. It
can require an active scene and can opt into the main toolbar. Friction owns
the resulting `QAction`, shortcut registration, enabled state, and menu
placement. The plugin receives a `Host`, from which it can obtain the main
window, document, and active scene and can finish a document action.

The API header is intentionally small. Plugins that only add UI or external
processing need only Qt and this header. Native format plugins that create or
inspect Friction objects also compile against Friction's core headers and link
to `frictioncore`; therefore they must match Friction's Qt version, compiler,
architecture, and native ABI. API v1 prevents loading an incompatible API
revision, but it does not promise binary compatibility with a different
Friction release.

## Minimal external plugin

[`src/plugins/examples/minimal`](src/plugins/examples/minimal) is a standalone
CMake project. Build it by pointing `FRICTION_PLUGIN_API_DIR` at
`src/plugins/api`:

```sh
cmake -S src/plugins/examples/minimal -B build/minimal-plugin \
  -DFRICTION_PLUGIN_API_DIR="$PWD/src/plugins/api"
cmake --build build/minimal-plugin
```

Copy the resulting module into the user `Plugins` directory and restart
Friction. The example adds an item under Help and does not link to Friction's
internal libraries.

For plugins built in this repository, use `add_friction_plugin(target ...)`.
It applies the common Qt, core/UI, Skia, FFmpeg, output, RPATH, and install
configuration. `BUILD_BUNDLED_PLUGINS=OFF` builds Friction without bundled
format modules.

## Migrated format plugins

The former feature branches now live in three independent modules:

| Plugin | Source | Action |
| --- | --- | --- |
| Lottie Export | `src/plugins/bundled/lottie-export` | Preview and export Lottie/dotLottie |
| Lottie Import | `src/plugins/bundled/lottie-import` | Import Lottie/dotLottie as editable objects |
| SVG Import | `src/plugins/bundled/svg-import` | Import animated SVG as editable objects |

Their conversion code remains local to each plugin. The Friction base only
contains the loader, host integration, shared-library build support, and a few
general read-only `ImageBox`/`TextBox` accessors required by exporters.

To migrate another existing feature:

1. Move the conversion and dialog sources into one plugin directory.
2. Replace its `MainWindow` menu method with a `Plugin` action and perform the
   operation in `triggerAction`.
3. Obtain `Document`, `Canvas`, and the parent widget from `Host`.
4. Keep format-specific sources and dependencies in the plugin target.
5. Build the module, copy it to `Plugins`, restart, and run the plugin load
   test before testing the conversion itself.

## Build and smoke test

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target friction friction_lottie_export \
  friction_lottie_import friction_svg_import
ctest --test-dir build -R friction_bundled_plugins_load --output-on-failure
```

The smoke test loads every bundled module through `QPluginLoader`, checks the
API version and unique id, then unloads it. Format behavior should additionally
be tested in Friction with representative `.svg`, `.json`, and `.lottie`
files.
