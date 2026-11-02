// Copyright (c) 2026, kyka-zavr.
#include "Templates.hpp"

#include "Paths.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>

QString Templates::renderText(const QString& tmpl, const Theme& theme) {
    static const QRegularExpression RE(R"(\{\{\s*([a-z_]+)(?:\.([a-z]+))?\s*\}\})");

    QString out;
    int     last = 0;
    auto    it   = RE.globalMatch(tmpl);
    while (it.hasNext()) {
        const auto M = it.next();
        out += tmpl.mid(last, M.capturedStart() - last);
        last = M.capturedEnd();

        const auto KEY = M.captured(1);
        const auto FMT = M.captured(2);
        const auto C   = theme.color(KEY); // rrggbbaa

        bool ok = true;
        const int R = C.mid(0, 2).toInt(&ok, 16), G = C.mid(2, 2).toInt(&ok, 16), B = C.mid(4, 2).toInt(&ok, 16), A = C.mid(6, 2).toInt(&ok, 16);

        if (FMT == "nohash")
            out += C.left(6);
        else if (FMT == "rgb")
            out += QString("%1,%2,%3").arg(R).arg(G).arg(B);
        else if (FMT == "rgba")
            out += QString("%1,%2,%3,%4").arg(R).arg(G).arg(B).arg(A / 255.0, 0, 'f', 2);
        else if (FMT == "hex8")
            out += "#" + C;
        else
            out += "#" + C.left(6);
    }
    return out + tmpl.mid(last);
}

QStringList Templates::renderAll(const Theme& theme, QStringList& errors) {
    QStringList written;

    for (const auto& name : QDir(Paths::templatesDir()).entryList({"*.tmpl"}, QDir::Files, QDir::Name)) {
        QFile f(Paths::templatesDir() + "/" + name);
        if (!f.open(QIODevice::ReadOnly)) {
            errors << name + ": cannot read";
            continue;
        }

        const auto TEXT = QString::fromUtf8(f.readAll());
        const auto NL   = TEXT.indexOf('\n');
        const auto HEAD = NL < 0 ? TEXT : TEXT.left(NL);
        if (!HEAD.startsWith("@target:")) {
            errors << name + ": the first line must be '@target: <path>'";
            continue;
        }

        const auto TARGET = Paths::expandHome(HEAD.mid(sizeof("@target:") - 1).trimmed());
        if (TARGET.isEmpty()) {
            errors << name + ": empty target";
            continue;
        }

        QDir().mkpath(QFileInfo(TARGET).absolutePath());
        QSaveFile out(TARGET);
        if (!out.open(QIODevice::WriteOnly)) {
            errors << name + ": cannot write " + TARGET;
            continue;
        }
        out.write(renderText(NL < 0 ? QString() : TEXT.mid(NL + 1), theme).toUtf8());
        if (!out.commit()) {
            errors << name + ": cannot write " + TARGET;
            continue;
        }
        written << TARGET;
    }

    return written;
}
