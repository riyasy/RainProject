import Cocoa

// MARK: - Overlay window

/// Borderless, transparent, click-through window at .screenSaver level.
/// canBecomeKey returns false so it never steals keyboard focus from other apps.
class TransparentWindow: NSWindow {
    override var canBecomeKey: Bool { false }
}

// MARK: - Entry point

@main
class AppMain: NSObject {
    static func main() {
        let app = NSApplication.shared
        let delegate = AppDelegate()
        app.delegate = delegate
        app.run()
    }
}
