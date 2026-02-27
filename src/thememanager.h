#pragma once
#include <QObject>
#include <QColor>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QSettings>
#include <QFileSystemWatcher>

class ThemeManager : public QObject {
    Q_OBJECT

    // Core editor colors
    Q_PROPERTY(QString background READ background NOTIFY themeChanged)
    Q_PROPERTY(QString backgroundAlt READ backgroundAlt NOTIFY themeChanged)
    Q_PROPERTY(QString surface READ surface NOTIFY themeChanged)
    Q_PROPERTY(QString surfaceAlt READ surfaceAlt NOTIFY themeChanged)
    Q_PROPERTY(QString border READ border NOTIFY themeChanged)
    Q_PROPERTY(QString borderFocus READ borderFocus NOTIFY themeChanged)
    Q_PROPERTY(QString text READ text NOTIFY themeChanged)
    Q_PROPERTY(QString textDim READ textDim NOTIFY themeChanged)
    Q_PROPERTY(QString textMuted READ textMuted NOTIFY themeChanged)
    Q_PROPERTY(QString accent READ accent NOTIFY themeChanged)
    Q_PROPERTY(QString accentDim READ accentDim NOTIFY themeChanged)
    Q_PROPERTY(QString error READ error NOTIFY themeChanged)
    Q_PROPERTY(QString warning READ warning NOTIFY themeChanged)
    Q_PROPERTY(QString success READ success NOTIFY themeChanged)
    Q_PROPERTY(QString hover READ hover NOTIFY themeChanged)
    Q_PROPERTY(QString selection READ selection NOTIFY themeChanged)
    Q_PROPERTY(QString comment READ comment NOTIFY themeChanged)

    Q_PROPERTY(bool matugenEnabled READ matugenEnabled WRITE setMatugenEnabled NOTIFY settingsChanged)
    Q_PROPERTY(QString matugenColorsPath READ matugenColorsPath WRITE setMatugenColorsPath NOTIFY settingsChanged)
    Q_PROPERTY(QString matugenScheme READ matugenScheme WRITE setMatugenScheme NOTIFY settingsChanged)

public:
    explicit ThemeManager(QObject *parent = nullptr) : QObject(parent) {
        loadSettings();
        connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this](const QString &) {
            if (m_matugenEnabled) loadMatugenColors();
        });
        if (m_matugenEnabled) loadMatugenColors();
    }

    // Color getters
    QString background() const { return m_colors.value("background", "#080C0F"); }
    QString backgroundAlt() const { return m_colors.value("backgroundAlt", "#0C1115"); }
    QString surface() const { return m_colors.value("surface", "#0A1419"); }
    QString surfaceAlt() const { return m_colors.value("surfaceAlt", "#0F1D24"); }
    QString border() const { return m_colors.value("border", "#1A2832"); }
    QString borderFocus() const { return m_colors.value("borderFocus", "#2A4A48"); }
    QString text() const { return m_colors.value("text", "#E8F4F3"); }
    QString textDim() const { return m_colors.value("textDim", "#A8C4C2"); }
    QString textMuted() const { return m_colors.value("textMuted", "#3D6B68"); }
    QString accent() const { return m_colors.value("accent", "#4ECDC4"); }
    QString accentDim() const { return m_colors.value("accentDim", "#2A6B4A"); }
    QString error() const { return m_colors.value("error", "#E85454"); }
    QString warning() const { return m_colors.value("warning", "#E8C44A"); }
    QString success() const { return m_colors.value("success", "#4ECDC4"); }
    QString hover() const { return m_colors.value("hover", "#162028"); }
    QString selection() const { return m_colors.value("selection", "#264A47"); }
    QString comment() const { return m_colors.value("comment", "#2A6B4A"); }

    bool matugenEnabled() const { return m_matugenEnabled; }
    QString matugenColorsPath() const { return m_matugenColorsPath; }
    QString matugenScheme() const { return m_matugenScheme; }

    void setMatugenEnabled(bool v) {
        if (m_matugenEnabled == v) return;
        m_matugenEnabled = v;
        saveSettings();
        if (v) loadMatugenColors();
        else resetToDefaults();
        emit settingsChanged();
    }

    void setMatugenColorsPath(const QString &p) {
        if (m_matugenColorsPath == p) return;
        // Stop watching old path
        if (!m_matugenColorsPath.isEmpty())
            m_watcher.removePath(m_matugenColorsPath);
        m_matugenColorsPath = p;
        saveSettings();
        if (m_matugenEnabled) loadMatugenColors();
        emit settingsChanged();
    }

    void setMatugenScheme(const QString &s) {
        if (m_matugenScheme == s) return;
        m_matugenScheme = s;
        saveSettings();
        if (m_matugenEnabled) loadMatugenColors();
        emit settingsChanged();
    }

    Q_INVOKABLE void resetToDefaults() {
        m_colors.clear();
        emit themeChanged();
    }

    Q_INVOKABLE void reloadMatugen() {
        if (m_matugenEnabled) loadMatugenColors();
    }

    Q_INVOKABLE QString resolveHome(const QString &path) {
        QString p = path;
        if (p.startsWith("~"))
            p.replace(0, 1, QDir::homePath());
        return p;
    }

