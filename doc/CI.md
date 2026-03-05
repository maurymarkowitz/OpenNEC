CI and Releases
===============

This project includes GitHub Actions workflows to build and test binaries and
to publish release artifacts. The key workflows are:

- `.github/workflows/ci.yml` — continuous integration: builds on Ubuntu,
  macOS, and Windows (MINGW), runs a smoke test, and uploads build artifacts.
- `.github/workflows/release.yml` — release pipeline: on a published GitHub
  Release the workflow builds and packages binaries for multiple targets and
  uploads zip artifacts to the Release.

Platform notes
--------------
- macOS arm64: the release workflow includes a macOS arm64 job that passes
  `-arch arm64` flags. This job runs successfully on an arm64 runner (native
  Apple Silicon); on x86 runners it may fail if cross-tooling or SDKs are
  unavailable. The job prints `uname -m` to help diagnose runner architecture.
- Windows: the workflows build using MSYS2/MINGW64 and produce `onec.exe`.
- Linux: x86_64 and aarch64 builds are produced; aarch64 uses a Docker
  container to build on `ubuntu:22.04`.

Triggering
---------
- CI runs on pushes and pull requests.
- Release builds run when a GitHub Release is published; you can also run the
  release workflow manually from the Actions UI (`workflow_dispatch`).

Artifacts
---------
Each release job zips the executable(s) plus `README.MD` and an example deck
and uploads the zip as a Release asset. Files are named `onec-<platform>-<arch>.zip`.

If you want changes to the workflows (extra targets, static/musl builds,
notarization/signing), tell me which targets and I will add them.
