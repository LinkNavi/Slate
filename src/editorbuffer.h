#pragma once
#include <QObject>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QStringList>
#include <QStack>

class EditorBuffer : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString filePath READ filePath NOTIFY filePathChanged)
    Q_PROPERTY(QString fileName READ fileName NOTIFY filePathChanged)
    Q_PROPERTY(QString content READ content WRITE setContent NOTIFY contentChanged)
    Q_PROPERTY(bool modified READ modified NOTIFY modifiedChanged)
    Q_PROPERTY(int lineCount READ lineCount NOTIFY contentChanged)
    Q_PROPERTY(QString language READ language NOTIFY filePathChanged)
    Q_PROPERTY(int cursorLine READ cursorLine WRITE setCursorLine NOTIFY cursorLineChanged)
    Q_PROPERTY(int cursorColumn READ cursorColumn WRITE setCursorColumn NOTIFY cursorColumnChanged)

public:
    explicit EditorBuffer(QObject *parent = nullptr) : QObject(parent) {}

    QString filePath() const { return m_filePath; }
    QString fileName() const { return QFileInfo(m_filePath).fileName(); }
    QString content() const { return m_content; }
    bool modified() const { return m_modified; }
    int lineCount() const { return m_content.count('\n') + 1; }
    int cursorLine() const { return m_cursorLine; }
    int cursorColumn() const { return m_cursorColumn; }

    QString language() const {
        QString ext = QFileInfo(m_filePath).suffix().toLower();
        static const QHash<QString, QString> langMap = {
            {"cpp", "C++"}, {"cc", "C++"}, {"cxx", "C++"}, {"h", "C++ Header"},
            {"hpp", "C++ Header"}, {"c", "C"}, {"py", "Python"}, {"js", "JavaScript"},
            {"ts", "TypeScript"}, {"rs", "Rust"}, {"go", "Go"}, {"java", "Java"},
            {"qml", "QML"}, {"html", "HTML"}, {"css", "CSS"}, {"json", "JSON"},
            {"xml", "XML"}, {"md", "Markdown"}, {"txt", "Plain Text"},
            {"sh", "Shell"}, {"bash", "Shell"}, {"cmake", "CMake"},
            {"toml", "TOML"}, {"yaml", "YAML"}, {"yml", "YAML"},
        };
        QString name = QFileInfo(m_filePath).fileName();
        if (name == "CMakeLists.txt") return "CMake";
        if (name == "Makefile" || name == "makefile") return "Makefile";
        if (name == "meson.build") return "Meson";
        return langMap.value(ext, "Plain Text");
    }

    void setContent(const QString &c) {
        if (m_content == c) return;
        m_content = c;
        m_modified = true;
        emit contentChanged();
        emit modifiedChanged();
    }

    void setCursorLine(int l) { if (m_cursorLine != l) { m_cursorLine = l; emit cursorLineChanged(); } }
    void setCursorColumn(int c) { if (m_cursorColumn != c) { m_cursorColumn = c; emit cursorColumnChanged(); } }

    Q_INVOKABLE bool loadFile(const QString &path) {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
        QTextStream in(&f);
        m_content = in.readAll();
        f.close();
        m_filePath = path;
        m_modified = false;
        m_savedContent = m_content;
        emit filePathChanged();
        emit contentChanged();
        emit modifiedChanged();
        return true;
    }

    Q_INVOKABLE bool saveFile() {
        if (m_filePath.isEmpty()) return false;
        return saveFileAs(m_filePath);
    }

    Q_INVOKABLE bool saveFileAs(const QString &path) {
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
        QTextStream out(&f);
        out << m_content;
        f.close();
        m_filePath = path;
        m_modified = false;
        m_savedContent = m_content;
        emit filePathChanged();
        emit modifiedChanged();
        return true;
    }

    Q_INVOKABLE bool isNewFile() const { return m_filePath.isEmpty(); }

    Q_INVOKABLE QString languageId() const {
        QString ext = QFileInfo(m_filePath).suffix().toLower();
        static const QHash<QString, QString> idMap = {
            {"cpp", "cpp"}, {"cc", "cpp"}, {"cxx", "cpp"}, {"h", "cpp"},
            {"hpp", "cpp"}, {"c", "c"}, {"py", "python"}, {"js", "javascript"},
            {"ts", "typescript"}, {"rs", "rust"}, {"go", "go"}, {"java", "java"},
            {"qml", "qml"}, {"html", "html"}, {"css", "css"}, {"json", "json"},
            {"xml", "xml"}, {"md", "markdown"}, {"sh", "shellscript"},
        };
        return idMap.value(ext, "plaintext");
    }

signals:
    void filePathChanged();
    void contentChanged();
    void modifiedChanged();
    void cursorLineChanged();
    void cursorColumnChanged();

private:
    QString m_filePath;
    QString m_content;
    QString m_savedContent;
    bool m_modified = false;
    int m_cursorLine = 1;
    int m_cursorColumn = 1;
};
