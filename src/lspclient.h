#pragma once
#include <QObject>
#include <QCoreApplication>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QHash>
#include <QTimer>
#include <functional>

class LspClient : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(QString serverName READ serverName NOTIFY serverNameChanged)
    Q_PROPERTY(QJsonArray diagnostics READ diagnostics NOTIFY diagnosticsChanged)
    Q_PROPERTY(QJsonArray completions READ completions NOTIFY completionsChanged)
    Q_PROPERTY(QVariantList semanticTokens READ semanticTokens NOTIFY semanticTokensChanged)
    Q_PROPERTY(QStringList tokenTypes READ tokenTypes NOTIFY tokenLegendChanged)

public:
    explicit LspClient(QObject *parent = nullptr) : QObject(parent) {
        connect(&m_process, &QProcess::readyReadStandardOutput, this, &LspClient::onReadyRead);
        connect(&m_process, &QProcess::readyReadStandardError, this, [this]() {
            qDebug() << "LSP stderr:" << m_process.readAllStandardError();
        });
        connect(&m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this](int code, QProcess::ExitStatus) {
            qDebug() << "LSP server exited with code" << code;
            m_running = false;
            emit runningChanged();
        });
    }

    ~LspClient() override { shutdown(); }

    bool running() const { return m_running; }
    QString serverName() const { return m_serverName; }
    QJsonArray diagnostics() const { return m_diagnostics; }
    QJsonArray completions() const { return m_completions; }
    QVariantList semanticTokens() const { return m_semanticTokens; }
    QStringList tokenTypes() const { return m_tokenTypes; }

    Q_INVOKABLE void startServer(const QString &command, const QStringList &args, const QString &rootPath) {
        if (m_running) shutdown();

        m_rootPath = rootPath;
        m_process.start(command, args);
        if (!m_process.waitForStarted(3000)) {
            qWarning() << "Failed to start LSP server:" << command;
            return;
        }
        m_running = true;
        m_serverName = command;
        emit runningChanged();
        emit serverNameChanged();
        sendInitialize();
    }

    Q_INVOKABLE void shutdown() {
        if (!m_running) return;
        QJsonObject params;
        sendRequest("shutdown", params);
        sendNotification("exit", {});
        m_process.waitForFinished(2000);
        if (m_process.state() != QProcess::NotRunning)
            m_process.kill();
        m_running = false;
        emit runningChanged();
    }

    Q_INVOKABLE void didOpen(const QString &uri, const QString &languageId, const QString &text) {
        QJsonObject params;
        QJsonObject textDoc;
        textDoc["uri"] = uri;
        textDoc["languageId"] = languageId;
        textDoc["version"] = 1;
        textDoc["text"] = text;
        params["textDocument"] = textDoc;
        sendNotification("textDocument/didOpen", params);
        m_docVersions[uri] = 1;
    }

    Q_INVOKABLE void didChange(const QString &uri, const QString &text) {
        int version = ++m_docVersions[uri];
        QJsonObject params;
        QJsonObject textDoc;
        textDoc["uri"] = uri;
        textDoc["version"] = version;
        params["textDocument"] = textDoc;
        QJsonArray changes;
        QJsonObject change;
        change["text"] = text;
        changes.append(change);
        params["contentChanges"] = changes;
        sendNotification("textDocument/didChange", params);
    }

    Q_INVOKABLE void didClose(const QString &uri) {
        QJsonObject params;
        QJsonObject textDoc;
        textDoc["uri"] = uri;
        params["textDocument"] = textDoc;
        sendNotification("textDocument/didClose", params);
        m_docVersions.remove(uri);
    }

    Q_INVOKABLE void didSave(const QString &uri, const QString &text) {
        QJsonObject params;
        QJsonObject textDoc;
        textDoc["uri"] = uri;
        params["textDocument"] = textDoc;
        params["text"] = text;
        sendNotification("textDocument/didSave", params);
    }

    Q_INVOKABLE void requestCompletion(const QString &uri, int line, int character) {
        QJsonObject params;
        QJsonObject textDoc;
        textDoc["uri"] = uri;
        params["textDocument"] = textDoc;
        QJsonObject pos;
        pos["line"] = line;
        pos["character"] = character;
        params["position"] = pos;

        sendRequest("textDocument/completion", params, [this](const QJsonObject &result) {
            QJsonArray items;
            if (result.contains("items"))
                items = result["items"].toArray();
            else if (result.contains("result")) {
                auto r = result["result"];
                if (r.isArray()) items = r.toArray();
                else if (r.isObject()) items = r.toObject()["items"].toArray();
            }
            m_completions = items;
            emit completionsChanged();
        });
    }

    Q_INVOKABLE void requestHover(const QString &uri, int line, int character) {
        QJsonObject params;
        QJsonObject textDoc;
        textDoc["uri"] = uri;
        params["textDocument"] = textDoc;
        QJsonObject pos;
        pos["line"] = line;
        pos["character"] = character;
        params["position"] = pos;

        sendRequest("textDocument/hover", params, [this](const QJsonObject &result) {
            auto r = result["result"].toObject();
            QString text;
            auto contents = r["contents"];
            if (contents.isString()) text = contents.toString();
            else if (contents.isObject()) text = contents.toObject()["value"].toString();
            emit hoverResult(text);
        });
    }

    Q_INVOKABLE void requestDefinition(const QString &uri, int line, int character) {
        QJsonObject params;
        QJsonObject textDoc;
        textDoc["uri"] = uri;
        params["textDocument"] = textDoc;
        QJsonObject pos;
        pos["line"] = line;
        pos["character"] = character;
        params["position"] = pos;

        sendRequest("textDocument/definition", params, [this](const QJsonObject &result) {
            auto r = result["result"];
            QString targetUri;
            int targetLine = 0, targetChar = 0;
            QJsonObject loc;
            if (r.isArray() && !r.toArray().isEmpty())
                loc = r.toArray().first().toObject();
            else if (r.isObject())
                loc = r.toObject();
            if (!loc.isEmpty()) {
                targetUri = loc["uri"].toString();
                auto range = loc["range"].toObject();
                auto start = range["start"].toObject();
                targetLine = start["line"].toInt();
                targetChar = start["character"].toInt();
            }
            emit definitionResult(targetUri, targetLine, targetChar);
        });
    }

    Q_INVOKABLE void requestSemanticTokens(const QString &uri) {
        if (!m_hasSemanticTokens) return;

        QJsonObject params;
        QJsonObject textDoc;
        textDoc["uri"] = uri;
        params["textDocument"] = textDoc;

        sendRequest("textDocument/semanticTokens/full", params, [this](const QJsonObject &result) {
            auto r = result["result"].toObject();
            auto data = r["data"].toArray();
            if (data.isEmpty()) return;

            // Decode delta-encoded tokens
            // Each token is 5 ints: deltaLine, deltaStartChar, length, tokenType, tokenModifiers
            m_semanticTokens.clear();
            int line = 0, startChar = 0;

            for (int i = 0; i + 4 < data.size(); i += 5) {
                int deltaLine = data[i].toInt();
                int deltaStart = data[i + 1].toInt();
                int length = data[i + 2].toInt();
                int tokenType = data[i + 3].toInt();
                int tokenMods = data[i + 4].toInt();

                if (deltaLine > 0) {
                    line += deltaLine;
                    startChar = deltaStart;
                } else {
                    startChar += deltaStart;
                }

                QVariantMap token;
                token["line"] = line;
                token["startChar"] = startChar;
                token["length"] = length;
                token["tokenType"] = tokenType;
                token["tokenModifiers"] = tokenMods;
                // Resolve type name from legend
                if (tokenType >= 0 && tokenType < m_tokenTypes.size())
                    token["typeName"] = m_tokenTypes[tokenType];
                else
                    token["typeName"] = QString("unknown");

                m_semanticTokens.append(token);
            }

            emit semanticTokensChanged();
        });
    }

    Q_INVOKABLE bool hasSemanticTokens() const { return m_hasSemanticTokens; }

    // Auto-detect which LSP server to use for a language
    Q_INVOKABLE static QString serverCommandForLanguage(const QString &langId) {
        static const QHash<QString, QString> servers = {
            {"cpp", "clangd"}, {"c", "clangd"},
            {"python", "pylsp"},
            {"rust", "rust-analyzer"},
            {"javascript", "typescript-language-server"},
            {"typescript", "typescript-language-server"},
            {"go", "gopls"},
            {"json", "vscode-json-languageserver"},
        };
        return servers.value(langId);
    }

    Q_INVOKABLE static QStringList serverArgsForLanguage(const QString &langId) {
        if (langId == "javascript" || langId == "typescript")
            return {"--stdio"};
        return {};
    }

