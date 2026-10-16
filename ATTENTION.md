# CYBERPUNCH Export Safety Rules

This file records the export constraints required for this project.

- Manufacturer and plugin/subtype codes must be exactly four letters: one uppercase letter followed by three lowercase letters. No digits.
- Use identical codes in build definitions, Projucer configuration, `AppConfig.h`, Extra Definitions, and every generated export configuration.
- Before claiming a macOS/AU export, inspect `Builds/MacOSX/Info.plist` and confirm `AudioComponents` contains the exact configured values.

Selected codes: manufacturer `Mbfx`; plugin/subtype `Larz`. Both are reserved for this project.

Do not substitute other codes during export. Verify that the selected codes
are consistent in generated build definitions, Projucer configuration,
`AppConfig.h`, Extra Definitions, and any generated plugin identity metadata.

Why these rules exist: A previous export used an incorrect manufacturer ID. That broke the plugin’s identity across build and export settings, which caused macOS automation using AppleScript and osascript to fail. These checks prevent that failure from recurring.