# PostWoman — a tiny raylib-based JSON inspector

This is a small C application using raylib for UI and libcurl for HTTP GET requests. It provides a simple text input for a URL, a Send button, and an area that displays the (pretty-printed) JSON response.

Requirements
- raylib installed (via homebrew: `brew install raylib` or your platform package)
- libcurl development headers (usually already available on macOS)
- gcc/clang supporting C11

Build
1. Open a terminal and cd into the project directory:

```sh
cd "/Users/pituser/Documents/Raylib mini projects/PostWoman"
```
2. Run `make`:

```sh
make
```

If your raylib is installed with pkg-config, you may prefer using pkg-config flags instead of the Makefile defaults.

Run

```sh
./PostWoman
```

Usage
- Click the URL input area and type/paste a URL (including `http://` or `https://`).
- Click "Send" (or press Enter) to fetch. The app will fetch in a background thread.
- The JSON response (raw or pretty-printed) will appear in the display area. Use the mouse wheel to scroll if the response is long.

Notes & Limitations
- The JSON pretty-printer is a simple, tolerant implementation — it handles common JSON but is not a full parser. If JSON is malformed it will still show the raw response.
- This minimal project uses a simple UI implemented with raylib primitives; it is intentionally small and dependency-free apart from raylib and libcurl.

License
MIT-style — feel free to adapt.
