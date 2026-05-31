import Cocoa

/// The programmatic settings panel (mode toggle, intensity, direction, drop
/// color, Quit). It holds no app state: the initial values are pushed in before
/// presentation, and every change is reported through the `on*` callbacks, which
/// `AppDelegate` applies and persists. Direction and color rows live in
/// `rainOnlyViews` and are hidden in snow mode.
final class SettingsViewController: NSViewController {

    // Change callbacks, wired up by AppDelegate.
    var onIntensity: ((Int)          -> Void)?
    var onDirection: ((Int)          -> Void)?
    var onColor:     ((NSColor)      -> Void)?
    var onMode:      ((ParticleMode) -> Void)?
    var onQuit:      (() -> Void)?

    // Seed values, set by AppDelegate before the panel is shown.
    var intensity: Int          = 200
    var direction: Int          = 0
    var mode: ParticleMode      = .rain
    var dropColor: NSColor      = NSColor(calibratedRed: 0.6, green: 0.82, blue: 1.0, alpha: 1.0)

    private var intensitySlider = NSSlider()
    private var intensityLabel  = NSTextField(labelWithString: "")
    private var directionSlider = NSSlider()
    private var directionLabel  = NSTextField(labelWithString: "")
    private var colorWell       = NSColorWell()
    private var modeControl     = NSSegmentedControl()

    // Views that are only relevant in rain mode
    private var rainOnlyViews: [NSView] = []

    // MARK: - View lifecycle

    override func loadView() {
        view = NSView(frame: NSRect(x: 0, y: 0, width: 300, height: 250))
        buildUI()
    }

    // MARK: - UI construction

    /// Lay out every control by hand (no Auto Layout): a top-down cursor `top`
    /// is decremented as each row is added. The intensity slider's 2–50 range maps
    /// to 20–500 particles (×10); direction is an integer −5…+5 with tick stops.
    private func buildUI() {
        let pad: CGFloat = 18
        let w   = view.bounds.width - pad * 2
        var top = view.bounds.height - pad   // 232

        // ── Mode toggle ────────────────────────────────────────────────────
        modeControl = NSSegmentedControl(labels: ["Rain", "Snow"],
                                         trackingMode: .selectOne,
                                         target: self,
                                         action: #selector(modeChanged(_:)))
        modeControl.selectedSegment = mode == .rain ? 0 : 1
        modeControl.frame = NSRect(x: pad, y: top - 22, width: w, height: 22)
        view.addSubview(modeControl)
        top -= 22 + 16   // 194

        func headerLabel(_ text: String) -> NSTextField {
            let f = NSTextField(labelWithString: text)
            f.font = .boldSystemFont(ofSize: 11)
            f.textColor = .secondaryLabelColor
            return f
        }

        func valueLabel(_ text: String) -> NSTextField {
            let f = NSTextField(labelWithString: text)
            f.font = .monospacedDigitSystemFont(ofSize: 11, weight: .regular)
            f.alignment = .right
            return f
        }

        func addSliderRow(title: String, slider: NSSlider, valLabel: NSTextField,
                          min: Double, max: Double, value: Double,
                          action: Selector, intSteps: Bool = false) -> [NSView] {
            top -= 16
            let lbl = headerLabel(title)
            lbl.frame = NSRect(x: pad, y: top, width: 120, height: 14)
            view.addSubview(lbl)

            valLabel.frame = NSRect(x: view.bounds.width - pad - 50, y: top, width: 50, height: 14)
            view.addSubview(valLabel)

            top -= 22
            slider.minValue = min; slider.maxValue = max
            slider.doubleValue = value; slider.isContinuous = true
            if intSteps {
                slider.numberOfTickMarks = Int(max - min) + 1
                slider.allowsTickMarkValuesOnly = true
            }
            slider.target = self; slider.action = action
            slider.frame = NSRect(x: pad, y: top, width: w, height: 18)
            view.addSubview(slider)
            top -= 14
            return [lbl, valLabel, slider]
        }

        // ── Intensity (always visible) ─────────────────────────────────────
        intensityLabel.stringValue = "\(intensity)"
        _ = addSliderRow(title: "Intensity", slider: intensitySlider, valLabel: intensityLabel,
                         min: 2, max: 50, value: Double(intensity / 10),
                         action: #selector(intensityChanged))

        // ── Direction (rain only) ──────────────────────────────────────────
        directionLabel.stringValue = formatDirection(direction)
        let dirViews = addSliderRow(title: "Direction", slider: directionSlider,
                                    valLabel: directionLabel,
                                    min: -5, max: 5, value: Double(direction),
                                    action: #selector(directionChanged), intSteps: true)
        rainOnlyViews += dirViews

        // ── Drop Color (rain only) ─────────────────────────────────────────
        top -= 16
        let clrLbl = headerLabel("Drop Color")
        clrLbl.frame = NSRect(x: pad, y: top, width: 100, height: 14)
        view.addSubview(clrLbl)
        rainOnlyViews.append(clrLbl)

        top -= 28
        colorWell.color = dropColor
        colorWell.target = self; colorWell.action = #selector(colorChanged)
        colorWell.frame = NSRect(x: pad, y: top, width: 44, height: 22)
        view.addSubview(colorWell)
        rainOnlyViews.append(colorWell)

        // ── Quit ───────────────────────────────────────────────────────────
        let btn = NSButton(title: "Quit", target: self, action: #selector(quit))
        btn.bezelStyle = .rounded
        btn.frame = NSRect(x: view.bounds.width - pad - 64, y: pad - 4, width: 64, height: 22)
        view.addSubview(btn)

        updateModeVisibility()
    }

    /// Show the direction/color rows only in rain mode.
    private func updateModeVisibility() {
        let isRain = mode == .rain
        rainOnlyViews.forEach { $0.isHidden = !isRain }
    }

    // MARK: - Actions

    @objc private func modeChanged(_ sender: NSSegmentedControl) {
        mode = sender.selectedSegment == 0 ? .rain : .snow
        updateModeVisibility()
        onMode?(mode)
    }

    @objc private func intensityChanged(_ sender: NSSlider) {
        let drops = Int(sender.intValue) * 10   // slider 2…50 → 20…500 particles
        intensityLabel.stringValue = "\(drops)"
        onIntensity?(drops)
    }

    @objc private func directionChanged(_ sender: NSSlider) {
        let d = Int(sender.intValue)
        directionLabel.stringValue = formatDirection(d)
        onDirection?(d)
    }

    @objc private func colorChanged(_ sender: NSColorWell) {
        onColor?(sender.color)
    }

    @objc private func quit() { onQuit?() }

    // MARK: - Helpers

    /// Render the direction value as an arrow + magnitude (e.g. "← 3", "↓", "→ 2").
    private func formatDirection(_ d: Int) -> String {
        switch d {
        case ..<0: return "← \(-d)"
        case 1...: return "→ \(d)"
        default:   return "↓"
        }
    }
}
