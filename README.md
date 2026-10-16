# LARZ

LARZ is an eight-voice, MIDI-triggered sub-drop synthesizer for macOS. It is
available as VST3 and Audio Unit (AU) plug-ins.

## Release

- Version: 1.0.6
- Manufacturer: MabbieFX Audio
- AU type / manufacturer / subtype: `aumu` / `Mbfx` / `Larz`
- Bundle ID: `com.mabbiefx.larz`
- Minimum macOS version: 12.0
- JUCE: 9.0.2 (pinned submodule)
- Release binaries: universal arm64 and x86_64

The [user manual](LARZ_User_Manual_v1.0.6.pdf) describes the controls and use.

## Build from source

Clone with the pinned JUCE dependency:

```sh
git clone --recurse-submodules <repository-url>
```

Open `LARZ.jucer` in Projucer, save the project, then open
`Builds/MacOSX/LARZ.xcodeproj`. Build the `LARZ - VST3` and `LARZ - AU`
schemes in Release configuration. The Xcode project targets arm64 and x86_64;
choose the architectures supported by your toolchain.

The generated exporters and `JuceLibraryCode` are included. Projucer can
regenerate them from the `.jucer` project and the pinned JUCE submodule.

## Install

The GitHub release DMG contains `LARZ-1.0.6-Installer.pkg` and this manual.
The installer lets you choose Audio Unit, VST3, or both. The plug-ins are
installed in the standard system plug-in folders.

The current package and DMG are unsigned and not notarized because no
Developer ID signing identity is configured in the build environment. Sign
and notarize them with a Developer ID Installer certificate before public
distribution to avoid Gatekeeper warnings.

## Repository contents

- `Source/` — audio processor, editor, and voice implementation
- `Assets/` — embedded editor artwork
- `Tests/` — voice model tests
- `Builds/MacOSX/` — generated macOS Xcode exporter
- `Packaging/` — installer choice definition
- `JUCE/` — JUCE 9.0.2 submodule

LARZ's original source code is released under the MIT License; see
[LICENSE](LICENSE). JUCE and its bundled third-party dependencies are not
covered by this license. See [JUCE/LICENSE.md](JUCE/LICENSE.md) for JUCE's
licensing terms.
