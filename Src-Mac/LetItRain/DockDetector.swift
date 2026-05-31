import Cocoa

/// Detects the Dock icon bar's frame in AppKit coordinates (bottom-left origin).
/// Primary method: System Events via AppleScript (exact icon bar bounds).
/// Fallback: NSScreen.visibleFrame difference (reserved strip, full width).
enum DockDetector {

    static func currentFrame() -> CGRect {
        appleScriptFrame() ?? visibleFrameRect()
    }

    // MARK: Private

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
    private static func appleScriptFrame() -> CGRect? {
        guard AXIsProcessTrusted() else { return nil }
        guard let scr = compiledScript else { return nil }
        var error: NSDictionary?
        let result = scr.executeAndReturnError(&error)
        guard error == nil, result.numberOfItems >= 4 else { return nil }

        let x  = CGFloat(result.atIndex(1)?.int32Value ?? 0)
        let qy = CGFloat(result.atIndex(2)?.int32Value ?? 0)
        let w  = CGFloat(result.atIndex(3)?.int32Value ?? 0)
        let h  = CGFloat(result.atIndex(4)?.int32Value ?? 0)
        guard let screen = NSScreen.main, w > 0, h > 0 else { return nil }

        // Convert Quartz (top-left origin) → AppKit (bottom-left origin)
        return CGRect(x: x, y: screen.frame.height - qy - h, width: w, height: h)
    }

    /// Approximates the Dock reserved strip from the difference between
    /// NSScreen.frame and NSScreen.visibleFrame. Covers the full strip width/height
    /// rather than just the icon bar — used when Accessibility is not granted.
    private static func visibleFrameRect() -> CGRect {
        guard let screen = NSScreen.main else { return .zero }
        let sf = screen.frame, vf = screen.visibleFrame
        if sf.width > vf.width {
            let w = sf.width - vf.width
            return CGRect(x: vf.origin.x > 0 ? 0 : vf.maxX, y: 0, width: w, height: sf.height)
        } else if vf.origin.y > 0 {
            return CGRect(x: 0, y: 0, width: sf.width, height: vf.origin.y)
        }
        return .zero
    }
}
