#pragma once
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QQuickTextDocument>
#include <QHash>
#include <QVector>

class SyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
    Q_PROPERTY(QQuickTextDocument* document READ quickDocument WRITE setQuickDocument NOTIFY documentChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QString colorKeyword READ colorKeyword WRITE setColorKeyword NOTIFY colorsChanged)
    Q_PROPERTY(QString colorType READ colorType WRITE setColorType NOTIFY colorsChanged)
    Q_PROPERTY(QString colorString READ colorString WRITE setColorString NOTIFY colorsChanged)
    Q_PROPERTY(QString colorComment READ colorComment WRITE setColorComment NOTIFY colorsChanged)
    Q_PROPERTY(QString colorNumber READ colorNumber WRITE setColorNumber NOTIFY colorsChanged)
    Q_PROPERTY(QString colorFunction READ colorFunction WRITE setColorFunction NOTIFY colorsChanged)
    Q_PROPERTY(QString colorPreprocessor READ colorPreprocessor WRITE setColorPreprocessor NOTIFY colorsChanged)
    Q_PROPERTY(QString colorOperator READ colorOperator WRITE setColorOperator NOTIFY colorsChanged)

public:
    explicit SyntaxHighlighter(QObject *parent = nullptr);
    ~SyntaxHighlighter() override;

    QQuickTextDocument* quickDocument() const;
    void setQuickDocument(QQuickTextDocument *doc);

    QString language() const;
    void setLanguage(const QString &lang);

    QString colorKeyword() const;
    QString colorType() const;
    QString colorString() const;
    QString colorComment() const;
    QString colorNumber() const;
    QString colorFunction() const;
    QString colorPreprocessor() const;
    QString colorOperator() const;

    void setColorKeyword(const QString &c);
    void setColorType(const QString &c);
    void setColorString(const QString &c);
    void setColorComment(const QString &c);
    void setColorNumber(const QString &c);
    void setColorFunction(const QString &c);
    void setColorPreprocessor(const QString &c);
    void setColorOperator(const QString &c);

    Q_INVOKABLE void setDiagnostics(const QVariantList &diags);
    Q_INVOKABLE void setSemanticTokens(const QVariantList &tokens);
    Q_INVOKABLE void clearSemanticTokens();

signals:
    void documentChanged();
    void languageChanged();
    void colorsChanged();

protected:
    void highlightBlock(const QString &text) override;

private:
    struct Rule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    QTextCharFormat makeFormat(const QString &color, bool bold = false, bool italic = false);
    void buildRules();
    void addCppRules(const QTextCharFormat &kw, const QTextCharFormat &type,
                     const QTextCharFormat &str, const QTextCharFormat &num,
                     const QTextCharFormat &fn, const QTextCharFormat &prep,
                     const QTextCharFormat &op);
    void addPythonRules(const QTextCharFormat &kw, const QTextCharFormat &type,
                        const QTextCharFormat &str, const QTextCharFormat &num,
                        const QTextCharFormat &fn, const QTextCharFormat &prep,
                        const QTextCharFormat &op);
    void addJsRules(const QTextCharFormat &kw, const QTextCharFormat &type,
                    const QTextCharFormat &str, const QTextCharFormat &num,
                    const QTextCharFormat &fn, const QTextCharFormat &prep,
                    const QTextCharFormat &op);
    void addRustRules(const QTextCharFormat &kw, const QTextCharFormat &type,
                      const QTextCharFormat &str, const QTextCharFormat &num,
                      const QTextCharFormat &fn, const QTextCharFormat &prep,
                      const QTextCharFormat &op);
    void addGoRules(const QTextCharFormat &kw, const QTextCharFormat &type,
                    const QTextCharFormat &str, const QTextCharFormat &num,
                    const QTextCharFormat &fn, const QTextCharFormat &prep,
                    const QTextCharFormat &op);
    void addQmlRules(const QTextCharFormat &kw, const QTextCharFormat &type,
                     const QTextCharFormat &str, const QTextCharFormat &num,
                     const QTextCharFormat &fn, const QTextCharFormat &prep,
                     const QTextCharFormat &op);
    QTextCharFormat semanticFormat(const QString &typeName, int modifiers);

    QQuickTextDocument *m_quickDoc = nullptr;
    QString m_language;
    QVector<Rule> m_rules;
    QTextCharFormat m_commentFormat;
    bool m_hasBlockComment = false;
    QString m_commentStart, m_commentEnd;
    QVariantList m_diagnostics;

    QString m_colorKeyword    = "#FF7B72";
    QString m_colorType       = "#79C0FF";
    QString m_colorString     = "#A5D6FF";
    QString m_colorComment    = "#8B949E";
    QString m_colorNumber     = "#F2CC60";
    QString m_colorFunction   = "#D2A8FF";
    QString m_colorPreprocessor = "#FFA657";
    QString m_colorOperator   = "#FF7B72";

    QVariantList m_semTokens;
    QHash<int, QVector<QVariantMap>> m_semTokensByLine;
};
