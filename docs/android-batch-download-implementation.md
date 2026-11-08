# Android batch download implementation

Development branch: `feature/android-batch-download`. The goal is to expose the existing shared Qt batch downloader to the Android QML interface, with per-artist naming and scoped-storage support.

Implementation checklist:
- QML batch task interface with bounded result counts.
- C++ controller preserving the existing `DownloadQueryGroup` and `BatchDownloader` model.
- File export to the user-visible Pictures collection under Android scoped storage.
- Validate Danbooru and Gelbooru tags, retries, cancellation, and duplicate handling.
- Run Android CI and verify installed APK on a device before release.

Safety: do not use the legacy `WRITE_EXTERNAL_STORAGE` permission as the sole export method; do not modify `master` until CI and device validation pass.
