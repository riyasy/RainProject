import Cocoa

enum ParticleMode: Int { case rain = 0, snow = 1 }

class AppDelegate: NSObject, NSApplicationDelegate {

    // MARK: - Overlay window

    var window: TransparentWindow!

    // MARK: - Settings
    // All accessed on the main thread (CADisplayLink fires on main).

    private var settingsIntensity: Int  = 200
    private var settingsDirection: Int  = 0
    private var settingsR: Float        = 0.6
    private var settingsG: Float        = 0.82
    private var settingsB: Float        = 1.0
    private var settingsMode: ParticleMode = .rain

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

    // MARK: - Menu bar

    private var statusItem: NSStatusItem?
    private var settingsPanel: NSPanel?

    // MARK: - App lifecycle

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

    @objc private func screenParametersChanged() {
        window.setFrame(NSScreen.main?.frame ?? .zero, display: true)
        setupAnimation()
    }

    // MARK: - Overlay window

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
            drops = (0..<settingsIntensity).map { _ in
                RainDrop(screenBounds: screenBounds, windX: wx, stagger: true)
            }
            snowSystem = nil
            startDockPolling()

        case .snow:
            snowRenderer.setup(in: cv, screenBounds: screenBounds)
            snowSystem = SnowSystem(screenBounds: screenBounds, count: settingsIntensity)
            drops = []
            // Snow never reads dockObstacle (it settles on the screen floor /
            // heightMap), so no AppleScript Dock polling in snow mode.
        }

        startDisplayLink()
    }

    /// Dock detection is only meaningful for rain (drops/splatters land on the
    /// Dock's top edge). The synchronous NSAppleScript query must run on the
    /// main thread (not thread-safe), so it's kept off the hot path in snow mode.
    private func startDockPolling() {
        dockObstacle = DockDetector.currentFrame()
        pollTimer = Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { [weak self] _ in
            guard let self else { return }
            let updated = DockDetector.currentFrame()
            if updated != self.dockObstacle { self.dockObstacle = updated }
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

    private func tickRain(dt: CGFloat) {
        let wx = windX(forDirection: settingsDirection)
        let target = settingsIntensity
        if drops.count != target {
            if drops.count < target {
                drops += (drops.count..<target).map { _ in RainDrop(screenBounds: screenBounds, windX: wx) }
            } else {
                drops.removeLast(drops.count - target)
            }
        }

        for i in drops.indices {
            if !drops[i].touchedGround {
                drops[i].vel.x = wx
                drops[i].updateDrop(dt: dt, dockObstacle: dockObstacle)
            } else {
                for j in drops[i].splatters.indices where drops[i].splatters[j].isAlive {
                    drops[i].splatters[j].update(dt: dt, screenBounds: screenBounds, dockObstacle: dockObstacle)
                }
                drops[i].splatterTime += Double(dt)
                if drops[i].splatterTime >= kSplatterDuration { drops[i].isDead = true }
            }
            if drops[i].isDead { drops[i].reset(windX: wx) }
        }

        rainRenderer.render(drops: drops, screenBounds: screenBounds,
                            color: (r: settingsR, g: settingsG, b: settingsB))
    }

    private func tickSnow(dt: CGFloat) {
        snowSystem?.update(dt: dt, time: lastTimestamp, targetCount: settingsIntensity)
        if let system = snowSystem {
            snowRenderer.render(system: system, screenBounds: screenBounds)
        }
    }

    // MARK: - Accessibility

    private func requestAccessibilityIfNeeded() {
        guard !AXIsProcessTrusted() else { return }
        let opts = [kAXTrustedCheckOptionPrompt.takeRetainedValue() as String: true]
        AXIsProcessTrustedWithOptions(opts as CFDictionary)
    }

    // MARK: - Menu bar

    private func setupMenuBar() {
        statusItem = NSStatusBar.system.statusItem(withLength: NSStatusItem.squareLength)
        statusItem?.button?.image = NSImage(systemSymbolName: "cloud.rain.fill",
                                            accessibilityDescription: "Let It Rain")
        statusItem?.button?.action = #selector(toggleSettings)
        statusItem?.button?.target = self
    }

    @objc private func toggleSettings() {
        if settingsPanel == nil { buildSettingsPanel() }
        guard let panel = settingsPanel else { return }
        if panel.isVisible {
            panel.orderOut(nil)
        } else {
            positionPanel(panel)
            panel.makeKeyAndOrderFront(nil)
            NSApp.activate(ignoringOtherApps: true)
        }
    }

    private func positionPanel(_ panel: NSPanel) {
        guard let btn = statusItem?.button,
              let btnWindow = btn.window else { return }
        let btnRect = btnWindow.convertToScreen(btn.frame)
        panel.setFrameOrigin(NSPoint(
            x: btnRect.midX - panel.frame.width / 2,
            y: btnRect.minY - panel.frame.height - 6
        ))
    }

    private func buildSettingsPanel() {
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

        let panel = NSPanel(contentRect: NSRect(x: 0, y: 0, width: 300, height: 250),
                            styleMask: [.titled, .closable, .nonactivatingPanel],
                            backing: .buffered, defer: false)
        panel.title = "Let It Rain"
        panel.contentViewController = vc
        panel.isFloatingPanel = true
        panel.level = NSWindow.Level(rawValue: Int(CGWindowLevelForKey(.maximumWindow)))
        panel.hidesOnDeactivate = false
        settingsPanel = panel
    }

    private func applyColor(_ c: NSColor) {
        let rgb = c.usingColorSpace(.deviceRGB) ?? c
        settingsR = Float(rgb.redComponent)
        settingsG = Float(rgb.greenComponent)
        settingsB = Float(rgb.blueComponent)
    }

    // MARK: - Persistence

    private func loadSettings() {
        let d = UserDefaults.standard
        if let v = d.object(forKey: Keys.intensity) as? Int   { settingsIntensity = v }
        if let v = d.object(forKey: Keys.direction) as? Int   { settingsDirection = v }
        if let v = d.object(forKey: Keys.colorR)    as? Float { settingsR = v }
        if let v = d.object(forKey: Keys.colorG)    as? Float { settingsG = v }
        if let v = d.object(forKey: Keys.colorB)    as? Float { settingsB = v }
        if let v = d.object(forKey: Keys.mode)      as? Int,
           let m = ParticleMode(rawValue: v)                   { settingsMode = m }
    }

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
