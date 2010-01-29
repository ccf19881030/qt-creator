/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include <qmljs/qmljshighlighter.h>

#include <QtCore/QSet>
#include <QtCore/QtAlgorithms>
#include <QtCore/QDebug>

using namespace QmlJS;

QScriptHighlighter::QScriptHighlighter(bool duiEnabled, QTextDocument *parent):
        QSyntaxHighlighter(parent),
        m_duiEnabled(duiEnabled)
{
    QVector<QTextCharFormat> rc;
    rc.resize(NumFormats);
    rc[NumberFormat].setForeground(Qt::blue);
    rc[StringFormat].setForeground(Qt::darkGreen);
    rc[TypeFormat].setForeground(Qt::darkMagenta);
    rc[KeywordFormat].setForeground(Qt::darkYellow);
    rc[LabelFormat].setForeground(Qt::darkRed);
    rc[CommentFormat].setForeground(Qt::red); rc[CommentFormat].setFontItalic(true);
    rc[PreProcessorFormat].setForeground(Qt::darkBlue);
    rc[VisualWhitespace].setForeground(Qt::lightGray); // for debug: rc[VisualWhitespace].setBackground(Qt::red);
    setFormats(rc);

    m_scanner.setKeywords(keywords());
}

bool QScriptHighlighter::isDuiEnabled() const
{ return m_duiEnabled; }

static bool checkStartOfBinding(const Token &token)
{
    switch (token.kind) {
    case Token::Semicolon:
    case Token::LeftBrace:
    case Token::RightBrace:
    case Token::LeftBracket:
    case Token::RightBracket:
        return true;

    default:
        return false;
    } // end of switch
}

void QScriptHighlighter::highlightBlock(const QString &text)
{
    const QList<Token> tokens = m_scanner(text, onBlockStart());

    int index = 0;
    while (index < tokens.size()) {
        const Token &token = tokens.at(index);

        switch (token.kind) {
            case Token::Keyword:
                setFormat(token.offset, token.length, m_formats[KeywordFormat]);
                break;

            case Token::String:
                highlightWhitespace(token, text, StringFormat);
                break;

            case Token::Comment:
                highlightWhitespace(token, text, CommentFormat);
                break;

            case Token::Number:
                highlightWhitespace(token, text, NumberFormat);
                break;

            case Token::LeftParenthesis:
                onOpeningParenthesis('(', token.offset);
                break;

            case Token::RightParenthesis:
                onClosingParenthesis(')', token.offset);
                break;

            case Token::LeftBrace:
                onOpeningParenthesis('{', token.offset);
                break;

            case Token::RightBrace:
                onClosingParenthesis('}', token.offset);
                break;

            case Token::LeftBracket:
                onOpeningParenthesis('[', token.offset);
                break;

            case Token::RightBracket:
                onClosingParenthesis(']', token.offset);
                break;

            case Token::Identifier: {
                if (index + 1 < tokens.size()) {
                    if (tokens.at(index + 1).is(Token::LeftBrace) && text.at(token.offset).isUpper()) {
                        setFormat(token.offset, token.length, m_formats[TypeFormat]);
                    } else if (index == 0 || checkStartOfBinding(tokens.at(index - 1))) {
                        const int start = index;

                        ++index; // skip the identifier.
                        while (index + 1 < tokens.size() &&
                               tokens.at(index).is(Token::Dot) &&
                               tokens.at(index + 1).is(Token::Identifier)) {
                            index += 2;
                        }

                        if (index < tokens.size() && tokens.at(index).is(Token::Colon)) {
                            // it's a binding.
                            for (int i = start; i < index; ++i) {
                                const Token &tok = tokens.at(i);
                                setFormat(tok.offset, tok.length, m_formats[LabelFormat]);
                            }
                            break;
                        } else {
                            index = start;
                        }
                    }
                }
            }   break;

            case Token::Delimiter:
                break;

            default:
                break;
        } // end swtich

        ++index;
    }

    int previousTokenEnd = 0;
    for (int index = 0; index < tokens.size(); ++index) {
        const Token &token = tokens.at(index);
        setFormat(previousTokenEnd, token.begin() - previousTokenEnd, m_formats[VisualWhitespace]);

        switch (token.kind) {
        case Token::Comment:
        case Token::String: {
            int i = token.begin(), e = token.end();
            while (i < e) {
                const QChar ch = text.at(i);
                if (ch.isSpace()) {
                    const int start = i;
                    do {
                        ++i;
                    } while (i < e && text.at(i).isSpace());
                    setFormat(start, i - start, m_formats[VisualWhitespace]);
                } else {
                    ++i;
                }
            }
        } break;

        default:
            break;
        } // end of switch

        previousTokenEnd = token.end();
    }

    int firstNonSpace = 0;
    if (! tokens.isEmpty()) {
        const Token &tk = tokens.first();
        firstNonSpace = tk.offset;
    }

    setCurrentBlockState(m_scanner.endState());
    onBlockEnd(m_scanner.endState(), firstNonSpace);
}

