# Configuration Toolkit

[![Stars](https://img.shields.io/github/stars/AldertLake/Config-Toolkit?style=for-the-badge&logo=github)](https://github.com/AldertLake/Config-Toolkit/stargazers)
[![Last Updated](https://img.shields.io/github/last-commit/AldertLake/Config-Toolkit?style=for-the-badge&label=last%20updated)](https://github.com/AldertLake/Config-Toolkit/commits)

An Unreal Engine Blueprint-focused plugin for reading, writing, and managing `.ini` config data directly from Blueprints.

It supports standard values, arrays, encrypted strings, soft object references, and soft class references while keeping asset loading safe and explicit.

## Start Here

- [Documentation Overview](https://aldertlake-docs.vercel.app/docs/config-toolkit)
- [Buy the Plugin (Support)](https://www.fab.com/listings/9186c8d0-f880-4277-84c4-a51a19220956)
- [Download the Plugin (Free)](https://github.com/AldertLake/Config-Toolkit/releases)

## Key Features

- Read and write `.ini` values from Blueprints.
- Save and load arrays.
- Store encrypted strings.
- Save and read Soft Object Reference and Soft Class Reference paths without forcing synchronous asset loading.
- Prevent unsafe hard object/class reads by design.
- Preserve normal `String` pins exactly as written.
- Check if keys exist.
- Delete config files.
- Remove config sections.
- Manually flush config changes when needed.
