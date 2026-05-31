import Cocoa

/// Detects the Dock icon bar's frame in AppKit coordinates (bottom-left origin).
/// Primary method: System Events via AppleScript (exact icon bar bounds).
/// Fallback: an empty rect — no Dock obstacle, so rain lands on the real screen
/// floor across the whole width (same behavior as snow).
enum DockDetector {

    /// `screenHeight` (the main screen's full height) is passed in by the caller
    /// rather than read from `NSScreen` here, so this can run off the main thread
    /// without touching AppKit. Used to flip Quartz (top-left) to AppKit (bottom-left).
    static func currentFrame(screenHeight: CGFloat) -> CGRect {
        appleScriptFrame(screenHeight: screenHeight) ?? .zero
    }

    // MARK: Private

    /// Compiled once and reused. Asks System Events for the Dock process's icon
    /// list bounds: `{x, y, width, height}` in Quartz (top-left origin) points.
    /// Only ever executed from `AppDelegate.dockQueue`, so the single shared
    /// instance is never used from two threads at once.
    private static let compiledScript: NSAppleScript? = NSAppleScript(source: """
        tell application "System Events"
            tell application process "Dock"
                set pos to position of list 1
                set sz to size of list 1
                return {item 1 of pos, item 2 of pos, item 1 of sz, item 2 of sz}
            end tell
        end tell
        """)

    /// Requires Accessibility permission (System Settings → Privacy → Accessibility).
    /// The first Apple Event sent also triggers an Automation permission dialog.
    private static func appleScriptFrame(screenHeight: CGFloat) -> CGRect? {
        guard AXIsProcessTrusted() else { return nil }
        guard let scr = compiledScript else { return nil }
        var error: NSDictionary?
        let result = scr.executeAndReturnError(&error)
        guard error == nil, result.numberOfItems >= 4 else { return nil }

        let x  = CGFloat(result.atIndex(1)?.int32Value ?? 0)
        let qy = CGFloat(result.atIndex(2)?.int32Value ?? 0)
        let w  = CGFloat(result.atIndex(3)?.int32Value ?? 0)
        let h  = CGFloat(result.atIndex(4)?.int32Value ?? 0)
        guard w > 0, h > 0 else { return nil }

        // Convert Quartz (top-left origin) → AppKit (bottom-left origin)
        return CGRect(x: x, y: screenHeight - qy - h, width: w, height: h)
    }
}
