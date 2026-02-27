#pragma once
#include <QAbstractListModel>
#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <algorithm>

struct FsEntry {
    QString name;
    QString fullPath;
    bool isDir;
    int depth;
    bool expanded = false;
};

class FileSystemModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString rootPath READ rootPath WRITE setRootPath NOTIFY rootPathChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        FullPathRole,
        IsDirRole,
        DepthRole,
        ExpandedRole
    };

    explicit FileSystemModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}

    int rowCount(const QModelIndex & = {}) const override { return m_entries.size(); }

    QVariant data(const QModelIndex &index, int role) const override {
        if (!index.isValid() || index.row() >= m_entries.size()) return {};
        const auto &e = m_entries[index.row()];
        switch (role) {
            case NameRole: return e.name;
            case FullPathRole: return e.fullPath;
            case IsDirRole: return e.isDir;
            case DepthRole: return e.depth;
            case ExpandedRole: return e.expanded;
        }
        return {};
    }

    QHash<int, QByteArray> roleNames() const override {
        return {
            {NameRole, "name"}, {FullPathRole, "fullPath"},
            {IsDirRole, "isDir"}, {DepthRole, "depth"},
            {ExpandedRole, "expanded"}
        };
    }

    QString rootPath() const { return m_rootPath; }
    void setRootPath(const QString &path) {
        if (m_rootPath == path) return;
        m_rootPath = path;
        m_expandedDirs.clear();
        reload();
        emit rootPathChanged();
    }

    Q_INVOKABLE void toggleDir(int index) {
        if (index < 0 || index >= m_entries.size()) return;
        auto &e = m_entries[index];
        if (!e.isDir) return;
        if (e.expanded) {
            m_expandedDirs.remove(e.fullPath);
        } else {
            m_expandedDirs.insert(e.fullPath);
        }
        reload();
    }

    Q_INVOKABLE void refresh() { reload(); }

    Q_INVOKABLE QString fileExtension(const QString &path) {
        return QFileInfo(path).suffix().toLower();
    }

signals:
    void rootPathChanged();

private:
    void reload() {
        beginResetModel();
        m_entries.clear();
        if (!m_rootPath.isEmpty())
            populateDir(m_rootPath, 0);
        endResetModel();
    }

    void populateDir(const QString &path, int depth) {
        QDir dir(path);
        auto entries = dir.entryInfoList(
            QDir::AllEntries | QDir::NoDotAndDotDot,
            QDir::DirsFirst | QDir::Name | QDir::IgnoreCase
        );
        for (const auto &fi : entries) {
            // Skip hidden files except .gitignore and similar
            if (fi.fileName().startsWith('.') &&
                fi.fileName() != ".gitignore" &&
                fi.fileName() != ".editorconfig")
                continue;
            // Skip build dirs
            if (fi.isDir() && (fi.fileName() == "node_modules" ||
                               fi.fileName() == "build" ||
                               fi.fileName() == ".cache" ||
                               fi.fileName() == "__pycache__"))
                continue;

            FsEntry e;
            e.name = fi.fileName();
            e.fullPath = fi.absoluteFilePath();
            e.isDir = fi.isDir();
            e.depth = depth;
            e.expanded = m_expandedDirs.contains(e.fullPath);
            m_entries.append(e);

            if (e.isDir && e.expanded)
                populateDir(e.fullPath, depth + 1);
        }
    }

    QString m_rootPath;
    QVector<FsEntry> m_entries;
    QSet<QString> m_expandedDirs;
};