signals:
    void runningChanged();
    void serverNameChanged();
    void diagnosticsChanged();
    void completionsChanged();
    void semanticTokensChanged();
    void tokenLegendChanged();
    void hoverResult(const QString &text);
    void definitionResult(const QString &uri, int line, int character);

private:
    void sendInitialize() {
        QJsonObject params;
        params["processId"] = QJsonValue((int)QCoreApplication::applicationPid());
        params["rootUri"] = "file://" + m_rootPath;
        QJsonObject capabilities;
        QJsonObject textDocCap;
        QJsonObject completionCap;
        completionCap["dynamicRegistration"] = false;
        QJsonObject completionItemCap;
        completionItemCap["snippetSupport"] = false;
        completionCap["completionItem"] = completionItemCap;
        textDocCap["completion"] = completionCap;
        QJsonObject syncCap;
        syncCap["dynamicRegistration"] = false;
        syncCap["didSave"] = true;
        textDocCap["synchronization"] = syncCap;
        QJsonObject hoverCap;
        hoverCap["dynamicRegistration"] = false;
        textDocCap["hover"] = hoverCap;
        QJsonObject defCap;
        defCap["dynamicRegistration"] = false;
        textDocCap["definition"] = defCap;
        QJsonObject publishDiag;
        publishDiag["relatedInformation"] = true;
        textDocCap["publishDiagnostics"] = publishDiag;

        // Semantic tokens capability
        QJsonObject semTokensCap;
        semTokensCap["dynamicRegistration"] = false;
        QJsonObject semRequests;
        semRequests["full"] = true;
        semTokensCap["requests"] = semRequests;
        QJsonArray semTokenTypes;
        // Standard LSP semantic token types
        for (const auto &t : {"namespace","type","class","enum","interface","struct",
                              "typeParameter","parameter","variable","property",
                              "enumMember","event","function","method","macro",
                              "keyword","modifier","comment","string","number",
                              "regexp","operator","decorator"}) {
            semTokenTypes.append(QString(t));
        }
        QJsonArray semTokenMods;
        for (const auto &m : {"declaration","definition","readonly","static",
                              "deprecated","abstract","async","modification",
                              "documentation","defaultLibrary"}) {
            semTokenMods.append(QString(m));
        }
        semTokensCap["tokenTypes"] = semTokenTypes;
        semTokensCap["tokenModifiers"] = semTokenMods;
        QJsonArray formats;
        formats.append("relative");
        semTokensCap["formats"] = formats;
        textDocCap["semanticTokens"] = semTokensCap;

        capabilities["textDocument"] = textDocCap;
        params["capabilities"] = capabilities;

        sendRequest("initialize", params, [this](const QJsonObject &result) {
            // Parse server capabilities for semantic tokens legend
            auto caps = result["result"].toObject()["capabilities"].toObject();
            auto semProvider = caps["semanticTokensProvider"].toObject();
            if (!semProvider.isEmpty()) {
                m_hasSemanticTokens = true;
                auto legend = semProvider["legend"].toObject();
                auto types = legend["tokenTypes"].toArray();
                m_tokenTypes.clear();
                for (const auto &t : types)
                    m_tokenTypes.append(t.toString());
                auto mods = legend["tokenModifiers"].toArray();
                m_tokenModifiers.clear();
                for (const auto &m : mods)
                    m_tokenModifiers.append(m.toString());
                emit tokenLegendChanged();
            }
            sendNotification("initialized", {});
        });
    }

    void sendRequest(const QString &method, const QJsonObject &params,
                     std::function<void(const QJsonObject&)> handler = nullptr) {
        int id = m_nextId++;
        QJsonObject msg;
        msg["jsonrpc"] = "2.0";
        msg["id"] = id;
        msg["method"] = method;
        msg["params"] = params;
        if (handler) m_handlers[id] = handler;
        sendMessage(msg);
    }

    void sendNotification(const QString &method, const QJsonObject &params) {
        QJsonObject msg;
        msg["jsonrpc"] = "2.0";
        msg["method"] = method;
        msg["params"] = params;
        sendMessage(msg);
    }

    void sendMessage(const QJsonObject &msg) {
        QByteArray content = QJsonDocument(msg).toJson(QJsonDocument::Compact);
        QByteArray header = "Content-Length: " + QByteArray::number(content.size()) + "\r\n\r\n";
        m_process.write(header + content);
    }

    void onReadyRead() {
        m_readBuffer += m_process.readAllStandardOutput();
        while (true) {
            int headerEnd = m_readBuffer.indexOf("\r\n\r\n");
            if (headerEnd < 0) break;
            // Parse Content-Length
            int contentLength = 0;
            QByteArray header = m_readBuffer.left(headerEnd);
            for (auto &line : header.split('\n')) {
                line = line.trimmed();
                if (line.startsWith("Content-Length:"))
                    contentLength = line.mid(15).trimmed().toInt();
            }
            int totalLen = headerEnd + 4 + contentLength;
            if (m_readBuffer.size() < totalLen) break;

            QByteArray content = m_readBuffer.mid(headerEnd + 4, contentLength);
            m_readBuffer.remove(0, totalLen);

            QJsonObject msg = QJsonDocument::fromJson(content).object();
            handleMessage(msg);
        }
    }

    void handleMessage(const QJsonObject &msg) {
        if (msg.contains("id") && !msg.contains("method")) {
            // Response
            int id = msg["id"].toInt();
            if (m_handlers.contains(id)) {
                m_handlers[id](msg);
                m_handlers.remove(id);
            }
        } else if (msg.contains("method")) {
            QString method = msg["method"].toString();
            if (method == "textDocument/publishDiagnostics") {
                auto params = msg["params"].toObject();
                m_diagnostics = params["diagnostics"].toArray();
                m_diagnosticUri = params["uri"].toString();
                emit diagnosticsChanged();
            }
        }
    }

    QProcess m_process;
    QByteArray m_readBuffer;
    int m_nextId = 1;
    bool m_running = false;
    QString m_rootPath;
    QString m_serverName;
    QString m_diagnosticUri;
    QJsonArray m_diagnostics;
    QJsonArray m_completions;
    QHash<int, std::function<void(const QJsonObject&)>> m_handlers;
    QHash<QString, int> m_docVersions;

    // Semantic tokens
    bool m_hasSemanticTokens = false;
    QStringList m_tokenTypes;
    QStringList m_tokenModifiers;
    QVariantList m_semanticTokens;
};
