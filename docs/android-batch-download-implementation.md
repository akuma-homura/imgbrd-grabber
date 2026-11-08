# Android graphical batch downloads

Work-in-progress implementation on branch `feature/android-batch-download`. This is an experimental feature, not a validated release.

## Implemented in this branch

- **Search UI:** tap Download to choose current search page or a bounded cross-page search (1–10,000 posts). A Tasks button opens progress, cancel and last-task retry.
- **Shared engine:** `BatchDownloadController` in `src/gui-qml/src/` uses the existing `BatchDownloader`, `DownloadQueryGroup` and `PackLoader`. The core now emits per-file save outcomes and supports a one-page limit.
- **Artist classification:** the existing filename formatter uses `%website%/%artist%/%md5%.%ext%`. The `Save → Artist` settings control missing and multiple artist values.
- **Scoped storage:** media is first downloaded to `QStandardPaths::AppDataLocation/grabber-batch`. On Android 10+ `MediaExporter.java` exports images to `Pictures/Grabber/<site>/<artist>/` using `MediaStore`; videos go to Movies/Grabber and miscellaneous files to Download/Grabber. On Android 9 or earlier the legacy Pictures directory is used.
- **Task recovery:** one last search is persisted to JSON. Before a new/retried search, complete staging files are re-exported to MediaStore and removed on success; the saved query is then rerun. It does **not** resume partial network transfers.

## Testing and device validation

1. Enable GitHub Actions for this fork, then run the existing **Build** workflow on the feature branch or draft PR.
2. Download the `Grabber.apk` workflow artifact. Install only on a test device initially.
3. On Android 10–16, configure Danbooru and Gelbooru sources. In the Search screen, search a known artist, tap Download, choose Current page and verify the Pictures/Grabber site/artist directory and original format.
4. Run a mixed-tag search with multiple artists, then choose Across search pages (limit 100). Confirm media appears under multiple artist directories.
5. Verify unknown artists, multiple artists, IDs repeated across sites, extensions (JPEG, PNG, GIF, WebP, AVIF), and video paths.
6. Repeat the query and verify no duplicate exported filenames. Interrupt network and retry from Tasks. Verify missing files are recorded as failures.
7. Test Cancel during in-flight requests; close the app and reopen, and test Resume last task.
8. Run `lib-tests` and formatting checks on a Qt-enabled build machine. **No passing CI run or device test was observed when this code was submitted.**

## Known limitations and follow-ups

- **Not yet implemented:** SAF custom-folder selection; file exports use fixed public media collections.
- **Not yet implemented:** Android foreground service, reliably downloading after process death or with the app terminated.
- **Not yet implemented:** a multi-task queued scheduler with arbitrary task history. Only one task is active at a time, and one last-search record is kept.
- **Important:** incomplete `.tmp` or `.part` downloads are intentionally excluded from staged-file recovery; the search must fetch them again. MediaStore export still requires device testing under different MD5 settings.
- **Global setting:** the Original checkbox updates Grabber's existing `Save/downloadoriginals` setting and therefore also affects subsequent single-image downloads.
- Existing bulk engine uses nested event loops during page loading and needs real-device responsiveness testing.
- No compiled APK is attached by this branch until the existing CI build runs and succeeds.
- Legacy Android storage permission and single-image save flow remain unchanged.

Do not merge to `master` or publish an APK until CI and on-device acceptance tests pass.
