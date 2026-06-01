import Cocoa

/// Which particle subsystem is active. The raw values are persisted in
/// `UserDefaults`, so don't renumber them.
enum ParticleMode: Int { case rain = 0, snow = 1 }

/// The app's controller and the hub for everything that isn't rendering.
///
/// Owns the transparent overlay window, the menu-bar status item and settings
/// panel, the persisted settings, the per-frame `CADisplayLink`, the physics
/// state (`drops` / `snowSystem`), and the two Metal renderers. Exactly one
/// renderer + subsystem is live at a time, selected by `settingsMode`.
///
/// Everything here runs on the main thread (the display link fires on main),
/// with the sole exception of the Dock query dispatched to `dockQueue`.
class AppDelegate: NSObject, NSApplicationDelegate {

    // MARK: - Overlay window

    var window: TransparentWindow!

    // MARK: - Settings
    // All accessed on the main thread (CADisplayLink fires on main).

    // Intensity is a base unit (5–50, the Windows build's "MaxParticles"); each
    // mode scales it by its own multiplier (see rainDropCount / snowFlakeCount).
    private var settingsIntensity: Int  = 25
    private var settingsDirection: Int  = 0
    private var settingsR: Float        = 0.6
    private var settingsG: Float        = 0.82
    private var settingsB: Float        = 1.0
    private var settingsMode: ParticleMode = .rain

    /// Active particle counts for the current intensity. Rain and snow use
    /// different multipliers (matching the Windows build) so the two platforms
    /// render the same density for a given intensity setting.
    private var rainDropCount: Int  { settingsIntensity * kRainDropMultiplier }
    private var snowFlakeCount: Int { settingsIntensity * kSnowFlakeMultiplier }

    /// `UserDefaults` keys for the persisted settings.
    private enum Keys {
        static let intensity = "intensity"
        static let direction = "direction"
        static let colorR    = "colorR"
        static let colorG    = "colorG"
        static let colorB    = "colorB"
        static let mode      = "mode"
    }

    // MARK: - Physics state  (main thread only)

    private var drops: [RainDrop] = []
    private var snowSystem: SnowSystem?
    private var screenBounds: CGRect = .zero
    private var dockObstacle: CGRect = .zero

    // MARK: - Subsystems

    private let rainRenderer = RainRenderer()
    private let snowRenderer = SnowRenderer()

    // MARK: - Timing

    private var displayLink: CADisplayLink?
    private var lastTimestamp: CFTimeInterval = -1
    private var pollTimer: Timer?

    /// Serial queue for the Dock AppleScript query. NSAppleScript isn't safe to
    /// use from multiple threads at once, but it's fine on one dedicated thread —
    /// so the (slow, synchronous) query runs here instead of stalling the main
    /// thread, and `DockDetector` is only ever touched from this queue.
    private let dockQueue = DispatchQueue(label: "com.letxt.SnowMan.LetItRain.dock",
                                          qos: .utility)

    // MARK: - Menu bar

    private var statusItem: NSStatusItem?
    private var settingsPopover: NSPopover?

    // MARK: - App lifecycle

    /// Build everything once the app finishes launching: load saved settings,
    /// show the menu-bar item and overlay window, ask for Accessibility (for Dock
    /// detection), start the animation, and rebuild on display reconfiguration.
    func applicationDidFinishLaunching(_ notification: Notification) {
        loadSettings()
        setupMenuBar()
        setupOverlayWindow()
        requestAccessibilityIfNeeded()
        setupAnimation()

        NotificationCenter.default.addObserver(
            self, selector: #selector(screenParametersChanged),
            name: NSApplication.didChangeScreenParametersNotification, object: nil
        )
    }

    /// Resolution / display-arrangement changed: resize the overlay to the new
    /// main-screen frame and rebuild the animation against the new geometry.
    @objc private func screenParametersChanged() {
        window.setFrame(NSScreen.main?.frame ?? .zero, display: true)
        setupAnimation()
    }

    // MARK: - Overlay window

    /// Create the full-screen overlay: borderless, transparent, click-through,
    /// at the screen-saver window level so it floats above normal windows.
    private func setupOverlayWindow() {
        let screenFrame = NSScreen.main?.frame ?? .zero
        window = TransparentWindow(contentRect: screenFrame,
                                   styleMask: [.borderless],
                                   backing: .buffered, defer: false)
        window.level = .screenSaver
        window.isOpaque = false
        window.backgroundColor = .clear
        window.ignoresMouseEvents = true
        window.orderFrontRegardless()
    }

    // MARK: - Animation

