#include "syntaxhighlighter.h"

SyntaxHighlighter::SyntaxHighlighter(QObject *parent)
    : QSyntaxHighlighter(parent) {}

SyntaxHighlighter::~SyntaxHighlighter() = default;

#include "moc_syntaxhighlighter.cpp"
