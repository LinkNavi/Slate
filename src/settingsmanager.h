#pragma once
#include <QObject>
#include <QSettings>
#include <QJsonObject>
#include <QJsonDocument>

class SettingsManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(int fontSize READ fontSize WRITE setFontSize NOTIFY settingsChanged)
    Q_PROPERTY(int tabWidth READ tabWidth WRITE setTabWidth NOTIFY settingsChanged)
    Q_PROPERTY(bool wordWrap READ wordWrap WRITE setWordWrap NOTIFY settingsChanged)
    Q_PROPERTY(bool showLineNumbers READ showLineNumbers WRITE setShowLineNumbers NOTIFY settingsChanged)
    Q_PROPERTY(bool showMinimap READ showMinimap WRITE setShowMinimap NOTIFY settingsChanged)
    Q_PROPERTY(QString fontFamily READ fontFamily WRITE setFontFamily NOTIFY settingsChanged)

    // LSP server overrides
    Q_PROPERTY(QString lspCpp READ lspCpp WRITE setLspCpp NOTIFY settingsChanged)
    Q_PROPERTY(QString lspPython READ lspPython WRITE setLspPython NOTIFY settingsChanged)
    Q_PROPERTY(QString lspRust READ lspRust WRITE setLspRust NOTIFY settingsChanged)
    Q_PROPERTY(QString lspJs READ lspJs WRITE setLspJs NOTIFY settingsChanged)
    Q_PROPERTY(QString lspGo READ lspGo WRITE setLspGo NOTIFY settingsChanged)
    Q_PROPERTY(bool lspEnabled READ lspEnabled WRITE setLspEnabled NOTIFY settingsChanged)
    Q_PROPERTY(bool lspAutostart READ lspAutostart WRITE setLspAutostart NOTIFY settingsChanged)

public:
    explicit SettingsManager(QObject *parent = nullptr) : QObject(parent) {
        load();
    }

    // Editor
    int fontSize() const { return m_fontSize; }
    int tabWidth() const { return m_tabWidth; }
    bool wordWrap() const { return m_wordWrap; }
    bool showLineNumbers() const { return m_showLineNumbers; }
    bool showMinimap() const { return m_showMinimap; }
    QString fontFamily() const { return m_fontFamily; }

    void setFontSize(int v) { if (m_fontSize != v) { m_fontSize = v; save(); emit settingsChanged(); } }
    void setTabWidth(int v) { if (m_tabWidth != v) { m_tabWidth = v; save(); emit settingsChanged(); } }
    void setWordWrap(bool v) { if (m_wordWrap != v) { m_wordWrap = v; save(); emit settingsChanged(); } }
    void setShowLineNumbers(bool v) { if (m_showLineNumbers != v) { m_showLineNumbers = v; save(); emit settingsChanged(); } }
    void setShowMinimap(bool v) { if (m_showMinimap != v) { m_showMinimap = v; save(); emit settingsChanged(); } }
    void setFontFamily(const QString &v) { if (m_fontFamily != v) { m_fontFamily = v; save(); emit settingsChanged(); } }

    // LSP
    QString lspCpp() const { return m_lspCpp; }
    QString lspPython() const { return m_lspPython; }
    QString lspRust() const { return m_lspRust; }
    QString lspJs() const { return m_lspJs; }
    QString lspGo() const { return m_lspGo; }
    bool lspEnabled() const { return m_lspEnabled; }
    bool lspAutostart() const { return m_lspAutostart; }

    void setLspCpp(const QString &v) { if (m_lspCpp != v) { m_lspCpp = v; save(); emit settingsChanged(); } }
    void setLspPython(const QString &v) { if (m_lspPython != v) { m_lspPython = v; save(); emit settingsChanged(); } }
    void setLspRust(const QString &v) { if (m_lspRust != v) { m_lspRust = v; save(); emit settingsChanged(); } }
    void setLspJs(const QString &v) { if (m_lspJs != v) { m_lspJs = v; save(); emit settingsChanged(); } }
    void setLspGo(const QString &v) { if (m_lspGo != v) { m_lspGo = v; save(); emit settingsChanged(); } }
    void setLspEnabled(bool v) { if (m_lspEnabled != v) { m_lspEnabled = v; save(); emit settingsChanged(); } }
    void setLspAutostart(bool v) { if (m_lspAutostart != v) { m_lspAutostart = v; save(); emit settingsChanged(); } }

    Q_INVOKABLE QString lspServerFor(const QString &langId) const {
        if (!m_lspEnabled) return {};
        if (langId == "cpp" || langId == "c") return m_lspCpp.isEmpty() ? "clangd" : m_lspCpp;
        if (langId == "python") return m_lspPython.isEmpty() ? "pylsp" : m_lspPython;
        if (langId == "rust") return m_lspRust.isEmpty() ? "rust-analyzer" : m_lspRust;
        if (langId == "javascript" || langId == "typescript") return m_lspJs.isEmpty() ? "typescript-language-server" : m_lspJs;
        if (langId == "go") return m_lspGo.isEmpty() ? "gopls" : m_lspGo;
        return {};
    }

    Q_INVOKABLE QStringList lspArgsFor(const QString &langId) const {
        if (langId == "javascript" || langId == "typescript")
            return {"--stdio"};
        return {};
    }

signals:
    void settingsChanged();

private:
    void load() {
        QSettings s;
        s.beginGroup("editor");
        m_fontSize = s.value("fontSize", 13).toInt();
        m_tabWidth = s.value("tabWidth", 4).toInt();
        m_wordWrap = s.value("wordWrap", false).toBool();
        m_showLineNumbers = s.value("showLineNumbers", true).toBool();
        m_showMinimap = s.value("showMinimap", false).toBool();
        m_fontFamily = s.value("fontFamily", "Courier New").toString();
        s.endGroup();

        s.beginGroup("lsp");
        m_lspEnabled = s.value("enabled", true).toBool();
        m_lspAutostart = s.value("autostart", true).toBool();
        m_lspCpp = s.value("cpp", "").toString();
        m_lspPython = s.value("python", "").toString();
        m_lspRust = s.value("rust", "").toString();
        m_lspJs = s.value("js", "").toString();
        m_lspGo = s.value("go", "").toString();
        s.endGroup();
    }

    void save() {
        QSettings s;
        s.beginGroup("editor");
        s.setValue("fontSize", m_fontSize);
        s.setValue("tabWidth", m_tabWidth);
        s.setValue("wordWrap", m_wordWrap);
        s.setValue("showLineNumbers", m_showLineNumbers);
        s.setValue("showMinimap", m_showMinimap);
        s.setValue("fontFamily", m_fontFamily);
        s.endGroup();

        s.beginGroup("lsp");
        s.setValue("enabled", m_lspEnabled);
        s.setValue("autostart", m_lspAutostart);
        s.setValue("cpp", m_lspCpp);
        s.setValue("python", m_lspPython);
        s.setValue("rust", m_lspRust);
        s.setValue("js", m_lspJs);
        s.setValue("go", m_lspGo);
        s.endGroup();
    }

    int m_fontSize = 13;
    int m_tabWidth = 4;
    bool m_wordWrap = false;
    bool m_showLineNumbers = true;
    bool m_showMinimap = false;
    QString m_fontFamily = "Courier New";

    bool m_lspEnabled = true;
    bool m_lspAutostart = true;
    QString m_lspCpp;
    QString m_lspPython;
    QString m_lspRust;
    QString m_lspJs;
    QString m_lspGo;
};
