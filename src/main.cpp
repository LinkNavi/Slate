#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "projectmanager.h"
#include "filesystemmodel.h"
#include "lspclient.h"
#include "editorbuffer.h"
#include "dialoghelper.h"
#include "thememanager.h"
#include "settingsmanager.h"
#include "syntaxhighlighter.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Slate");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Slate");

    qmlRegisterType<EditorBuffer>("Slate", 1, 0, "EditorBuffer");
    qmlRegisterType<LspClient>("Slate", 1, 0, "LspClient");
    qmlRegisterType<SyntaxHighlighter>("Slate", 1, 0, "SyntaxHighlighter");

    ProjectManager projectManager;
    FileSystemModel fsModel;
    DialogHelper dialogHelper;
    ThemeManager themeManager;
    SettingsManager settingsManager;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("projectManager", &projectManager);
    engine.rootContext()->setContextProperty("fileSystemModel", &fsModel);
    engine.rootContext()->setContextProperty("dialogHelper", &dialogHelper);
    engine.rootContext()->setContextProperty("theme", &themeManager);
    engine.rootContext()->setContextProperty("settings", &settingsManager);

    const QUrl url(u"qrc:/slate/qml/HomeScreen.qml"_qs);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection
    );

    engine.load(url);
    return app.exec();
}
