import Cocoa

/// A linear slider cell that draws a plain, uniform track instead of the default
/// leading accent "fill". Used for the bidirectional direction slider so it reads
/// as symmetric around its center (0) rather than like a progress bar filled from
/// the left — matching the Windows trackbar.
private final class CenteredSliderCell: NSSliderCell {
    override func drawBar(inside rect: NSRect, flipped: Bool) {
        let thickness = min(rect.height, 3)
        var bar = rect
        bar.origin.y += (bar.height - thickness) / 2
        bar.size.height = thickness
        let radius = thickness / 2
        NSColor.tertiaryLabelColor.setFill()
        NSBezierPath(roundedRect: bar, xRadius: radius, yRadius: radius).fill()
    }
}

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
    var intensity: Int          = 25   // base unit (5–50); scaled per mode
    var direction: Int          = 0
    var mode: ParticleMode      = .rain
    var dropColor: NSColor      = NSColor(calibratedRed: 0.6, green: 0.82, blue: 1.0, alpha: 1.0)

    private var intensitySlider = NSSlider()
    private var directionSlider = NSSlider()
    private var directionLabel  = NSTextField(labelWithString: "")
    private var colorWell       = NSColorWell()
    private var modeControl     = NSSegmentedControl()

    // Views that are only relevant in rain mode
    private var rainOnlyViews: [NSView] = []

    // MARK: - View lifecycle

    override func loadView() {
        // Plain transparent view: the hosting NSPopover supplies the native
        // vibrancy material, shadow, and rounded corners.
        view = NSView(frame: NSRect(x: 0, y: 0, width: 300, height: 318))
        preferredContentSize = view.bounds.size
        buildUI()
    }

    /// Repo and contact links, mirrored from the Windows settings dialog.
    private let gitHubURL = URL(string: "https://github.com/riyasy/RainProject")!
    private let contactEmail = "ryftools@outlook.com"

    // MARK: - UI construction

    /// Lay out every control by hand (no Auto Layout): a top-down cursor `top`
    /// is decremented as each row is added. Intensity is a base unit (5–50) with
    /// no numeric readout; direction is an integer −5…+5 with tick stops.
    private func buildUI() {
        let pad: CGFloat = 18
        let w   = view.bounds.width - pad * 2
        var top = view.bounds.height - pad

        // ── Title / header ─────────────────────────────────────────────────
        let title = NSTextField(labelWithString: "Let It Rain")
        title.font = .boldSystemFont(ofSize: 14)
        title.alignment = .center
        title.frame = NSRect(x: pad, y: top - 20, width: w, height: 20)
        view.addSubview(title)
        top -= 20 + 14   // reserve exactly the strip added to the view height

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

        func addSliderRow(title: String, slider: NSSlider, valLabel: NSTextField? = nil,
                          min: Double, max: Double, value: Double,
                          action: Selector, intSteps: Bool = false) -> [NSView] {
            top -= 16
            let lbl = headerLabel(title)
            lbl.frame = NSRect(x: pad, y: top, width: 120, height: 14)
            view.addSubview(lbl)

            if let valLabel {
                valLabel.frame = NSRect(x: view.bounds.width - pad - 50, y: top, width: 50, height: 14)
                view.addSubview(valLabel)
            }

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
            var views: [NSView] = [lbl, slider]
            if let valLabel { views.append(valLabel) }
            return views
        }

        // ── Intensity (always visible) ─────────────────────────────────────
        // No numeric readout: the base unit (5–50) is meaningless to the user, so
        // the slider stands alone — matching the Windows dialog.
        _ = addSliderRow(title: "Intensity", slider: intensitySlider,
                         min: 5, max: 50, value: Double(intensity),
                         action: #selector(intensityChanged))

        // ── Direction (rain only) ──────────────────────────────────────────
        // Bidirectional: a plain (unfilled) track so it reads symmetric around 0.
        directionSlider.cell = CenteredSliderCell()
        directionLabel.stringValue = formatDirection(direction)
        let dirViews = addSliderRow(title: "Direction", slider: directionSlider,
                                    valLabel: directionLabel,
                                    min: -5, max: 5, value: Double(direction),
                                    action: #selector(directionChanged), intSteps: true)
        rainOnlyViews += dirViews

        // ── Drop Color (rain only): label + well share one row ─────────────
        top -= 16
        top -= 22
        let clrLbl = headerLabel("Drop Color")
        clrLbl.frame = NSRect(x: pad, y: top + 4, width: 100, height: 14)   // centered on the 22pt well
        view.addSubview(clrLbl)
        rainOnlyViews.append(clrLbl)

        colorWell.color = dropColor
        colorWell.target = self; colorWell.action = #selector(colorChanged)
        colorWell.frame = NSRect(x: pad + 100, y: top, width: 44, height: 22)
        view.addSubview(colorWell)
        rainOnlyViews.append(colorWell)

        // ── Footer (always visible): GitHub link, contact email, Quit ───────
        let sep = NSBox()
        sep.boxType = .separator
        sep.frame = NSRect(x: pad, y: 64, width: w, height: 1)
        view.addSubview(sep)

        // GitHub icon + link, mirroring the Windows settings dialog button.
        let gitHub = NSButton(title: " GitHub", target: self, action: #selector(openGitHub))
        gitHub.isBordered = false
        gitHub.bezelStyle = .recessed
        gitHub.imagePosition = .imageLeading
        gitHub.contentTintColor = .labelColor
        if let icon = NSImage(named: "GitHubIcon") {
            icon.isTemplate = true
            icon.size = NSSize(width: 16, height: 16)
            gitHub.image = icon
        }
        gitHub.sizeToFit()
        gitHub.frame = NSRect(x: pad, y: 38, width: gitHub.frame.width, height: 20)
        view.addSubview(gitHub)

        // Contact email as a mailto link.
        let email = NSButton(title: contactEmail, target: self, action: #selector(openEmail))
        email.isBordered = false
        email.bezelStyle = .recessed
        email.contentTintColor = .secondaryLabelColor
        email.font = .systemFont(ofSize: 11)
        email.sizeToFit()
        email.frame = NSRect(x: pad, y: 14, width: email.frame.width, height: 16)
        view.addSubview(email)

        // ── Quit ───────────────────────────────────────────────────────────
        let btn = NSButton(title: "Quit", target: self, action: #selector(quit))
        btn.bezelStyle = .rounded
        btn.frame = NSRect(x: view.bounds.width - pad - 64, y: 22, width: 64, height: 22)
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
        onIntensity?(Int(sender.intValue))   // base intensity (5…50); scaled per mode
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

    @objc private func openGitHub() { NSWorkspace.shared.open(gitHubURL) }

    @objc private func openEmail() {
        if let url = URL(string: "mailto:\(contactEmail)") {
            NSWorkspace.shared.open(url)
        }
    }

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
