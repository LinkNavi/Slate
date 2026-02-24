#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    app.setApplicationName("Slate");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Slate");

    QQmlApplicationEngine engine;

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