signals:
    void themeChanged();
    void settingsChanged();

private:
    void loadSettings() {
        QSettings s;
        s.beginGroup("theme");
        m_matugenEnabled = s.value("matugenEnabled", false).toBool();
        m_matugenColorsPath = s.value("matugenColorsPath",
            QDir::homePath() + "/.cache/matugen/colors.json").toString();
        m_matugenScheme = s.value("matugenScheme", "dark").toString();
        s.endGroup();
    }

    void saveSettings() {
        QSettings s;
        s.beginGroup("theme");
        s.setValue("matugenEnabled", m_matugenEnabled);
        s.setValue("matugenColorsPath", m_matugenColorsPath);
        s.setValue("matugenScheme", m_matugenScheme);
        s.endGroup();
    }

    void loadMatugenColors() {
        QString path = resolveHome(m_matugenColorsPath);
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) {
            qWarning() << "ThemeManager: cannot open matugen colors:" << path;
            return;
        }

        auto doc = QJsonDocument::fromJson(f.readAll());
        f.close();
        if (!doc.isObject()) return;

        auto root = doc.object();

        // Try to find the scheme colors
        QJsonObject schemeColors;

        // Format 1: { "colors": { "dark": { ... }, "light": { ... } } }
        if (root.contains("colors") && root["colors"].isObject()) {
            auto colors = root["colors"].toObject();
            if (colors.contains(m_matugenScheme))
                schemeColors = colors[m_matugenScheme].toObject();
            else if (!colors.isEmpty())
                schemeColors = colors.begin().value().toObject();
        }
        // Format 2: flat { "primary": "#xxx", ... }
        else if (root.contains("primary")) {
            schemeColors = root;
        }

        if (schemeColors.isEmpty()) return;

        // Map Material You colors to Slate theme
        auto get = [&](const QString &key) -> QString {
            if (schemeColors.contains(key))
                return schemeColors[key].toString();
            return {};
        };

        QString bg = get("background");
        QString surface_ = get("surface");
        QString surfVar = get("surface_variant");
        QString primary = get("primary");
        QString primaryCont = get("primary_container");
        QString onSurf = get("on_surface");
        QString onSurfVar = get("on_surface_variant");
        QString outline = get("outline");
        QString outlineVar = get("outline_variant");
        QString errorC = get("error");
        QString tertiary = get("tertiary");
        QString secondary = get("secondary");
        QString secondaryCont = get("secondary_container");
        QString onBg = get("on_background");

        if (!bg.isEmpty()) m_colors["background"] = bg;
        if (!surface_.isEmpty()) {
            m_colors["backgroundAlt"] = surface_;
            m_colors["surface"] = darken(surface_, 0.15);
            m_colors["surfaceAlt"] = lighten(surface_, 0.1);
        }
        if (!outlineVar.isEmpty()) {
            m_colors["border"] = darken(outlineVar, 0.3);
            m_colors["borderFocus"] = outlineVar;
        }
        if (!onBg.isEmpty()) m_colors["text"] = onBg;
        if (!onSurf.isEmpty()) m_colors["text"] = onSurf;
        if (!onSurfVar.isEmpty()) {
            m_colors["textDim"] = onSurfVar;
        }
        if (!outline.isEmpty()) m_colors["textMuted"] = outline;
        if (!primary.isEmpty()) {
            m_colors["accent"] = primary;
            m_colors["success"] = primary;
        }
        if (!primaryCont.isEmpty()) m_colors["accentDim"] = primaryCont;
        if (!errorC.isEmpty()) m_colors["error"] = errorC;
        if (!tertiary.isEmpty()) m_colors["warning"] = tertiary;
        if (!secondaryCont.isEmpty()) {
            m_colors["hover"] = darken(secondaryCont, 0.5);
            m_colors["selection"] = darken(secondaryCont, 0.3);
        }
        if (!secondary.isEmpty()) m_colors["comment"] = darken(secondary, 0.3);

        // Watch for live changes
        if (!m_watcher.files().contains(path) && QFile::exists(path))
            m_watcher.addPath(path);

        emit themeChanged();
    }

    static QString darken(const QString &hex, double amount) {
        QColor c(hex);
        float h, s, l, a;
        c.getHslF(&h, &s, &l, &a);
        l = qMax(0.0f, l - (float)amount);
        c.setHslF(h, s, l, a);
        return c.name();
    }

    static QString lighten(const QString &hex, double amount) {
        QColor c(hex);
        float h, s, l, a;
        c.getHslF(&h, &s, &l, &a);
        l = qMin(1.0f, l + (float)amount);
        c.setHslF(h, s, l, a);
        return c.name();
    }

    QHash<QString, QString> m_colors;
    bool m_matugenEnabled = false;
    QString m_matugenColorsPath;
    QString m_matugenScheme;
    QFileSystemWatcher m_watcher;
};
