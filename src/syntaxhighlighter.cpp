#include "syntaxhighlighter.h"
#include <QColor>
#include <QFont>

SyntaxHighlighter::SyntaxHighlighter(QObject *parent)
    : QSyntaxHighlighter(parent) {}

SyntaxHighlighter::~SyntaxHighlighter() = default;

QQuickTextDocument* SyntaxHighlighter::quickDocument() const { return m_quickDoc; }

void SyntaxHighlighter::setQuickDocument(QQuickTextDocument *doc) {
    if (m_quickDoc == doc) return;
    m_quickDoc = doc;
    setDocument(doc ? doc->textDocument() : nullptr);
    emit documentChanged();
}

QString SyntaxHighlighter::language() const { return m_language; }

void SyntaxHighlighter::setLanguage(const QString &lang) {
    if (m_language == lang) return;
    m_language = lang;
    buildRules();
    rehighlight();
    emit languageChanged();
}

QString SyntaxHighlighter::colorKeyword() const { return m_colorKeyword; }
QString SyntaxHighlighter::colorType() const { return m_colorType; }
QString SyntaxHighlighter::colorString() const { return m_colorString; }
QString SyntaxHighlighter::colorComment() const { return m_colorComment; }
QString SyntaxHighlighter::colorNumber() const { return m_colorNumber; }
QString SyntaxHighlighter::colorFunction() const { return m_colorFunction; }
QString SyntaxHighlighter::colorPreprocessor() const { return m_colorPreprocessor; }
QString SyntaxHighlighter::colorOperator() const { return m_colorOperator; }

void SyntaxHighlighter::setColorKeyword(const QString &c)     { m_colorKeyword = c;     buildRules(); rehighlight(); emit colorsChanged(); }
void SyntaxHighlighter::setColorType(const QString &c)        { m_colorType = c;        buildRules(); rehighlight(); emit colorsChanged(); }
void SyntaxHighlighter::setColorString(const QString &c)      { m_colorString = c;      buildRules(); rehighlight(); emit colorsChanged(); }
void SyntaxHighlighter::setColorComment(const QString &c)     { m_colorComment = c;     buildRules(); rehighlight(); emit colorsChanged(); }
void SyntaxHighlighter::setColorNumber(const QString &c)      { m_colorNumber = c;      buildRules(); rehighlight(); emit colorsChanged(); }
void SyntaxHighlighter::setColorFunction(const QString &c)    { m_colorFunction = c;    buildRules(); rehighlight(); emit colorsChanged(); }
void SyntaxHighlighter::setColorPreprocessor(const QString &c){ m_colorPreprocessor = c; buildRules(); rehighlight(); emit colorsChanged(); }
void SyntaxHighlighter::setColorOperator(const QString &c)    { m_colorOperator = c;    buildRules(); rehighlight(); emit colorsChanged(); }

void SyntaxHighlighter::setDiagnostics(const QVariantList &diags) {
    m_diagnostics = diags;
    rehighlight();
}

void SyntaxHighlighter::setSemanticTokens(const QVariantList &tokens) {
    m_semTokens = tokens;
    m_semTokensByLine.clear();
    for (const auto &v : tokens) {
        auto t = v.toMap();
        int line = t["line"].toInt();
        m_semTokensByLine[line].append(t);
    }
    rehighlight();
}

void SyntaxHighlighter::clearSemanticTokens() {
    m_semTokens.clear();
    m_semTokensByLine.clear();
    rehighlight();
}

QTextCharFormat SyntaxHighlighter::makeFormat(const QString &color, bool bold, bool italic) {
    QTextCharFormat fmt;
    if (!color.isEmpty()) fmt.setForeground(QColor(color));
    if (bold) fmt.setFontWeight(QFont::Bold);
    if (italic) fmt.setFontItalic(true);
    return fmt;
}

