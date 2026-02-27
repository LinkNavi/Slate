#pragma once
#include <QObject>
#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QSettings>

class ProjectManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString projectRoot READ projectRoot NOTIFY projectRootChanged)
    Q_PROPERTY(QString projectName READ projectName NOTIFY projectRootChanged)
    Q_PROPERTY(bool isGitRepo READ isGitRepo NOTIFY projectRootChanged)
    Q_PROPERTY(QStringList recentProjects READ recentProjects NOTIFY recentProjectsChanged)

public:
    explicit ProjectManager(QObject *parent = nullptr) : QObject(parent) {
        loadRecentProjects();
    }

    QString projectRoot() const { return m_projectRoot; }
    QString projectName() const {
        if (m_projectRoot.isEmpty()) return {};
        return QDir(m_projectRoot).dirName();
    }
    bool isGitRepo() const { return m_isGitRepo; }
    QStringList recentProjects() const { return m_recentProjects; }

    Q_INVOKABLE bool openDirectory(const QString &path) {
        QString root = findProjectRoot(path);
        if (root.isEmpty()) root = path;
        if (!QDir(root).exists()) return false;

        m_projectRoot = root;
        m_isGitRepo = QDir(root + "/.git").exists();
        addRecentProject(root);
        emit projectRootChanged();
        return true;
    }

    Q_INVOKABLE void closeProject() {
        m_projectRoot.clear();
        m_isGitRepo = false;
        emit projectRootChanged();
    }

signals:
    void projectRootChanged();
    void recentProjectsChanged();

private:
    QString findProjectRoot(const QString &path) {
        QDir dir(path);
        if (!dir.exists()) {
            QFileInfo fi(path);
            dir = fi.dir();
        }
        // Walk up looking for .git
        for (int i = 0; i < 20 && dir.exists(); ++i) {
            if (QDir(dir.filePath(".git")).exists())
                return dir.absolutePath();
            if (!dir.cdUp()) break;
        }
        return {};
    }

    void addRecentProject(const QString &path) {
        m_recentProjects.removeAll(path);
        m_recentProjects.prepend(path);
        if (m_recentProjects.size() > 10)
            m_recentProjects.removeLast();
        saveRecentProjects();
        emit recentProjectsChanged();
    }

    void loadRecentProjects() {
        QSettings s;
        m_recentProjects = s.value("recentProjects").toStringList();
    }

    void saveRecentProjects() {
        QSettings s;
        s.setValue("recentProjects", m_recentProjects);
    }

    QString m_projectRoot;
    bool m_isGitRepo = false;
    QStringList m_recentProjects;
};
