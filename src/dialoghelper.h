#pragma once
#include <QObject>
#include <QFileDialog>
#include <QGuiApplication>
#include <QClipboard>
#include <QFile>
#include <QDir>
#include <QInputDialog>
#include <QMessageBox>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>

class DialogHelper : public QObject {
    Q_OBJECT
public:
    explicit DialogHelper(QObject *parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE QString openFile() {
        return QFileDialog::getOpenFileName(nullptr, "Open File", QString(), "All files (*)");
    }

    Q_INVOKABLE QString openFolder() {
        return QFileDialog::getExistingDirectory(nullptr, "Open Folder");
    }

    Q_INVOKABLE QString saveFile() {
        return QFileDialog::getSaveFileName(nullptr, "Save As", QString(), "All files (*)");
    }

    Q_INVOKABLE void copyToClipboard(const QString &text) {
        QGuiApplication::clipboard()->setText(text);
    }

    Q_INVOKABLE bool deleteFile(const QString &path) {
        auto result = QMessageBox::question(nullptr, "Delete",
            "Delete \"" + QFileInfo(path).fileName() + "\"?\nThis cannot be undone.",
            QMessageBox::Yes | QMessageBox::No);
        if (result != QMessageBox::Yes) return false;

        QFileInfo fi(path);
        if (fi.isDir())
            return QDir(path).removeRecursively();
        else
            return QFile::remove(path);
    }

    Q_INVOKABLE QString renameFile(const QString &path) {
        QFileInfo fi(path);
        bool ok;
        QString newName = QInputDialog::getText(nullptr, "Rename",
            "New name:", QLineEdit::Normal, fi.fileName(), &ok);
        if (!ok || newName.isEmpty() || newName == fi.fileName()) return {};

        QString newPath = fi.dir().filePath(newName);
        if (QFile::rename(path, newPath))
            return newPath;
        return {};
    }

    Q_INVOKABLE QString createNewFile(const QString &dirPath) {
        bool ok;
        QString name = QInputDialog::getText(nullptr, "New File",
            "File name:", QLineEdit::Normal, "", &ok);
        if (!ok || name.isEmpty()) return {};

        QString fullPath = QDir(dirPath).filePath(name);
        QFile f(fullPath);
        if (f.open(QIODevice::WriteOnly)) {
            f.close();
            return fullPath;
        }
        return {};
    }

    Q_INVOKABLE void openTerminalAt(const QString &path) {
        QProcess::startDetached("x-terminal-emulator", {"--working-directory=" + path});
    }

    Q_INVOKABLE void revealInFiles(const QString &path) {
        QFileInfo fi(path);
        QDesktopServices::openUrl(QUrl::fromLocalFile(fi.isDir() ? path : fi.dir().absolutePath()));
    }
};