void SyntaxHighlighter::highlightBlock(const QString &text) {
    for (const auto &rule : m_rules) {
        auto it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            auto match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    if (m_hasBlockComment) {
        setCurrentBlockState(0);
        int startIndex = 0;
        if (previousBlockState() != 1)
            startIndex = text.indexOf(m_commentStart);

        while (startIndex >= 0) {
            int endIndex = text.indexOf(m_commentEnd, startIndex + m_commentStart.length());
            int commentLength;
            if (endIndex == -1) {
                setCurrentBlockState(1);
                commentLength = text.length() - startIndex;
            } else {
                commentLength = endIndex - startIndex + m_commentEnd.length();
            }
            setFormat(startIndex, commentLength, m_commentFormat);
            startIndex = text.indexOf(m_commentStart, startIndex + commentLength);
        }
    }

    int blockNum = currentBlock().blockNumber();
    if (m_semTokensByLine.contains(blockNum)) {
        for (const auto &token : m_semTokensByLine[blockNum]) {
            int startCol = token["startChar"].toInt();
            int length   = token["length"].toInt();
            QString typeName = token["typeName"].toString();
            int mods = token["tokenModifiers"].toInt();
            QTextCharFormat fmt = semanticFormat(typeName, mods);
            if (fmt.isValid())
                setFormat(startCol, length, fmt);
        }
    }

    for (const auto &v : m_diagnostics) {
        auto diag = v.toMap();
        auto range = diag["range"].toMap();
        auto start = range["start"].toMap();
        auto end   = range["end"].toMap();
        int startLine = start["line"].toInt();
        int endLine   = end["line"].toInt();

        if (blockNum >= startLine && blockNum <= endLine) {
            int startCol = (blockNum == startLine) ? start["character"].toInt() : 0;
            int endCol   = (blockNum == endLine)   ? end["character"].toInt()   : text.length();
            endCol = qMin(endCol, text.length());

            int severity = diag["severity"].toInt();
            QTextCharFormat diagFmt;
            diagFmt.setUnderlineStyle(QTextCharFormat::WaveUnderline);
            if (severity == 1)      diagFmt.setUnderlineColor(QColor("#E85454"));
            else if (severity == 2) diagFmt.setUnderlineColor(QColor("#E8C44A"));
            else                    diagFmt.setUnderlineColor(QColor("#4ECDC4"));

            for (int i = startCol; i < endCol; ++i) {
                auto existing = format(i);
                existing.setUnderlineStyle(diagFmt.underlineStyle());
                existing.setUnderlineColor(diagFmt.underlineColor());
                setFormat(i, 1, existing);
            }
        }
    }
}

void SyntaxHighlighter::buildRules() {
    m_rules.clear();
    m_hasBlockComment = false;

    auto kwFmt   = makeFormat(m_colorKeyword, true);
    auto typeFmt = makeFormat(m_colorType);
    auto strFmt  = makeFormat(m_colorString);
    m_commentFormat = makeFormat(m_colorComment, false, true);
    auto numFmt  = makeFormat(m_colorNumber);
    auto fnFmt   = makeFormat(m_colorFunction);
    auto prepFmt = makeFormat(m_colorPreprocessor);
    auto opFmt   = makeFormat(m_colorOperator);

    if (m_language == "cpp" || m_language == "c") {
        addCppRules(kwFmt, typeFmt, strFmt, numFmt, fnFmt, prepFmt, opFmt);
    } else if (m_language == "python") {
        addPythonRules(kwFmt, typeFmt, strFmt, numFmt, fnFmt, prepFmt, opFmt);
    } else if (m_language == "javascript" || m_language == "typescript") {
        addJsRules(kwFmt, typeFmt, strFmt, numFmt, fnFmt, prepFmt, opFmt);
    } else if (m_language == "rust") {
        addRustRules(kwFmt, typeFmt, strFmt, numFmt, fnFmt, prepFmt, opFmt);
    } else if (m_language == "go") {
        addGoRules(kwFmt, typeFmt, strFmt, numFmt, fnFmt, prepFmt, opFmt);
    } else if (m_language == "qml") {
        addQmlRules(kwFmt, typeFmt, strFmt, numFmt, fnFmt, prepFmt, opFmt);
    } else if (m_language == "json") {
        m_rules.append({QRegularExpression(R"("(?:[^"\\]|\\.)*")"), strFmt});
        m_rules.append({QRegularExpression(R"(\b-?\d+\.?\d*(?:[eE][+-]?\d+)?\b)"), numFmt});
        m_rules.append({QRegularExpression(R"(\b(?:true|false|null)\b)"), kwFmt});
    } else if (m_language == "html" || m_language == "xml") {
        m_rules.append({QRegularExpression(R"(</?[a-zA-Z][a-zA-Z0-9-]*\b)"), kwFmt});
        m_rules.append({QRegularExpression(R"(\b[a-zA-Z-]+(?==))"), typeFmt});
        m_rules.append({QRegularExpression(R"("(?:[^"\\]|\\.)*")"), strFmt});
        m_rules.append({QRegularExpression(R"('(?:[^'\\]|\\.)*')"), strFmt});
        m_rules.append({QRegularExpression(R"(<!--.*?-->)"), m_commentFormat});
    } else if (m_language == "css") {
        m_rules.append({QRegularExpression(R"([.#][a-zA-Z_][a-zA-Z0-9_-]*)"), kwFmt});
        m_rules.append({QRegularExpression(R"([a-zA-Z-]+(?=\s*:))"), typeFmt});
        m_rules.append({QRegularExpression(R"("(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*')"), strFmt});
        m_rules.append({QRegularExpression(R"(\b\d+\.?\d*(px|em|rem|%|vh|vw)?\b)"), numFmt});
        m_rules.append({QRegularExpression(R"(#[0-9a-fA-F]{3,8}\b)"), numFmt});
        m_hasBlockComment = true;
        m_commentStart = "/*"; m_commentEnd = "*/";
    } else if (m_language == "shellscript") {
        m_rules.append({QRegularExpression(R"(\b(?:if|then|else|elif|fi|for|while|do|done|case|esac|in|function|return|local|export|source|echo|exit|read|set|unset|shift|trap)\b)"), kwFmt});
        m_rules.append({QRegularExpression(R"(\$\{?[a-zA-Z_][a-zA-Z0-9_]*\}?)"), typeFmt});
        m_rules.append({QRegularExpression(R"("(?:[^"\\]|\\.)*")"), strFmt});
        m_rules.append({QRegularExpression(R"('(?:[^'\\])*')"), strFmt});
        m_rules.append({QRegularExpression(R"(#.*$)"), m_commentFormat});
        m_rules.append({QRegularExpression(R"(\b\d+\b)"), numFmt});
    } else if (m_language == "markdown") {
        m_rules.append({QRegularExpression(R"(^#{1,6}\s+.*)"), kwFmt});
        m_rules.append({QRegularExpression(R"(\*\*.*?\*\*)"), makeFormat(m_colorKeyword, true)});
        m_rules.append({QRegularExpression(R"(\*.*?\*)"), makeFormat(m_colorString, false, true)});
        m_rules.append({QRegularExpression(R"(`[^`]+`)"), typeFmt});
        m_rules.append({QRegularExpression(R"(\[.*?\]\(.*?\))"), fnFmt});
    }
}

void SyntaxHighlighter::addCppRules(const QTextCharFormat &kwFmt, const QTextCharFormat &typeFmt,
                                     const QTextCharFormat &strFmt, const QTextCharFormat &numFmt,
                                     const QTextCharFormat &fnFmt, const QTextCharFormat &prepFmt,
                                     const QTextCharFormat &opFmt) {
    m_rules.append({QRegularExpression(R"(^\s*#\s*\w+)"), prepFmt});
    m_rules.append({QRegularExpression(
        R"(\b(?:alignas|alignof|and|and_eq|asm|auto|bitand|bitor|break|case|catch|class|compl|concept|const|consteval|constexpr|constinit|const_cast|continue|co_await|co_return|co_yield|decltype|default|delete|do|dynamic_cast|else|enum|explicit|export|extern|for|friend|goto|if|inline|mutable|namespace|new|noexcept|not|not_eq|operator|or|or_eq|private|protected|public|register|reinterpret_cast|requires|return|sizeof|static|static_assert|static_cast|struct|switch|template|this|throw|try|typedef|typeid|typename|union|using|virtual|volatile|while|xor|xor_eq|override|final)\b)"),
        kwFmt});
    m_rules.append({QRegularExpression(
        R"(\b(?:bool|char|char8_t|char16_t|char32_t|double|float|int|long|short|signed|unsigned|void|wchar_t|int8_t|int16_t|int32_t|int64_t|uint8_t|uint16_t|uint32_t|uint64_t|size_t|string|vector|map|set|shared_ptr|unique_ptr|nullptr_t|true|false|nullptr|QString|QObject|QList|QHash|QVector|QMap|QSet|QStringList|QVariant|QByteArray)\b)"),
        typeFmt});
    m_rules.append({QRegularExpression(R"(\b([a-zA-Z_]\w*)\s*(?=\())"), fnFmt});
    m_rules.append({QRegularExpression(R"(\b(?:0[xX][0-9a-fA-F]+|0[bB][01]+|\d+\.?\d*(?:[eE][+-]?\d+)?)[fFlLuU]*\b)"), numFmt});
    m_rules.append({QRegularExpression(R"("(?:[^"\\]|\\.)*")"), strFmt});
    m_rules.append({QRegularExpression(R"('(?:[^'\\]|\\.)*')"), strFmt});
    m_rules.append({QRegularExpression(R"(<[a-zA-Z_/][a-zA-Z0-9_./]*>)"), strFmt});
    m_rules.append({QRegularExpression(R"(//.*$)"), m_commentFormat});
    m_hasBlockComment = true;
    m_commentStart = "/*"; m_commentEnd = "*/";
    (void)opFmt;
}

void SyntaxHighlighter::addPythonRules(const QTextCharFormat &kwFmt, const QTextCharFormat &typeFmt,
                                        const QTextCharFormat &strFmt, const QTextCharFormat &numFmt,
                                        const QTextCharFormat &fnFmt, const QTextCharFormat &,
                                        const QTextCharFormat &) {
    m_rules.append({QRegularExpression(
        R"(\b(?:and|as|assert|async|await|break|class|continue|def|del|elif|else|except|finally|for|from|global|if|import|in|is|lambda|nonlocal|not|or|pass|raise|return|try|while|with|yield)\b)"),
        kwFmt});
    m_rules.append({QRegularExpression(
        R"(\b(?:True|False|None|int|float|str|list|dict|set|tuple|bool|bytes|type|object|range|print|len|isinstance|super|property|staticmethod|classmethod)\b)"),
        typeFmt});
    m_rules.append({QRegularExpression(R"(\b([a-zA-Z_]\w*)\s*(?=\())"), fnFmt});
    m_rules.append({QRegularExpression(R"(\b(?:0[xX][0-9a-fA-F]+|0[oO][0-7]+|0[bB][01]+|\d+\.?\d*(?:[eE][+-]?\d+)?j?)\b)"), numFmt});
    m_rules.append({QRegularExpression(R"(""".*?""")"), strFmt});
    m_rules.append({QRegularExpression(R"('''.*?''')"), strFmt});
    m_rules.append({QRegularExpression(R"("(?:[^"\\]|\\.)*")"), strFmt});
    m_rules.append({QRegularExpression(R"('(?:[^'\\]|\\.)*')"), strFmt});
    m_rules.append({QRegularExpression(R"(#.*$)"), m_commentFormat});
    m_rules.append({QRegularExpression(R"(@\w+)"), typeFmt});
}

void SyntaxHighlighter::addJsRules(const QTextCharFormat &kwFmt, const QTextCharFormat &typeFmt,
                                    const QTextCharFormat &strFmt, const QTextCharFormat &numFmt,
                                    const QTextCharFormat &fnFmt, const QTextCharFormat &,
                                    const QTextCharFormat &) {
    m_rules.append({QRegularExpression(
        R"(\b(?:break|case|catch|class|const|continue|debugger|default|delete|do|else|export|extends|finally|for|function|if|import|in|instanceof|let|new|of|return|super|switch|this|throw|try|typeof|var|void|while|with|yield|async|await|from)\b)"),
        kwFmt});
    m_rules.append({QRegularExpression(
        R"(\b(?:true|false|null|undefined|NaN|Infinity|Array|Object|String|Number|Boolean|Map|Set|Promise|Symbol|console|document|window|Math|JSON|Date|RegExp|Error)\b)"),
        typeFmt});
    m_rules.append({QRegularExpression(R"(\b([a-zA-Z_$]\w*)\s*(?=\())"), fnFmt});
    m_rules.append({QRegularExpression(R"(\b(?:0[xX][0-9a-fA-F]+|0[oO][0-7]+|0[bB][01]+|\d+\.?\d*(?:[eE][+-]?\d+)?n?)\b)"), numFmt});
    m_rules.append({QRegularExpression(R"("(?:[^"\\]|\\.)*")"), strFmt});
    m_rules.append({QRegularExpression(R"('(?:[^'\\]|\\.)*')"), strFmt});
    m_rules.append({QRegularExpression(R"(`(?:[^`\\]|\\.)*`)"), strFmt});
    m_rules.append({QRegularExpression(R"(//.*$)"), m_commentFormat});
    m_hasBlockComment = true;
    m_commentStart = "/*"; m_commentEnd = "*/";
}

void SyntaxHighlighter::addRustRules(const QTextCharFormat &kwFmt, const QTextCharFormat &typeFmt,
                                      const QTextCharFormat &strFmt, const QTextCharFormat &numFmt,
                                      const QTextCharFormat &fnFmt, const QTextCharFormat &,
                                      const QTextCharFormat &opFmt) {
    m_rules.append({QRegularExpression(
        R"(\b(?:as|async|await|break|const|continue|crate|dyn|else|enum|extern|fn|for|if|impl|in|let|loop|match|mod|move|mut|pub|ref|return|self|Self|static|struct|super|trait|type|union|unsafe|use|where|while|yield)\b)"),
        kwFmt});
    m_rules.append({QRegularExpression(
        R"(\b(?:bool|char|f32|f64|i8|i16|i32|i64|i128|isize|str|u8|u16|u32|u64|u128|usize|String|Vec|Box|Rc|Arc|Option|Result|Some|None|Ok|Err|true|false)\b)"),
        typeFmt});
    m_rules.append({QRegularExpression(R"(\b([a-zA-Z_]\w*)\s*(?=\())"), fnFmt});
    m_rules.append({QRegularExpression(R"(\b\d+\.?\d*(?:[eE][+-]?\d+)?(?:_?\w+)?\b)"), numFmt});
    m_rules.append({QRegularExpression(R"("(?:[^"\\]|\\.)*")"), strFmt});
    m_rules.append({QRegularExpression(R"('[^'\\]')"), strFmt});
    m_rules.append({QRegularExpression(R"(//.*$)"), m_commentFormat});
    m_rules.append({QRegularExpression(R"(#\[.*?\])"), opFmt});
    m_rules.append({QRegularExpression(R"(\b[A-Z]\w*\b)"), typeFmt});
    m_hasBlockComment = true;
    m_commentStart = "/*"; m_commentEnd = "*/";
}

void SyntaxHighlighter::addGoRules(const QTextCharFormat &kwFmt, const QTextCharFormat &typeFmt,
                                    const QTextCharFormat &strFmt, const QTextCharFormat &numFmt,
                                    const QTextCharFormat &fnFmt, const QTextCharFormat &,
                                    const QTextCharFormat &) {
    m_rules.append({QRegularExpression(
        R"(\b(?:break|case|chan|const|continue|default|defer|else|fallthrough|for|func|go|goto|if|import|interface|map|package|range|return|select|struct|switch|type|var)\b)"),
        kwFmt});
    m_rules.append({QRegularExpression(
        R"(\b(?:bool|byte|complex64|complex128|error|float32|float64|int|int8|int16|int32|int64|rune|string|uint|uint8|uint16|uint32|uint64|uintptr|true|false|nil|iota|append|cap|close|copy|delete|len|make|new|panic|print|println|recover)\b)"),
        typeFmt});
    m_rules.append({QRegularExpression(R"(\b([a-zA-Z_]\w*)\s*(?=\())"), fnFmt});
    m_rules.append({QRegularExpression(R"(\b\d+\.?\d*(?:[eE][+-]?\d+)?\b)"), numFmt});
    m_rules.append({QRegularExpression(R"("(?:[^"\\]|\\.)*")"), strFmt});
    m_rules.append({QRegularExpression(R"(`[^`]*`)"), strFmt});
    m_rules.append({QRegularExpression(R"(//.*$)"), m_commentFormat});
    m_hasBlockComment = true;
    m_commentStart = "/*"; m_commentEnd = "*/";
}

void SyntaxHighlighter::addQmlRules(const QTextCharFormat &kwFmt, const QTextCharFormat &typeFmt,
                                     const QTextCharFormat &strFmt, const QTextCharFormat &numFmt,
                                     const QTextCharFormat &fnFmt, const QTextCharFormat &,
                                     const QTextCharFormat &) {
    m_rules.append({QRegularExpression(
        R"(\b(?:import|property|signal|alias|readonly|default|required|component|function|var|let|const|if|else|for|while|do|switch|case|break|continue|return|true|false|null|undefined)\b)"),
        kwFmt});
    m_rules.append({QRegularExpression(
        R"(\b(?:int|real|double|string|bool|list|var|color|url|date|point|rect|size|font|Item|Rectangle|Text|Image|MouseArea|Column|Row|ColumnLayout|RowLayout|Repeater|ListView|Flickable|Timer|Canvas|Window|ApplicationWindow|Loader)\b)"),
        typeFmt});
    m_rules.append({QRegularExpression(R"(\b[A-Z][a-zA-Z0-9]*\b)"), typeFmt});
    m_rules.append({QRegularExpression(R"(\b([a-zA-Z_]\w*)\s*(?=\())"), fnFmt});
    m_rules.append({QRegularExpression(R"(\b\d+\.?\d*\b)"), numFmt});
    m_rules.append({QRegularExpression(R"("(?:[^"\\]|\\.)*")"), strFmt});
    m_rules.append({QRegularExpression(R"('(?:[^'\\]|\\.)*')"), strFmt});
    m_rules.append({QRegularExpression(R"(//.*$)"), m_commentFormat});
    m_rules.append({QRegularExpression(R"(\b\w+\s*:)"), typeFmt});
    m_hasBlockComment = true;
    m_commentStart = "/*"; m_commentEnd = "*/";
}

QTextCharFormat SyntaxHighlighter::semanticFormat(const QString &typeName, int modifiers) {
    QTextCharFormat fmt;
    bool isBold = false, isItalic = false;

    if (modifiers & (1 << 0)) isBold = true;
    if (modifiers & (1 << 1)) isBold = true;
    if (modifiers & (1 << 3)) isItalic = true;

    if (typeName == "keyword" || typeName == "modifier") {
        fmt.setForeground(QColor(m_colorKeyword)); isBold = true;
    } else if (typeName == "type" || typeName == "class" || typeName == "struct"
               || typeName == "enum" || typeName == "interface" || typeName == "typeParameter") {
        fmt.setForeground(QColor(m_colorType));
    } else if (typeName == "function" || typeName == "method") {
        fmt.setForeground(QColor(m_colorFunction));
    } else if (typeName == "variable" || typeName == "parameter") {
        if (typeName == "parameter") isItalic = true;
        fmt.setForeground(QColor(m_colorType));
    } else if (typeName == "property" || typeName == "enumMember") {
        fmt.setForeground(QColor(m_colorType));
    } else if (typeName == "namespace") {
        fmt.setForeground(QColor(m_colorType)); isItalic = true;
    } else if (typeName == "string") {
        fmt.setForeground(QColor(m_colorString));
    } else if (typeName == "number") {
        fmt.setForeground(QColor(m_colorNumber));
    } else if (typeName == "comment") {
        fmt.setForeground(QColor(m_colorComment)); isItalic = true;
    } else if (typeName == "macro" || typeName == "decorator") {
        fmt.setForeground(QColor(m_colorPreprocessor));
    } else if (typeName == "operator") {
        fmt.setForeground(QColor(m_colorOperator));
    } else if (typeName == "regexp") {
        fmt.setForeground(QColor(m_colorString));
    } else {
        return QTextCharFormat();
    }

    if (isBold) fmt.setFontWeight(QFont::Bold);
    if (isItalic) fmt.setFontItalic(true);
    return fmt;
}