void QScriptHighlighter::setFormats(const QVector<QTextCharFormat> &s)
{
    Q_ASSERT(s.size() == NumFormats);
    qCopy(s.constBegin(), s.constEnd(), m_formats);
}

QSet<QString> QScriptHighlighter::keywords()
{
    QSet<QString> keywords;

    keywords << QLatin1String("Infinity");
    keywords << QLatin1String("NaN");
    keywords << QLatin1String("abstract");
    keywords << QLatin1String("boolean");
    keywords << QLatin1String("break");
    keywords << QLatin1String("byte");
    keywords << QLatin1String("case");
    keywords << QLatin1String("catch");
    keywords << QLatin1String("char");
    keywords << QLatin1String("class");
    keywords << QLatin1String("const");
    keywords << QLatin1String("constructor");
    keywords << QLatin1String("continue");
    keywords << QLatin1String("debugger");
    keywords << QLatin1String("default");
    keywords << QLatin1String("delete");
    keywords << QLatin1String("do");
    keywords << QLatin1String("double");
    keywords << QLatin1String("else");
    keywords << QLatin1String("enum");
    keywords << QLatin1String("export");
    keywords << QLatin1String("extends");
    keywords << QLatin1String("false");
    keywords << QLatin1String("final");
    keywords << QLatin1String("finally");
    keywords << QLatin1String("float");
    keywords << QLatin1String("for");
    keywords << QLatin1String("function");
    keywords << QLatin1String("goto");
    keywords << QLatin1String("if");
    keywords << QLatin1String("implements");
    keywords << QLatin1String("import");
    keywords << QLatin1String("in");
    keywords << QLatin1String("instanceof");
    keywords << QLatin1String("int");
    keywords << QLatin1String("interface");
    keywords << QLatin1String("long");
    keywords << QLatin1String("native");
    keywords << QLatin1String("new");
    keywords << QLatin1String("package");
    keywords << QLatin1String("private");
    keywords << QLatin1String("protected");
    keywords << QLatin1String("public");
    keywords << QLatin1String("return");
    keywords << QLatin1String("short");
    keywords << QLatin1String("static");
    keywords << QLatin1String("super");
    keywords << QLatin1String("switch");
    keywords << QLatin1String("synchronized");
    keywords << QLatin1String("this");
    keywords << QLatin1String("throw");
    keywords << QLatin1String("throws");
    keywords << QLatin1String("transient");
    keywords << QLatin1String("true");
    keywords << QLatin1String("try");
    keywords << QLatin1String("typeof");
    keywords << QLatin1String("undefined");
    keywords << QLatin1String("var");
    keywords << QLatin1String("void");
    keywords << QLatin1String("volatile");
    keywords << QLatin1String("while");
    keywords << QLatin1String("with");

    return keywords;
}

int QScriptHighlighter::onBlockStart()
{
    return currentBlockState();
}

void QScriptHighlighter::onBlockEnd(int, int)
{
}

void QScriptHighlighter::onOpeningParenthesis(QChar, int)
{
}

void QScriptHighlighter::onClosingParenthesis(QChar, int)
{
}

void QScriptHighlighter::highlightWhitespace(const Token &token, const QString &text, int nonWhitespaceFormat)
{
    const QTextCharFormat normalFormat = m_formats[nonWhitespaceFormat];
    const QTextCharFormat visualSpaceFormat = m_formats[VisualWhitespace];

    const int end = token.end();
    int index = token.offset;

    while (index != end) {
        const bool isSpace = text.at(index).isSpace();
        const int start = index;

        do { ++index; }
        while (index != end && text.at(index).isSpace() == isSpace);

        const int tokenLength = index - start;
        setFormat(start, tokenLength, isSpace ? visualSpaceFormat : normalFormat);
    }
}