    /// (Re)build the active subsystem from scratch. Idempotent: tears down both
    /// renderers and the timers, captures current screen geometry, then sets up
    /// only the renderer + particle state for `settingsMode` and (re)starts the
    /// display link. Called on launch, on mode switch, and on display changes.
    private func setupAnimation() {
        displayLink?.invalidate()
        displayLink = nil
        pollTimer?.invalidate()
        pollTimer = nil

        screenBounds = NSScreen.main?.frame ?? .zero
        dockObstacle = .zero

        let cv = window.contentView!
        cv.wantsLayer = true

        // Teardown both renderers; set up only the active one.
        rainRenderer.teardown()
        snowRenderer.teardown()

        switch settingsMode {
        case .rain:
            rainRenderer.setup(in: cv, screenBounds: screenBounds)
            let wx = windX(forDirection: settingsDirection)
            drops = (0..<rainDropCount).map { _ in
                RainDrop(screenBounds: screenBounds, windX: wx, stagger: true)
            }
            snowSystem = nil
            startDockPolling()

        case .snow:
            snowRenderer.setup(in: cv, screenBounds: screenBounds)
            snowSystem = SnowSystem(screenBounds: screenBounds, count: snowFlakeCount)
            drops = []
            // Snow never reads dockObstacle (it settles on the screen floor /
            // heightMap), so no AppleScript Dock polling in snow mode.
        }

        startDisplayLink()
    }

    /// Dock detection is only meaningful for rain (drops/splatters land on the
    /// Dock's top edge), so it's kept off entirely in snow mode. The timer fires
    /// on the main run loop but only schedules the work — the synchronous query
    /// runs on `dockQueue`.
    private func startDockPolling() {
        refreshDockObstacle()
        pollTimer = Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { [weak self] _ in
            self?.refreshDockObstacle()
        }
    }

    /// Run the Dock query off the main thread, then apply the result back on main
    /// (so `dockObstacle` stays single-threaded — written and read only on main).
    private func refreshDockObstacle() {
        let screenHeight = screenBounds.height   // read NSScreen-derived value on main
        dockQueue.async { [weak self] in
            let updated = DockDetector.currentFrame(screenHeight: screenHeight)
            DispatchQueue.main.async {
                guard let self else { return }
                if updated != self.dockObstacle { self.dockObstacle = updated }
            }
        }
    }

