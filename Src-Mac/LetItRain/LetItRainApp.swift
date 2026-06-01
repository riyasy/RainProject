import Cocoa

// MARK: - Overlay window

/// Borderless, transparent, click-through window at .screenSaver level.
/// canBecomeKey returns false so it never steals keyboard focus from other apps.
class TransparentWindow: NSWindow {
    override var canBecomeKey: Bool { false }
}

// MARK: - Entry point

/// Manual `@main` entry point. The app has no storyboard/SwiftUI lifecycle, so
/// we bootstrap `NSApplication` by hand and install `AppDelegate`. The delegate
/// is held by `NSApplication` for the process lifetime.
@main
class AppMain: NSObject {
    static func main() {
        let app = NSApplication.shared
        let delegate = AppDelegate()
        app.delegate = delegate
        app.run()
    }
}
