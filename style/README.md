# Portal artwork

`favicon.png` and `Background.jpg` are the original user-provided images. Keep them unchanged.

Run `./style/build-assets.ps1` from the project root after changing artwork. It creates a 64×64 PNG icon and 640×1024 JPEG background in `.build/web-assets/`, and regenerates the checked-in `btpad_pico/WebAssets.h` used by the firmware. Generated images total 97,115 bytes; the source images total 1,458,418 bytes. The firmware serves the binary images directly from flash at `/favicon.png` and `/background.jpg` without an external image host.

The script requires Windows PowerShell/PowerShell with System.Drawing. Ordinary firmware builds use the generated header and do not need image conversion dependencies.

For local layout review, run the desktop tests to export `.test-build/portal-preview.html`, run this asset script, then `node tests/preview.cjs` and visit `http://127.0.0.1:8765`. This is a mock preview with GP06 held and test configuration; it does not save settings or connect to the controller.