    private func startDisplayLink() {
        lastTimestamp = -1
        // CADisplayLink retains its target, so this forms a retain cycle with
        // AppDelegate. That's intentional and harmless: AppDelegate lives for the
        // whole app, and setupAnimation() invalidates the old link (breaking the
        // cycle) before creating a new one.
        let link = window.displayLink(target: self, selector: #selector(tick(_:)))
        link.add(to: .main, forMode: .common)
        displayLink = link
    }

    // MARK: - Per-frame tick

    /// Display-link callback: compute the frame's delta-time and advance the
    /// active subsystem. `dt` is clamped to 50 ms so a stall (e.g. app paused)
    /// can't teleport particles across the screen in one step; the first frame
    /// uses the link's nominal duration since there's no previous timestamp.
    @objc private func tick(_ link: CADisplayLink) {
        let dt: CGFloat = lastTimestamp < 0 ? CGFloat(link.duration)
            : CGFloat(min(link.targetTimestamp - lastTimestamp, 0.05))
        lastTimestamp = link.targetTimestamp

        switch settingsMode {
        case .rain:
            tickRain(dt: dt)
        case .snow:
            tickSnow(dt: dt)
        }
    }

    /// Advance and draw one rain frame: reconcile the drop count to the intensity
    /// setting, then for each drop either fall (applying the live wind) or, once
    /// it has landed, animate its splatter burst until the burst expires and the
    /// drop is recycled. Finally hand the whole list to the renderer.
    private func tickRain(dt: CGFloat) {
        let wx = windX(forDirection: settingsDirection)
        let target = rainDropCount
        // Grow or shrink the pool to match the current intensity slider.
        if drops.count != target {
            if drops.count < target {
                drops += (drops.count..<target).map { _ in RainDrop(screenBounds: screenBounds, windX: wx) }
            } else {
                drops.removeLast(drops.count - target)
            }
        }

        for i in drops.indices {
            if !drops[i].touchedGround {
                drops[i].vel.x = wx                       // direction slider can change mid-fall
                drops[i].updateDrop(dt: dt, dockObstacle: dockObstacle)
            } else {
                // Landed: tick the live splatter droplets and age the burst.
                for j in drops[i].splatters.indices where drops[i].splatters[j].isAlive {
                    drops[i].splatters[j].update(dt: dt, screenBounds: screenBounds, dockObstacle: dockObstacle)
                }
                drops[i].splatterTime += Double(dt)
                if drops[i].splatterTime >= kSplatterDuration { drops[i].isDead = true }
            }
            if drops[i].isDead { drops[i].reset(windX: wx) }   // recycle in place
        }

        rainRenderer.render(drops: drops, screenBounds: screenBounds,
                            color: (r: settingsR, g: settingsG, b: settingsB))
    }

    /// Advance and draw one snow frame. `SnowSystem` owns count reconciliation,
    /// motion, settling, and the pile, so this just steps it and renders.
    private func tickSnow(dt: CGFloat) {
        snowSystem?.update(dt: dt, time: lastTimestamp, targetCount: snowFlakeCount)
        if let system = snowSystem {
            snowRenderer.render(system: system, screenBounds: screenBounds)
        }
    }

    // MARK: - Accessibility

    /// Prompt for Accessibility on first launch if not already trusted. The
    /// permission lets `DockDetector` read the Dock's exact icon-bar bounds;
    /// without it rain just lands on the screen floor everywhere.
    private func requestAccessibilityIfNeeded() {
        guard !AXIsProcessTrusted() else { return }
        let opts = [kAXTrustedCheckOptionPrompt.takeRetainedValue() as String: true]
        AXIsProcessTrustedWithOptions(opts as CFDictionary)
    }

    // MARK: - Menu bar

    /// Install the menu-bar status item whose button toggles the settings panel.
    private func setupMenuBar() {
        statusItem = NSStatusBar.system.statusItem(withLength: NSStatusItem.squareLength)
        statusItem?.button?.image = NSImage(systemSymbolName: "umbrella.fill",
                                            accessibilityDescription: "Let It Rain")
        statusItem?.button?.action = #selector(toggleSettings)
        statusItem?.button?.target = self
    }

    /// Show or hide the settings popover (built lazily on first use), anchored to
    /// the status-item button so it gets the native menu-bar window look.
    @objc private func toggleSettings() {
        if settingsPopover == nil { buildSettingsPopover() }
        guard let popover = settingsPopover, let btn = statusItem?.button else { return }
        if popover.isShown {
            popover.performClose(nil)
        } else {
            NSApp.activate(ignoringOtherApps: true)
            popover.show(relativeTo: btn.bounds, of: btn, preferredEdge: .maxY)
            // The rain overlay lives at `.screenSaver` level and covers the whole
            // screen; lift the popover window above it so rain isn't painted over
            // the settings UI.
            if let win = popover.contentViewController?.view.window {
                win.level = NSWindow.Level(rawValue: Int(CGWindowLevelForKey(.maximumWindow)))
            }
        }
    }

    /// Lazily build the settings popover and wire its `on*` callbacks back to this
    /// delegate: each writes the corresponding setting, persists it, and (for a mode
    /// change) rebuilds the animation. `.transient` makes it dismiss on outside click.
    private func buildSettingsPopover() {
        let vc = SettingsViewController()
        vc.intensity = settingsIntensity
        vc.direction = settingsDirection
        vc.mode      = settingsMode
        vc.dropColor = NSColor(calibratedRed: CGFloat(settingsR),
                               green: CGFloat(settingsG),
                               blue: CGFloat(settingsB), alpha: 1.0)
        vc.onIntensity = { [weak self] v in self?.settingsIntensity = v; self?.saveSettings() }
        vc.onDirection = { [weak self] v in self?.settingsDirection = v; self?.saveSettings() }
        vc.onColor     = { [weak self] c in self?.applyColor(c); self?.saveSettings() }
        vc.onMode      = { [weak self] m in
            self?.settingsMode = m
            self?.saveSettings()
            self?.setupAnimation()
        }
        vc.onQuit = { NSApp.terminate(nil) }

        let popover = NSPopover()
        popover.contentViewController = vc
        popover.contentSize = NSSize(width: 300, height: 318)
        popover.behavior = .transient
        popover.animates = true
        settingsPopover = popover
    }

    /// Store a chosen drop color as plain device-RGB floats for the renderer.
    private func applyColor(_ c: NSColor) {
        let rgb = c.usingColorSpace(.deviceRGB) ?? c   // normalize so .redComponent etc. are valid
        settingsR = Float(rgb.redComponent)
        settingsG = Float(rgb.greenComponent)
        settingsB = Float(rgb.blueComponent)
    }

    // MARK: - Persistence

    /// Load persisted settings, leaving each at its default if absent.
    private func loadSettings() {
        let d = UserDefaults.standard
        // Intensity is now a 5–50 base unit. Older builds stored the raw particle
        // count (20–500, i.e. the old slider position ×10); fold those back to a
        // slider position so existing users keep roughly the same intensity.
        if let v = d.object(forKey: Keys.intensity) as? Int {
            settingsIntensity = min(max(v > 50 ? v / 10 : v, 5), 50)
        }
        if let v = d.object(forKey: Keys.direction) as? Int   { settingsDirection = v }
        if let v = d.object(forKey: Keys.colorR)    as? Float { settingsR = v }
        if let v = d.object(forKey: Keys.colorG)    as? Float { settingsG = v }
        if let v = d.object(forKey: Keys.colorB)    as? Float { settingsB = v }
        if let v = d.object(forKey: Keys.mode)      as? Int,
           let m = ParticleMode(rawValue: v)                   { settingsMode = m }
    }

    /// Persist all current settings (called after any settings-panel change).
    private func saveSettings() {
        let d = UserDefaults.standard
        d.set(settingsIntensity,      forKey: Keys.intensity)
        d.set(settingsDirection,      forKey: Keys.direction)
        d.set(settingsR,              forKey: Keys.colorR)
        d.set(settingsG,              forKey: Keys.colorG)
        d.set(settingsB,              forKey: Keys.colorB)
        d.set(settingsMode.rawValue,  forKey: Keys.mode)
    }
}
